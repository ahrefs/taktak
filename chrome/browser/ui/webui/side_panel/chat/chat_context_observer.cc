#include "chat_context_observer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/renderer/chat/page_content_extractor.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/accessibility/ax_mode.h"
#include "ui/accessibility/ax_updates_and_events.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {
constexpr auto kVideoPageContentTypes =
    base::MakeFixedFlatSet<chat::mojom::PageContentType>(
        {chat::mojom::PageContentType::VideoTranscriptYouTube,
         chat::mojom::PageContentType::VideoTranscriptVTT});

using ExtractPageContentCallback =
    ChatContextObserver::ExtractPageContentCallback;

class PageContentExtractorHelper {
 public:
  PageContentExtractorHelper() {}

  void Start(mojo::Remote<chat::mojom::PageContentExtractor> content_extractor,
             ExtractPageContentCallback callback) {
    content_extractor_ = std::move(content_extractor);
    if (!content_extractor_) {
      DeleteSelf();
      return;
    }

    // Ref:
    // https://chromium.googlesource.com/chromium/src/+/refs/heads/main/mojo/public/cpp/bindings/README.md#a-note-about-endpoint-lifetime-and-callbacks
    // Once a `mojo::Remote<T>` is destroyed, it is guaranteed that pending
    // callbacks as well as the connection error handler (if registered) won't
    // be called. Once a `mojo::Receiver<T>` is destroyed, it is guaranteed that
    // no more method calls are dispatched to the implementation and the
    // connection error handler (if registered) won't be called.
    content_extractor_.set_disconnect_handler(base::BindOnce(
        &PageContentExtractorHelper::DeleteSelf, base::Unretained(this)));
    content_extractor_->ExtractPageContent(
        base::BindOnce(&PageContentExtractorHelper::OnPageContentExtracted,
                       base::Unretained(this), std::move(callback)));
  }

  void OnPageContentExtracted(ExtractPageContentCallback callback,
                              chat::mojom::PageContentPtr data) {
    if (!data) {
      DVLOG(0) << __func__ << " no extracted page content.";
      SendResultAndDeleteSelf(std::move(callback));
      return;
    }

    DVLOG(1) << "OnTabContentResult: " << data.get();
    const bool is_video = base::Contains(kVideoPageContentTypes, data->type);
    DVLOG(1) << "Is video? " << is_video;

    if (!is_video) {
      DCHECK(data->content->is_content());
      auto content = data->content->get_content();
      DVLOG(1) << __func__ << ": Got content with char length of "
               << content.length();
      SendResultAndDeleteSelf(std::move(callback), content);
      return;
    }

    SendResultAndDeleteSelf(std::move(callback));
  }

 private:
  void DeleteSelf() { delete this; }
  void SendResultAndDeleteSelf(ExtractPageContentCallback callback,
                               std::string content = "") {
    std::move(callback).Run(content);
    delete this;
  }
  mojo::Remote<chat::mojom::PageContentExtractor> content_extractor_;
  base::WeakPtrFactory<PageContentExtractorHelper> weak_ptr_factory_{this};
};
}  // namespace

// static
void ChatContextObserver::BindPageContentExtractorHost(
    content::RenderFrameHost* rfh,
    mojo::PendingAssociatedReceiver<chat::mojom::PageContentExtractorHost>
        receiver) {
  CHECK(rfh);
  if (!rfh->IsInPrimaryMainFrame()) {
    DVLOG(1) << "Render frame is not in primary main frame. Not binding to "
                "extractor host.";
    return;
  }
  auto* sender = content::WebContents::FromRenderFrameHost(rfh);
  if (!sender) {
    DVLOG(1) << "Cannot bind extractor host, no valid WebContents";
    return;
  }
  auto* chat_context_observer = ChatContextObserver::FromWebContents(sender);
  if (!chat_context_observer) {
    DVLOG(1) << "Cannot bind extractor host, no ChatContextObserver - "
             << sender->GetVisibleURL();
    return;
  }
  DVLOG(1) << "Binding extractor host to ChatContextObserver";
  chat_context_observer->BindPageContentExtractorReceiver(std::move(receiver));
}

void ChatContextObserver::BindPageContentExtractorReceiver(
    mojo::PendingAssociatedReceiver<chat::mojom::PageContentExtractorHost>
        receiver) {
  page_content_extractor_receiver_.reset();
  page_content_extractor_receiver_.Bind(std::move(receiver));
}

