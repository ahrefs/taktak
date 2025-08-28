// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CHAT_PAGE_HANDLER_H
#define CHROMIUM_CHAT_PAGE_HANDLER_H

#include <atomic>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/types/expected.h"
#include "chat_ui.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/side_panel/chat/api/completion_api_client.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "chrome/common/page_content_extractor/page_content_extractor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "page_content_extractor_helper.h"

namespace content {
    class WebContents;

    class WebUI;
}  // namespace content

class ChatPageHandler : public chat::mojom::PageHandler {
 public:
  ChatPageHandler(mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
                  mojo::PendingRemote<chat::mojom::Page> page,
                  ChatUI* chat_ui,
                  content::WebUI* web_ui,
                  content::WebContents* owner_web_contents,
                  content::WebContents* chat_context_web_contents);

  ChatPageHandler(const ChatPageHandler&) = delete;

  ChatPageHandler& operator=(const ChatPageHandler&) = delete;

  ~ChatPageHandler() override;

  void GetSiteInfo(GetSiteInfoCallback callback) override;

  void GetSiteInfoFromCache(GetSiteInfoFromCacheCallback callback) override;

  void GetActionList(GetActionListCallback callback) override;

  void GetChatState(GetChatStateCallback callback) override;

  void SaveConversation(chat::mojom::SavableConversationModelPtr conversation) override;

  void SaveSiteInfo(chat::mojom::SiteInfoPtr site_info) override;

  void SaveThinkingState(bool thinking_state) override;

  void GetThinkingState(GetThinkingStateCallback callback) override;

  void ClearChatState() override;

  void SubmitAction(chat::mojom::ActionType action_type,
                    const std::string& action_param, bool enable_thinking) override;

  void SubmitQuery(chat::mojom::ActionType action_type,
                   const std::string& query,
                   const std::string& url,
                   std::vector<chat::mojom::ConversationItemPtr> conversation_history, bool enable_thinking) override;

  void ShowUI() override;

  void CloseUI() override;

  void SetSiteInfo(chat::mojom::SiteInfoPtr site_info,
                   content::WebContents* contents);

  void SubmitQueryCallback(chat::mojom::ActionType action_type,
                           std::string completion);

  void SubmitQueryCompletedCallback(
      chat::mojom::ActionType action_type,
      base::expected<std::string, chat::mojom::APIErrorType> result);

  void CancelQuery() override;

  void OpenURL(const std::string& url, ui::mojom::ClickModifiersPtr click_modifiers) override;

  base::WeakPtr<ChatPageHandler> GetWeakPtr();

 private:
  void OnPageContentExtracted(
      chat::mojom::ActionType action_type,
      const std::string& prompt,
      const std::vector<struct CompletionMessage>& completion_messages,
      bool enable_thinking,
      std::string content,
      std::string url);

  mojo::Receiver<chat::mojom::PageHandler> receiver_;
  mojo::Remote<chat::mojom::Page> page_;
  const raw_ptr<ChatUI> chat_ui_ = nullptr;
  base::WeakPtr<content::WebContents> owner_web_contents_ = nullptr;
  base::WeakPtr<content::WebContents> chat_context_web_contents_ = nullptr;
  const raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<CompletionApiClient> api_client_ = nullptr;
  std::unique_ptr<PageContentExtractorHelper> page_content_extractor_helper_ =
      nullptr;
  std::atomic<bool> isQueryCancellingInProgress_;
  base::flat_map<std::string, std::string> extracted_content_cache_;
  base::WeakPtrFactory<ChatPageHandler> weak_ptr_factory_{this};
};

#endif //CHROMIUM_CHAT_PAGE_HANDLER_H
