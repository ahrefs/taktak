#ifndef CHROMIUM_CHAT_CONTEXT_OBSERVER_H
#define CHROMIUM_CHAT_CONTEXT_OBSERVER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "chrome/common/chat/page_content_extractor.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

namespace ai_chat {
// Monitors changes in web content and updates the AI chat with the latest
// information.
class ChatContextObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ChatContextObserver>,
      public chat::mojom::PageContentExtractorHost {
 public:

    using ExtractPageContentCallback =
            base::OnceCallback<void(std::string content)>;

  static void BindPageContentExtractorHost(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<chat::mojom::PageContentExtractorHost>
          receiver);

  class PageContentExtractorHelperDelegate {
   public:
    virtual ~PageContentExtractorHelperDelegate() = default;
    virtual void ExtractPageContent(ExtractPageContentCallback callback) = 0;
  };

  ChatContextObserver(const ChatContextObserver&) = delete;

  ChatContextObserver& operator=(const ChatContextObserver&) = delete;

  ~ChatContextObserver() override;

  // chat::mojom::PageContentExtractorHost
  void OnInterceptedPageContentChanged() override;

  void GetPageContent(ExtractPageContentCallback callback);

 private:
  ChatContextObserver(content::WebContents* web_contents);

  friend class content::WebContentsUserData<ChatContextObserver>;

  // begin content::WebContentsObserver
  void WebContentsDestroyed() override;

  // Called when an event of significance occurs that, if the page is a
  // same-document navigation, should result in that previous navigation
  // being considered as a new page.
  void MaybeSameDocumentIsNewPage();

  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;

  void TitleWasSet(content::NavigationEntry* entry) override;

  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  // end content::WebContentsObserver

  void BindPageContentExtractorReceiver(
      mojo::PendingAssociatedReceiver<chat::mojom::PageContentExtractorHost>
          receiver);

  void SetPendingGetContentCallback(ExtractPageContentCallback callback);

  GURL GetPageURL() const;

  std::u16string GetPageTitle() const;

  void OnNewPage(int64_t navigation_id);

  int pending_navigation_id_;
  bool is_same_document_navigation_ = false;
  std::u16string previous_page_title_;
  bool is_page_loaded_ = false;
  raw_ptr<content::WebContents> inner_web_contents_ = nullptr;

  std::unique_ptr<PageContentExtractorHelperDelegate>
      page_content_extractor_helper_delegate_;
  ExtractPageContentCallback pending_extract_page_content_callback_;

  mojo::AssociatedReceiver<chat::mojom::PageContentExtractorHost>
      page_content_extractor_receiver_{this};

  base::WeakPtrFactory<ChatContextObserver> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace ai_chat

#endif  // CHROMIUM_CHAT_CONTEXT_OBSERVER_H
