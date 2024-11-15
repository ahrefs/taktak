#include "chat_page_handler.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/webui/side_panel/chat/api/completion_api_client.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/renderer/chat/page_content_extractor.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {
constexpr auto kVideoPageContentTypes =
    base::MakeFixedFlatSet<chat::mojom::PageContentType>(
        {chat::mojom::PageContentType::VideoTranscriptYouTube,
         chat::mojom::PageContentType::VideoTranscriptVTT});

using ExtractPageContentCallback =
    base::OnceCallback<void(std::string page_content)>;

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

ChatPageHandler::ChatPageHandler(
    mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
    mojo::PendingRemote<chat::mojom::Page> page,
    ChatUI* chat_ui,
    content::WebUI* web_ui,
    content::WebContents* owner_web_contents,
    content::WebContents* chat_context_web_contents)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      chat_ui_(chat_ui),
      owner_web_contents_(owner_web_contents),
      chat_context_web_contents_(chat_context_web_contents),
      profile_(Profile::FromWebUI(web_ui)) {
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess();
  api_client_ =
      std::make_unique<CompletionApiClient>(std::move(url_loader_factory));
}

ChatPageHandler::~ChatPageHandler() = default;

void ChatPageHandler::ShowUI() {
    auto embedder = chat_ui_->embedder();
    if (embedder) {
        embedder->ShowUI();
    }
}

void ChatPageHandler::CloseUI() {
    auto embedder = chat_ui_->embedder();
    if (embedder)
        embedder->CloseUI();
}

void ChatPageHandler::SetSiteInfo(chat::mojom::SiteInfoPtr site_info) {
    if (page_.is_bound()) {
        page_->OnSiteInfoChanged(std::move(site_info));
    }
}

void ChatPageHandler::GetSiteInfo(GetSiteInfoCallback callback) {
  DCHECK(chat_context_web_contents_);
  chat::mojom::SiteInfoPtr site_info = chat::mojom::SiteInfo::New();
  site_info->url = "";
  site_info->is_content_usable_in_conversations = false;

  if (chat_context_web_contents_) {
    site_info->title =
        base::UTF16ToUTF8(chat_context_web_contents_->GetTitle());
    const GURL gurl = chat_context_web_contents_->GetLastCommittedURL();
    if (gurl.SchemeIsHTTPOrHTTPS()) {
      site_info->url = gurl.spec();
      site_info->is_content_usable_in_conversations = true;
    } else {
      site_info->url = "";
      site_info->is_content_usable_in_conversations = false;
    }
  }

    std::move(callback).Run(site_info.Clone());
}

void ChatPageHandler::GetActionList(GetActionListCallback callback) {
    std::vector<chat::mojom::ActionItemPtr> action_items;

    chat::mojom::ActionItemPtr summarize_item = chat::mojom::ActionItem::New(
            chat::mojom::ActionType::SUMMARIZE_PAGE,
            l10n_util::GetStringUTF8(IDS_CHAT_SUMMARIZE_THIS_PAGE));

    chat::mojom::ActionItemPtr explain_item = chat::mojom::ActionItem::New(
            chat::mojom::ActionType::EXPLAIN,
            l10n_util::GetStringUTF8(IDS_CHAT_EXPLAIN_IT_IN_SIMPLE_LANGUAGE));

    chat::mojom::ActionItemPtr translate_item = chat::mojom::ActionItem::New(
            chat::mojom::ActionType::TRANSLATE,
            l10n_util::GetStringUTF8(IDS_CHAT_TRANSLATE));

    chat::mojom::ActionItemPtr draft_social_media_post_item =
            chat::mojom::ActionItem::New(
                    chat::mojom::ActionType::DRAFT_SOCIAL_MEDIA_POST,
                    l10n_util::GetStringUTF8(IDS_CHAT_DRAFT_A_SOCIAL_MEDIA_POST));

    chat::mojom::ActionItemPtr fact_check_item = chat::mojom::ActionItem::New(
            chat::mojom::ActionType::FACT_CHECK,
            l10n_util::GetStringUTF8(IDS_CHAT_DRAFT_FACT_CHECT));

    action_items.push_back(summarize_item.Clone());
    action_items.push_back(explain_item.Clone());
    action_items.push_back(translate_item.Clone());
    action_items.push_back(draft_social_media_post_item.Clone());
    action_items.push_back(fact_check_item.Clone());

    std::move(callback).Run(std::move(action_items));
}

void ChatPageHandler::SubmitQueryCallback(std::string completion) {
    LOG(INFO) << completion;
}

void ChatPageHandler::SubmitQueryCompletedCallback(
        base::expected<std::string, chat::mojom::APIError> result) {
    LOG(INFO) << result.has_value();
}

base::WeakPtr<ChatPageHandler> ChatPageHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ChatPageHandler::SubmitAction(chat::mojom::ActionType action_type) {
    if (page_.is_bound()) {
        LOG(INFO) << action_type;

        if (action_type == chat::mojom::ActionType::SUMMARIZE_PAGE) {
          LOG(INFO) << "ActionType: Summarize_page";
          LOG(INFO) << "****"
                    << base::UTF16ToUTF8(
                           chat_context_web_contents_->GetTitle());
          const GURL gurl = chat_context_web_contents_->GetLastCommittedURL();
          LOG(INFO) << "****" << gurl.spec();

          auto* primary_rfh = chat_context_web_contents_->GetPrimaryMainFrame();
          DCHECK(primary_rfh->IsRenderFrameLive());

          mojo::Remote<chat::mojom::PageContentExtractor> extractor;
          primary_rfh->GetRemoteInterfaces()->GetInterface(
              extractor.BindNewPipeAndPassReceiver());

          auto* extractor_helper = new PageContentExtractorHelper();
          extractor_helper->Start(
              std::move(extractor),
              base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                             base::Unretained(this)));

        } else {
          // todo: to implement for other action types later
          chat::mojom::ActionResponsePtr response =
              chat::mojom::ActionResponse::New();
          response->action_type = action_type;
          response->result = "MOCK Result";
          page_->OnSubmitActionResponse(response.Clone());
        }
    }
}

void ChatPageHandler::OnPageContentExtracted(std::string content) {
  // todo: to put prompt in resource file
  api_client_->QueryPrompt(
      "Provide a brief summary of the key takeaways for the following:" +
          content,
      base::NullCallback(), base::NullCallback());
}

void ChatPageHandler::SubmitQuery(chat::mojom::ActionType action_type, const std::string& query) {
  // todo: use proper callback
  api_client_->QueryPrompt(query, base::NullCallback(), base::NullCallback());
}
