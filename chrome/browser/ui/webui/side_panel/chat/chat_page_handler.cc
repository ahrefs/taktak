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
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/web_ui.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

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
      profile_(Profile::FromWebUI(web_ui)),
      page_content_extractor_helper_(std::make_unique<PageContentExtractorHelper>(chat_context_web_contents))
      {
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

void ChatPageHandler::SetSiteInfo(chat::mojom::SiteInfoPtr site_info, content::WebContents* contents) {
    page_content_extractor_helper_ = std::make_unique<PageContentExtractorHelper>(contents);
    chat_context_web_contents_ = contents;
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
    const GURL gurl = chat_context_web_contents_->GetLastCommittedURL();
    if (gurl.SchemeIsHTTPOrHTTPS()) {
      site_info->title =
          base::UTF16ToUTF8(chat_context_web_contents_->GetTitle());
      site_info->url = gurl.spec();
      site_info->is_content_usable_in_conversations = true;
    } else {
      site_info->title = "";
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

base::WeakPtr<ChatPageHandler> ChatPageHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ChatPageHandler::SubmitAction(chat::mojom::ActionType action_type) {
    // todo: to put prompt in resource file
    std::string summarize_prompt = "Provide a brief summary of the key takeaways for the following:";
    std::string explain_prompt = "Explain the following in simple language:";
    std::string fact_check_prompt = "Fact check the following:";
    if (page_.is_bound()) {
      DVLOG(0) << action_type;

      if (action_type == chat::mojom::ActionType::SUMMARIZE_PAGE) {
          page_content_extractor_helper_->ExtractPageContent(
                  base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                                 base::Unretained(this), action_type, summarize_prompt));

      } else if (action_type == chat::mojom::ActionType::EXPLAIN) {
          page_content_extractor_helper_->ExtractPageContent(
                  base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                                 base::Unretained(this), action_type, explain_prompt));

      } else if (action_type == chat::mojom::ActionType::FACT_CHECK) {
          page_content_extractor_helper_->ExtractPageContent(
                  base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                                 base::Unretained(this), action_type, fact_check_prompt));
      }
      else {
        // todo: to implement for other action types later
        chat::mojom::ActionResponsePtr response =
            chat::mojom::ActionResponse::New();
        response->action_type = action_type;
        response->response_type = chat::mojom::ResponseType::DELTA;
        response->result = "MOCK Result";
        page_->OnSubmitActionResponse(response.Clone());
      }
    }
}

void ChatPageHandler::OnPageContentExtracted(
    chat::mojom::ActionType action_type,
    const std::string& prompt,
    std::string content) {
  std::string max_content = content;

  //Note: This max tokens seems message + completion.
  const size_t max_tokens = 32768;
  if (content.length() > max_tokens) {
    max_content = content.substr(0, max_tokens);
  }

  api_client_->QueryPrompt(
      prompt + max_content,
      base::BindOnce(&ChatPageHandler::SubmitQueryCompletedCallback,
                     base::Unretained(this), action_type),
      base::BindRepeating(&ChatPageHandler::SubmitQueryCallback,
                          base::Unretained(this), action_type));
}

void ChatPageHandler::SubmitQueryCallback(chat::mojom::ActionType action_type,
                                          std::string completion) {
  chat::mojom::ActionResponsePtr response = chat::mojom::ActionResponse::New();
  response->action_type = action_type;
  response->response_type = chat::mojom::ResponseType::DELTA;
  response->result = completion;
  page_->OnSubmitActionResponse(response.Clone());
}

void ChatPageHandler::SubmitQueryCompletedCallback(
    chat::mojom::ActionType action_type,
    base::expected<std::string, chat::mojom::APIErrorType> result) {
  chat::mojom::ActionResponsePtr response = chat::mojom::ActionResponse::New();
  response->action_type = action_type;
  if (result.has_value()) {
    DVLOG(0) << __func__ << " success -> " << result.value();
    response->response_type = chat::mojom::ResponseType::COMPLETED;
    response->result = result.value();
  } else {
    DVLOG(0) << __func__ << " error -> " << result.error();
    response->response_type = chat::mojom::ResponseType::ERROR;
    response->result =
        "error_message_here";  // todo: to get error message from api response
                               // and pass it to chat UI
  }
  page_->OnSubmitActionResponse(response.Clone());
}

void ChatPageHandler::SubmitQuery(chat::mojom::ActionType action_type, const std::string& query) {
  api_client_->QueryPrompt(
      query,
      base::BindOnce(&ChatPageHandler::SubmitQueryCompletedCallback,
                     base::Unretained(this), action_type),
      base::BindRepeating(&ChatPageHandler::SubmitQueryCallback,
                          base::Unretained(this), action_type));
}
