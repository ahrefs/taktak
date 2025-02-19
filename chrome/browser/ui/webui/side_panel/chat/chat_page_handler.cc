#include "chat_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/base/window_open_disposition.h"
#include "ui/base/window_open_disposition_utils.h"

namespace {

    constexpr size_t
    kMaxUserPromptLength = 90'000;
    constexpr char kUserRole[] = "user";
    constexpr char kAssistantRole[] = "assistant";

    std::string BuildPrompt(const std::string &query,
                            const std::string &extracted_content,
                            chat::mojom::ActionType action_type) {
        if (action_type == chat::mojom::ActionType::SUMMARIZE_PAGE ||
            action_type == chat::mojom::ActionType::EXPLAIN ||
            action_type == chat::mojom::ActionType::FACT_CHECK ||
            action_type == chat::mojom::ActionType::TRANSLATE ||
            action_type == chat::mojom::ActionType::DRAFT_SOCIAL_MEDIA_POST) {
            return query + ": " + extracted_content;
        } else {
            std::string context_prompt =
                    l10n_util::GetStringUTF8(IDS_CHAT_CONTEXT_PROMPT);
            return base::ReplaceStringPlaceholders(context_prompt,
                                                   {query, extracted_content}, nullptr);
        }
    }
}  // namespace

ChatPageHandler::ChatPageHandler(
        mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
        mojo::PendingRemote<chat::mojom::Page> page,
        ChatUI *chat_ui,
        content::WebUI *web_ui,
        content::WebContents *owner_web_contents,
        content::WebContents *chat_context_web_contents)
        : receiver_(this, std::move(receiver)),
          page_(std::move(page)),
          chat_ui_(chat_ui),
          owner_web_contents_(owner_web_contents->GetWeakPtr()),
          chat_context_web_contents_(chat_context_web_contents->GetWeakPtr()),
          profile_(Profile::FromWebUI(web_ui)),
          page_content_extractor_helper_(std::make_unique<PageContentExtractorHelper>(chat_context_web_contents)) {
    isQueryCancellingInProgress_.store(false);
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
    if (embedder) {
        embedder->CloseUI();
    }

    Browser *browser = chrome::FindLastActive();
    if (!browser) {
        return;
    }

    if (SidePanelUI *ui = browser->GetFeatures().side_panel_ui()) {
        ui->Close();
    }
}

void ChatPageHandler::SetSiteInfo(chat::mojom::SiteInfoPtr site_info, content::WebContents *contents) {
    page_content_extractor_helper_ = std::make_unique<PageContentExtractorHelper>(contents);
    chat_context_web_contents_ = contents->GetWeakPtr();
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
            l10n_util::GetStringUTF8(IDS_CHAT_DRAFT_FACT_CHECK));

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

void ChatPageHandler::SubmitAction(chat::mojom::ActionType action_type,
                                   const std::string &action_param,
                                   bool enable_thinking) {
    isQueryCancellingInProgress_.store(false);
    if (page_.is_bound()) {
        std::string summarize_prompt =
                l10n_util::GetStringUTF8(IDS_CHAT_PROMPT_SUMMARIZE_THIS_PAGE);
        std::string explain_prompt = l10n_util::GetStringUTF8(
                IDS_CHAT_PROMPT_EXPLAIN_IT_IN_SIMPLE_LANGUAGE);
        std::string fact_check_prompt =
                l10n_util::GetStringUTF8(IDS_CHAT_PROMPT_DRAFT_FACT_CHECK);
        std::string translate_to_prompt =
                l10n_util::GetStringUTF8(IDS_CHAT_PROMPT_TRANSLATE);
        std::string draft_social_media_post_prompt =
                l10n_util::GetStringUTF8(IDS_CHAT_PROMPT_DRAFT_A_SOCIAL_MEDIA_POST);

        std::vector<struct CompletionMessage> completion_messages = {};

        if (action_type == chat::mojom::ActionType::SUMMARIZE_PAGE) {
            page_content_extractor_helper_->ExtractPageContent(base::BindOnce(
                    &ChatPageHandler::OnPageContentExtracted, base::Unretained(this),
                    action_type, summarize_prompt, completion_messages, enable_thinking));

        } else if (action_type == chat::mojom::ActionType::EXPLAIN) {
            page_content_extractor_helper_->ExtractPageContent(
                    base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                                   base::Unretained(this), action_type,
                                   explain_prompt, completion_messages, enable_thinking));

        } else if (action_type == chat::mojom::ActionType::FACT_CHECK) {
            page_content_extractor_helper_->ExtractPageContent(
                    base::BindOnce(&ChatPageHandler::OnPageContentExtracted,
                                   base::Unretained(this), action_type,
                                   fact_check_prompt, completion_messages, enable_thinking));
        } else if (action_type == chat::mojom::ActionType::TRANSLATE) {
            page_content_extractor_helper_->ExtractPageContent(base::BindOnce(
                    &ChatPageHandler::OnPageContentExtracted,
                    base::Unretained(this), action_type,
                    translate_to_prompt + " " + action_param, completion_messages, enable_thinking));
        } else if (action_type ==
                   chat::mojom::ActionType::DRAFT_SOCIAL_MEDIA_POST) {
            page_content_extractor_helper_->ExtractPageContent(base::BindOnce(
                    &ChatPageHandler::OnPageContentExtracted,
                    base::Unretained(this), action_type,
                    draft_social_media_post_prompt + " " + action_param,
                    completion_messages, enable_thinking));
        } else {
            // this branch should not be reached because all the action items are
            // handled in above blocks
        }
    }
}

