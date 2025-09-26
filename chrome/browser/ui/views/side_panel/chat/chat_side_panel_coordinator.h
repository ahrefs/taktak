// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H
#define CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"

class Profile;
class SidePanelRegistry;
class TabStripModel;

// ChatSidePanelCoordinator handles the creation and registration of the
// Chat SidePanelEntry.
class ChatSidePanelCoordinator {
public:
    ChatSidePanelCoordinator(Profile* profile, TabStripModel* tab_strip_model);
    ChatSidePanelCoordinator(const ChatSidePanelCoordinator&) = delete;
    ChatSidePanelCoordinator& operator=(const ChatSidePanelCoordinator&) = delete;
    ~ChatSidePanelCoordinator();

    void CreateAndRegisterEntry(SidePanelRegistry* global_registry);
    void UpdateOpeningPanelId(SidePanelEntryId panel_id);
    void UpdateClosingPanelId(SidePanelEntryId panel_id);

private:
    const raw_ref<Profile> profile_;
    const raw_ref<TabStripModel> tab_strip_model_;
};

#endif  // CHROMIUM_CHAT_SIDE_PANEL_COORDINATOR_H
