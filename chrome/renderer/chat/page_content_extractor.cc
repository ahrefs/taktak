// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "page_content_extractor.h"

#include <iterator>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include "base/containers/contains.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree.h"
#include "v8/include/v8-isolate.h"

namespace ai_chat {
namespace {
static const ax::mojom::Role kContentParentRoles[]{
    ax::mojom::Role::kMain,
    ax::mojom::Role::kArticle,
};

static const ax::mojom::Role kContentRoles[]{
    ax::mojom::Role::kHeading,
    ax::mojom::Role::kParagraph,
    ax::mojom::Role::kNote,
};

static const ax::mojom::Role kRolesToSkip[]{
    ax::mojom::Role::kAudio,          ax::mojom::Role::kBanner,
    ax::mojom::Role::kButton,         ax::mojom::Role::kComplementary,
    ax::mojom::Role::kContentInfo,    ax::mojom::Role::kFooter,
    ax::mojom::Role::kImage,          ax::mojom::Role::kLabelText,
    ax::mojom::Role::kNavigation,     ax::mojom::Role::kSectionFooter,
    ax::mojom::Role::kTextField,      ax::mojom::Role::kTextFieldWithComboBox,
    ax::mojom::Role::kComboBoxSelect, ax::mojom::Role::kListBox,
    ax::mojom::Role::kListBoxOption,  ax::mojom::Role::kCheckBox,
    ax::mojom::Role::kRadioButton,    ax::mojom::Role::kSlider,
    ax::mojom::Role::kSpinButton,     ax::mojom::Role::kSearchBox,
};

void GetContentRootNodes(const ui::AXNode* root,
                         std::vector<const ui::AXNode*>* content_root_nodes) {
  std::queue<const ui::AXNode*> queue;
  queue.push(root);
  while (!queue.empty()) {
    const ui::AXNode* node = queue.front();
    queue.pop();
    // If a main or article node is found, add it to the list of content root
    // nodes and continue. Do not explore children for nested article nodes.
    if (base::Contains(kContentParentRoles, node->GetRole())) {
      content_root_nodes->push_back(node);
      continue;
    }
    for (auto iter = node->UnignoredChildrenBegin();
         iter != node->UnignoredChildrenEnd(); ++iter) {
      queue.push(iter.get());
    }
  }
}

void AddContentNodesToVector(const ui::AXNode* node,
                             std::vector<const ui::AXNode*>* content_nodes) {
  if (base::Contains(kContentRoles, node->GetRole())) {
    content_nodes->emplace_back(node);
    return;
  }
  if (base::Contains(kRolesToSkip, node->GetRole())) {
    return;
  }
  for (auto iter = node->UnignoredChildrenBegin();
       iter != node->UnignoredChildrenEnd(); ++iter) {
    AddContentNodesToVector(iter.get(), content_nodes);
  }
}

void AddTextNodesToVector(const ui::AXNode* node,
                          std::vector<std::u16string>* strings) {
  const ui::AXNodeData& node_data = node->data();

  if (base::Contains(kRolesToSkip, node_data.role)) {
    return;
  }

  if (node_data.role == ax::mojom::Role::kStaticText) {
    if (node_data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      strings->push_back(
          node_data.GetString16Attribute(ax::mojom::StringAttribute::kName));
    }
    return;
  }

  for (const ui::AXNode* child : node->children()) {
    AddTextNodesToVector(child, strings);
  }
}
}  // namespace

PageContentExtractor::PageContentExtractor(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry,
    int32_t isolated_world_id)
    : content::RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PageContentExtractor>(render_frame),
      isolated_world_id_(isolated_world_id),
      weak_ptr_factory_(this) {
  if (!render_frame->IsMainFrame()) {
    return;
  }
  registry->AddInterface(base::BindRepeating(
      &PageContentExtractor::BindReceiver, base::Unretained(this)));
}

PageContentExtractor::~PageContentExtractor() = default;

void PageContentExtractor::OnDestruct() {
  delete this;
}

base::WeakPtr<PageContentExtractor> PageContentExtractor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PageContentExtractor::BindReceiver(
    mojo::PendingReceiver<chat::mojom::PageContentExtractor> receiver) {
  DVLOG(0) << "Yep Chat PageContentExtractor handler bound.";
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void PageContentExtractor::ExtractPageContent(
    bool includesHTML,
    chat::mojom::PageContentExtractor::ExtractPageContentCallback callback) {
  DVLOG(0) << __func__ << "The current page will be extracted for Yep Chat.";
  ExtractPageText(
      render_frame(), isolated_world_id_,
      base::BindOnce(&PageContentExtractor::OnPageTextExtracted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      includesHTML);
}

void PageContentExtractor::ExtractPageText(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id,
    base::OnceCallback<void(const std::optional<std::string>&,
                            const std::optional<std::string>&)> callback,
    bool includesHTML) {
  auto snapshotter = render_frame->CreateAXTreeSnapshotter(
      ui::AXMode::kWebContents | ui::AXMode::kHTML | ui::AXMode::kOnScreenOnly);
  ui::AXTreeUpdate snapshot;
  snapshotter->Snapshot(
      /* max_nodes= */ 9000, /* timeout= */ base::Seconds(4), &snapshot);
  ui::AXTree tree(snapshot);

  std::vector<const ui::AXNode*> content_root_nodes;
  std::vector<const ui::AXNode*> content_nodes;
  GetContentRootNodes(tree.root(), &content_root_nodes);

  for (const ui::AXNode* content_root_node : content_root_nodes) {
    std::vector<const ui::AXNode*> content_nodes_this_root;
    AddContentNodesToVector(content_root_node, &content_nodes_this_root);
    // If no content was retrieved for this root node, fall back to using
    // the text directly from the root node. This ensures we capture content
    // from the node identified as containing important information.
    if (content_nodes_this_root.empty()) {
      content_nodes.emplace_back(content_root_node);
    } else {
      std::ranges::move(content_nodes_this_root,
                        std::back_inserter(content_nodes));
    }
  }

  std::vector<std::u16string> text_node_contents;
  for (const ui::AXNode* content_node : content_nodes) {
    AddTextNodesToVector(content_node, &text_node_contents);
  }

  std::string contents_text =
      base::UTF16ToUTF8(base::JoinString(text_node_contents, u" "));

  blink::WebLocalFrame* main_frame = render_frame->GetWebFrame();

  // Retrieve the current URL
  blink::WebURL web_url = main_frame->GetDocumentLoader()->GetUrl();

  // Convert to GURL for convenience
  GURL url(web_url);

  if (contents_text.empty()) {
    v8::HandleScope handle_scope(
        main_frame->GetAgentGroupScheduler()->Isolate());
    std::string script =
        "(function () {\n"
        "    const unwantedTopLevelTags = new Set([\n"
        "        'META', 'LINK', 'HEAD', 'SCRIPT', 'NOSCRIPT',\n"
        "        'STYLE', 'IFRAME', 'IMG', 'FIGURE', 'HEADER', 'FOOTER'\n"
        "    ]);\n"
        "    \n"
        "    const unwantedTags = new Set([\n"
        "        'META', 'LINK', 'SCRIPT', 'NOSCRIPT', 'ASIDE', 'NAV',\n"
        "        'STYLE', 'IFRAME', 'IMG', 'FIGURE', 'FOOTER'\n"
        "    ]);\n"
        "\n"
        "    const isVisible = (element) => {\n"
        "        const style = window.getComputedStyle(element);\n"
        "        return style.display !== 'none' && style.visibility !== "
        "'hidden' && style.opacity !== '0';\n"
        "    };\n"
        "\n"
        "    return Array.from(document.body.children)\n"
        "        .filter(e => !unwantedTopLevelTags.has(e.tagName) && "
        "isVisible(e))\n"
        "        .map(e => {\n"
        "            const clone = e.cloneNode(true);\n"
        "            clone.querySelectorAll('*').forEach(child => {\n"
        "                if (unwantedTags.has(child.tagName) || "
        "!child.textContent.trim()) {\n"
        "                    child.remove();\n"
        "                } else {\n"
        "                    while (child.attributes.length > 0) {\n"
        "                        "
        "child.removeAttribute(child.attributes[0].name);\n"
        "                    }\n"
        "                }\n"
        "            });\n"
        "            return clone.outerHTML;\n"
        "        })\n"
        "        .join('');\n"
        "})()";

    blink::WebScriptSource source =
        includesHTML
            ? blink::WebScriptSource(blink::WebString::FromASCII(script))
            : blink::WebScriptSource(
                  blink::WebString::FromASCII("document.body.innerText"));

    auto on_script_executed =
        [](base::OnceCallback<void(const std::optional<std::string>&,
                                   const std::optional<std::string>&)> callback,
           std::string url, std::optional<base::Value> value,
           base::TimeTicks start_time) {
          if (value && value->is_string()) {
            std::move(callback).Run(value->GetString(), url);
          } else {
            std::move(callback).Run("", "");
          }
        };

    render_frame->GetWebFrame()->RequestExecuteScript(
        isolated_world_id, UNSAFE_TODO(base::span(&source, 1u)),
        blink::mojom::UserActivationOption::kDoNotActivate,
        blink::mojom::EvaluationTiming::kAsynchronous,
        blink::mojom::LoadEventBlockingOption::kDoNotBlock,
        base::BindOnce(on_script_executed, std::move(callback), url.spec()),
        blink::BackForwardCacheAware::kAllow,
        blink::mojom::WantResultOption::kWantResult,
        blink::mojom::PromiseResultOption::kAwait);
  } else {
    std::move(callback).Run(std::move(contents_text), std::move(url.spec()));
  }
}

void PageContentExtractor::OnPageTextExtracted(
    chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
    const std::optional<std::string>& content,
    const std::optional<std::string>& url) {
  std::move(callback).Run(std::move(content), std::move(url));
}
}  // namespace ai_chat