void ChatPageHandler::OnPageContentExtracted(
        chat::mojom::ActionType action_type,
        const std::string &prompt,
        const std::vector<struct CompletionMessage> &completion_messages,
        bool enable_thinking,
        std::string content,
        std::string url) {

    DVLOG(0) << __func__ << " extracted content -> " << content;

    extracted_content_cache_.clear();

    std::string max_content = content;
    if (content.length() > kMaxUserPromptLength) {
        max_content = content.substr(0, kMaxUserPromptLength);
    }

    if (!url.empty()) {
        extracted_content_cache_[url] = max_content;
    }

    if (isQueryCancellingInProgress_.load()) {
        isQueryCancellingInProgress_.store(false);
        return;
    }

    std::vector<struct CompletionMessage> all_messages;
    for (auto &msg: completion_messages) {
        all_messages.push_back(msg);
    }
    all_messages.push_back(
            {BuildPrompt(prompt, max_content, action_type), kUserRole});

    api_client_->QueryPrompt(
            all_messages,
            enable_thinking,
            base::BindOnce(&ChatPageHandler::SubmitQueryCompletedCallback,
                           base::Unretained(this), action_type),
            base::BindRepeating(&ChatPageHandler::SubmitQueryCallback,
                                base::Unretained(this), action_type));
}

void ChatPageHandler::SubmitQuery(chat::mojom::ActionType action_type,
                                  const std::string &query,
                                  const std::string &url,
                                  std::vector<chat::mojom::ConversationItemPtr> conversation_history,
                                  bool enable_thinking) {

    std::vector<struct CompletionMessage> completion_messages;

    for (auto &item: conversation_history) {
        completion_messages.push_back({item->user_query, kUserRole});
        completion_messages.push_back({item->llm_response, kAssistantRole});
    }

    if (extracted_content_cache_.contains(url) /* Context is in the cache */) {
        auto previous_content = extracted_content_cache_[url];
        completion_messages.push_back(
                {BuildPrompt(query, previous_content, action_type), kUserRole});
        api_client_->QueryPrompt(
                completion_messages,
                enable_thinking,
                base::BindOnce(&ChatPageHandler::SubmitQueryCompletedCallback,
                               base::Unretained(this), action_type),
                base::BindRepeating(&ChatPageHandler::SubmitQueryCallback,
                                    base::Unretained(this), action_type));

    } else if (!url.empty()/* Context is not in the cache; user visit new page so new content should be extracted */) {
        page_content_extractor_helper_->ExtractPageContent(base::BindOnce(
                &ChatPageHandler::OnPageContentExtracted, base::Unretained(this),
                action_type, query, completion_messages, enable_thinking));
    } else /* user removed the context via Chat UI or the current opening tab is
              empty */
    {
        completion_messages.push_back({query, kUserRole});
        api_client_->QueryPrompt(
                completion_messages,
                enable_thinking,
                base::BindOnce(&ChatPageHandler::SubmitQueryCompletedCallback,
                               base::Unretained(this), action_type),
                base::BindRepeating(&ChatPageHandler::SubmitQueryCallback,
                                    base::Unretained(this), action_type));
    }
}

void ChatPageHandler::SubmitQueryCallback(chat::mojom::ActionType action_type,
                                          std::string completion) {
    chat::mojom::ActionResponsePtr response = chat::mojom::ActionResponse::New();
    response->action_type = action_type;
    response->response_type = chat::mojom::ResponseType::DELTA;
    response->result = std::move(completion);
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
        response->result = l10n_util::GetStringUTF8(IDS_CHAT_GENERIC_ERROR);
    }
    page_->OnSubmitActionResponse(response.Clone());
}

void ChatPageHandler::CancelQuery() {
    isQueryCancellingInProgress_.store(true);
    api_client_->ClearAllQueries();
}

void ChatPageHandler::OpenURL(
        const std::string &url,
        ui::mojom::ClickModifiersPtr click_modifiers) {
    Browser *browser = chrome::FindLastActive();
    if (!browser) {
        return;
    }

    // Open in active tab if the user is on the NTP.
    WindowOpenDisposition open_location = ui::DispositionFromClick(
            click_modifiers->middle_button, click_modifiers->alt_key,
            click_modifiers->ctrl_key, click_modifiers->meta_key,
            click_modifiers->shift_key);

    GURL gurl(url);

    if (gurl.is_valid()) {
        content::OpenURLParams params(gurl, content::Referrer(), open_location,
                                      ui::PAGE_TRANSITION_AUTO_BOOKMARK, false);
        browser->OpenURL(params, /*navigation_handle_callback=*/{});
    }
}