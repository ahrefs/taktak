#ifndef CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H
#define CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H

#include "chrome/browser/ui/browser_user_data.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"

class Browser;
class SidePanelRegistry;

namespace views {
class View;
}  // namespace views

class ChatSidePanelCoordinator
    : public BrowserUserData<ChatSidePanelCoordinator> {
 public:
  explicit ChatSidePanelCoordinator(Browser* browser);
  ~ChatSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);
  void UpdateOpeningPanelId(SidePanelEntryId panel_id);
  void UpdateClosingPanelId(SidePanelEntryId panel_id);

 private:
  friend class BrowserUserData<ChatSidePanelCoordinator>;
  std::unique_ptr<views::View> CreateChatWebView();

  BROWSER_USER_DATA_KEY_DECL();
};

#endif  // CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H
