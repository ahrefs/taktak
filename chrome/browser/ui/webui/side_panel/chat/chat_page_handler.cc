#include "chat_page_handler.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

ChatPageHandler::ChatPageHandler(
    mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
    mojo::PendingRemote<chat::mojom::Page> page,
    ChatUI* chat_ui,
    content::WebUI* web_ui)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      chat_ui_(chat_ui),
      web_ui_(web_ui),
      web_contents_(web_ui->GetWebContents()) {}

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
  auto title = base::UTF16ToUTF8(web_contents_->GetTitle());
  std::string url;
  const GURL gurl = web_contents_->GetLastCommittedURL();
  if (gurl.SchemeIsHTTPOrHTTPS()) {
    url = gurl.spec();
  }
  chat::mojom::SiteInfoPtr site_info = chat::mojom::SiteInfo::New();
  site_info->title = title;
  site_info->url = url;

  // todo: to check the schema of the current tab
  site_info->is_content_usable_in_conversations = true;

  // todo: to check the content of the current tab
  site_info->is_content_modified = false;

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

void ChatPageHandler::SubmitAction(chat::mojom::ActionType action_type) {
    if (page_.is_bound()) {
        LOG(INFO) << action_type;
        chat::mojom::ActionResponsePtr response = chat::mojom::ActionResponse::New();
        response->action_type = action_type;
        response->result = "MOCK Result";
        page_->OnSubmitActionResponse(response.Clone());
    }
}


void ChatPageHandler::SubmitQuery(chat::mojom::ActionType action_type, const std::string& query) {

}
