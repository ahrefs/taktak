#include "chrome/browser/ui/webui/side_panel/chat/chat_cache.h"

ChatCache::ChatCache() : enable_thinking(false) {}

ChatCache::~ChatCache() = default;

chat::mojom::ChatStatePtr ChatCache::GetChatState() {
    auto chat_state = chat::mojom::ChatState::New();
    chat_state->conversations = std::vector<chat::mojom::SavableConversationModelPtr>();

    for (const auto &entry: chat_cache_) {
        chat_state->conversations.push_back(entry.second.Clone());
    }

    chat_state->enable_thinking = enable_thinking;

    return chat_state;
}

void ChatCache::SaveConversation(chat::mojom::SavableConversationModelPtr conversation) {
    if (!conversation || conversation->id.empty()) {
        return;
    }

    chat_cache_[conversation->id] = std::move(conversation);
}

void ChatCache::SaveThinkingState(bool thinking_state) {
    enable_thinking = thinking_state;
}

void ChatCache::ClearChatState() {
    chat_cache_.clear();
    enable_thinking = false;
}