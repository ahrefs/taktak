// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_AI_CHAT_TOOLBAR_BUTTON_H
#define CHROMIUM_AI_CHAT_TOOLBAR_BUTTON_H

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;

class AIChatToolbarButton : public ToolbarButton {
    METADATA_HEADER(AIChatToolbarButton, ToolbarButton)

public:
    AIChatToolbarButton(PressedCallback callback);

    AIChatToolbarButton(const AIChatToolbarButton &) = delete;

    AIChatToolbarButton &operator=(const AIChatToolbarButton &) = delete;

    ~AIChatToolbarButton() override;

    void AddHighlight();

    void ResetHighlight();

protected:
    SkColor GetForegroundColor(ButtonState state) const override;

private:
    std::optional<Button::ScopedAnchorHighlight> anchor_higlight_;
};

#endif //CHROMIUM_AI_CHAT_TOOLBAR_BUTTON_H