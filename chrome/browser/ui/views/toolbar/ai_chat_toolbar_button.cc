#include "ai_chat_toolbar_button.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view.h"

AIChatToolbarButton::AIChatToolbarButton(PressedCallback callback)
                                         : ToolbarButton(std::move(callback)) {
  SetTooltipText(l10n_util::GetStringUTF16(IDS_AI_CHAT_TITLE));
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetVectorIcon(kChatIcon);
  SetVisible(true);
  ConfigureInkDropForToolbar(this);
}

AIChatToolbarButton::~AIChatToolbarButton() = default;

void AIChatToolbarButton::AddHighlight() {
    anchor_higlight_ = AddAnchorHighlight();
}

void AIChatToolbarButton::ResetHighlight() {
    anchor_higlight_.reset();
}

BEGIN_METADATA(AIChatToolbarButton)
END_METADATA
