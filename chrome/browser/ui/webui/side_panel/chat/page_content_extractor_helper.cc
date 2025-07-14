// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "page_content_extractor_helper.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "chrome/common/page_content_extractor/page_content_extractor.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace {

class PageContentExtractorInternal {
 public:
    PageContentExtractorInternal() {}

    void Start(
        mojo::Remote<chat::mojom::PageContentExtractor> content_extractor,
        base::OnceCallback<void(std::string content, std::string url)> callback,
        bool includesHTML) {
      content_extractor_ = std::move(content_extractor);
      if (!content_extractor_) {
        DeleteSelf();
        return;
      }

      // Ref:
      // https://chromium.googlesource.com/chromium/src/+/refs/heads/main/mojo/public/cpp/bindings/README.md#a-note-about-endpoint-lifetime-and-callbacks
      // Once a `mojo::Remote<T>` is destroyed, it is guaranteed that pending
      // callbacks as well as the connection error handler (if registered) won't
      // be called. Once a `mojo::Receiver<T>` is destroyed, it is guaranteed
      // that no more method calls are dispatched to the implementation and the
      // connection error handler (if registered) won't be called.
      content_extractor_.set_disconnect_handler(base::BindOnce(
          &PageContentExtractorInternal::DeleteSelf, base::Unretained(this)));
      content_extractor_->ExtractPageContent(
          includesHTML,
          base::BindOnce(&PageContentExtractorInternal::OnPageContentExtracted,
                         base::Unretained(this), std::move(callback)));
    }

    void OnPageContentExtracted(
        base::OnceCallback<void(std::string content, std::string url)> callback,
        const std::optional<std::string>& content,
        const std::optional<std::string>& url) {
      if (!content.has_value()) {
        DVLOG(0) << __func__ << " |>> Extracted content is null.";
        SendResultAndDeleteSelf(std::move(callback));
        return;
      }

      if (content->empty()) {
        DVLOG(0) << __func__ << " |>> Extracted content is empty.";
        SendResultAndDeleteSelf(std::move(callback));
        return;
      }

      if (!url.has_value()) {
        DVLOG(0) << __func__ << " |>> url to extract content is null.";
        SendResultAndDeleteSelf(std::move(callback));
        return;
        }

        if (url->empty()) {
          DVLOG(0) << __func__ << " |>> url to extract content is empty.";
          SendResultAndDeleteSelf(std::move(callback));
          return;
        }

      SendResultAndDeleteSelf(std::move(callback), content.value(), url.value());
    }

 private:
  void DeleteSelf() { delete this; }

  void SendResultAndDeleteSelf(base::OnceCallback<void(std::string content, std::string url)> callback,
                               std::string content = "", std::string url = "") {
    std::move(callback).Run(content, url);
    delete this;
  }

  mojo::Remote<chat::mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentExtractorInternal> weak_ptr_factory_{this};
};
}  // namespace

PageContentExtractorHelper::PageContentExtractorHelper(
    content::WebContents* web_contents)
    : web_contents_(web_contents->GetWeakPtr()) {}

PageContentExtractorHelper::~PageContentExtractorHelper() = default;

void PageContentExtractorHelper::ExtractPageContent(
    base::OnceCallback<void(std::string content, std::string url)> callback,
    bool includesHTML) {
  auto* primary_rfh = web_contents_->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  mojo::Remote<chat::mojom::PageContentExtractor> extractor;
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());

  auto* internal_extractor = new PageContentExtractorInternal();
  internal_extractor->Start(std::move(extractor), std::move(callback),
                            includesHTML);
}
