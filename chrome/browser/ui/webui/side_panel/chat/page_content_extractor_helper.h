#ifndef CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H
#define CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H

#include <string>

#include "base/functional/callback_forward.h"
#include "chat_context_observer.h"
#include "chrome/renderer/chat/page_content_extractor.h"
#include "chat_context_observer.h"

namespace ai_chat {
class PageContentExtractorHelper
: public ChatContextObserver::PageContentExtractorHelperDelegate {
 public:

  explicit PageContentExtractorHelper(content::WebContents* web_contents);
  ~PageContentExtractorHelper() override;
  PageContentExtractorHelper(const PageContentExtractorHelper&) = delete;
  PageContentExtractorHelper& operator=(const PageContentExtractorHelper&) =
      delete;
  void ExtractPageContent(ChatContextObserver::ExtractPageContentCallback callback) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
};
}  // namespace ai_chat

#endif  // CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H
