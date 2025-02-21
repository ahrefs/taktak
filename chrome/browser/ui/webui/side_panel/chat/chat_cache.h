#ifndef CHROMIUM_CHAT_CACHE_H
#define CHROMIUM_CHAT_CACHE_H

#include <optional>
#include <string>
#include <unordered_map>
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"

class ChatCache {
public:
    ChatCache();
    ChatCache(const ChatCache&) = delete;
    ChatCache& operator=(const ChatCache&) = delete;
    ~ChatCache() ;

   chat::mojom::ChatStatePtr GetChatState();

   void SaveConversation(chat::mojom::SavableConversationModelPtr conversation);

   void SaveThinkingState(bool thinking_state);

   void ClearChatState();

private:
    std::unordered_map<std::string, chat::mojom::SavableConversationModelPtr> chat_cache_;
    bool enable_thinking;
};

#endif //CHROMIUM_CHAT_CACHE_H