ChatContextObserver::ChatContextObserver(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<ChatContextObserver>(*web_contents) {
  previous_page_title_ = web_contents->GetTitle();
}

ChatContextObserver::~ChatContextObserver() = default;

void ChatContextObserver::SetPendingGetContentCallback(
    ExtractPageContentCallback callback) {
  if (pending_extract_page_content_callback_) {
    std::move(pending_extract_page_content_callback_).Run("");
  }
  pending_extract_page_content_callback_ = std::move(callback);
}

GURL ChatContextObserver::GetPageURL() const {
  return web_contents()->GetLastCommittedURL();
}

void ChatContextObserver::GetPageContent(ExtractPageContentCallback callback) {
    // fix: this doesn't work properly, need to introduce intermediate class to capture related WebContents
  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();
  DCHECK(primary_rfh->IsRenderFrameLive());

  mojo::Remote<chat::mojom::PageContentExtractor> extractor;
  primary_rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());

  auto* extractor_helper = new PageContentExtractorHelper();
  extractor_helper->Start(std::move(extractor), std::move(callback));
}

std::u16string ChatContextObserver::GetPageTitle() const {
  return web_contents()->GetTitle();
}

// begin content::WebContentsObserver
void ChatContextObserver::WebContentsDestroyed() {
  inner_web_contents_ = nullptr;
}

void ChatContextObserver::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  if (!load_details.is_main_frame) {
    return;
  }
  // UniqueID will provide a consistent value for the entry when navigating
  // through history, allowing us to re-join conversations and navigations.
  int pending_navigation_id = load_details.entry->GetUniqueID();
  pending_navigation_id_ = pending_navigation_id;
  DVLOG(1) << __func__ << " id: " << pending_navigation_id_
           << "\n url: " << load_details.entry->GetVirtualURL()
           << "\n current page title: " << GetPageTitle()
           << "\n previous page title: " << previous_page_title_
           << "\n same document? " << load_details.is_same_document;

  // Allow same-document navigation, as content often changes as a result
  // of framgment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing before committing to starting a new conversation
  // and treating it as a "fresh page".
  is_same_document_navigation_ = load_details.is_same_document;
  // Experimentally only call |OnNewPage| for same-page navigations _if_
  // it results in a page title change (see |TtileWasSet|). Title detection
  // also done within the navigation entry so that back/forward navigations
  // are handled correctly.

  // Page loaded is only considered changing when full document changes
  if (!is_same_document_navigation_) {
    is_page_loaded_ = false;
  }
  if (!is_same_document_navigation_ || previous_page_title_ != GetPageTitle()) {
    // OnNewPage(pending_navigation_id_);
  }
  previous_page_title_ = GetPageTitle();
}

void ChatContextObserver::TitleWasSet(content::NavigationEntry* entry) {
  DVLOG(1) << __func__ << ": id=" << entry->GetUniqueID()
           << " title=" << entry->GetTitle();
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = GetPageTitle();
}

void ChatContextObserver::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  DVLOG(1) << __func__ << ": " << validated_url.spec();
  if (validated_url == GetPageURL()) {
    is_page_loaded_ = true;
    if (pending_extract_page_content_callback_) {
      GetPageContent(std::move(pending_extract_page_content_callback_));
    }
  }
}
// end content::WebContentsObserver

void ChatContextObserver::MaybeSameDocumentIsNewPage() {
  if (is_same_document_navigation_) {
    DVLOG(2) << "Same document navigation detected new \"page\" - calling "
                "OnNewPage()";
    // Cancel knowledge that the current navigation should be associated
    // with any conversation that's associated with the previous navigation.
    // Tell any conversation that it shouldn't be associated with this
    // content anymore, as we've moved on.
    // OnNewPage(pending_navigation_id_);
    // Don't respond to further TitleWasSet
    is_same_document_navigation_ = false;
  }
}

// mojom::PageContentExtractorHost
void ChatContextObserver::OnInterceptedPageContentChanged() {
  // Maybe mark that the page changed, if we didn't detect it already via title
  // change after a same-page navigation. This is the main benefit of this
  // function.
  MaybeSameDocumentIsNewPage();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ChatContextObserver);
}  // namespace ai_chat
