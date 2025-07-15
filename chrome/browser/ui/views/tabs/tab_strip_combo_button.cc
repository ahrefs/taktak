// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_combo_button.h"

#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/theme_provider.h"
#include "ui/color/color_id.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

/*
 * Note: This implementation maintains the default button styling and ignores
 * most feature flag configurations, with the exception of the reverse button
 * order feature param.
 * 
 * Known issue: The separator positioning between new_tab_button and 
 * search_tab_button is not properly centered and needs adjustment.
 * 
 * TODO: Exercise caution when merging changes from upstream to preserve
 * these customizations or take the upstream in if the separator positioning bug is fixed
 */
namespace {

// LINT.IfChange(AccidentalClickType)
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class AccidentalClickType {
  kClick = 0,
  kAccidentalClick = 1,
  kMaxValue = kAccidentalClick,
};
// LINT.ThenChange(//tools/metrics/histograms/metadata/tab/enums.xml:AccidentalClickType)

constexpr int kButtonGapNoBackground = 4;
constexpr base::TimeDelta kAccidentalClickThreshold = base::Seconds(1);
constexpr char kNewTabButtonAccidentalClickName[] =
    "Tabs.NewTabButton.AccidentalClicks";
constexpr char kTabSearchAccidentalClickName[] =
    "Tabs.TabSearch.AccidentalClicks";
}  // namespace

TabStripComboButton::TabStripComboButton(BrowserWindowInterface* browser,
                                         TabStrip* tab_strip) {
  Edge new_tab_button_flat_edge = Edge::kNone;
  std::unique_ptr<TabStripControlButton> new_tab_button =
      std::make_unique<TabStripControlButton>(
          tab_strip->controller(),
          base::BindRepeating(&TabStrip::NewTabButtonPressed,
                              base::Unretained(tab_strip)),
          vector_icons::kAddIcon, new_tab_button_flat_edge);
  new_tab_button->SetProperty(views::kElementIdentifierKey,
                              kNewTabButtonElementId);

  // Add a gap between the new tab button and tab search button.
  gfx::Insets button_margins =
      gfx::Insets::TLBR(0, kButtonGapNoBackground, 0, kButtonGapNoBackground);
  new_tab_button->SetProperty(views::kMarginsKey, button_margins);

  new_tab_button->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_TOOLTIP_NEW_TAB));
  new_tab_button->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB));
  subscriptions_.push_back(new_tab_button->AddStateChangedCallback(
      base::BindRepeating(&TabStripComboButton::OnNewTabButtonStateChanged,
                          base::Unretained(this))));

#if BUILDFLAG(IS_LINUX)
  // The New Tab Button can be middle-clicked on Linux.
  new_tab_button->SetTriggerableEventFlags(
      new_tab_button->GetTriggerableEventFlags() | ui::EF_MIDDLE_MOUSE_BUTTON);
#endif

  std::unique_ptr<views::Separator> separator =
      std::make_unique<views::Separator>();
  separator->SetBorderRadius(TabStyle::Get()->GetSeparatorCornerRadius());
  separator->SetPreferredSize(TabStyle::Get()->GetSeparatorSize());
  separator->SetVisible(true);
  subscriptions_.push_back(browser->RegisterDidBecomeActive(base::BindRepeating(
      &TabStripComboButton::DidBecomeActive, base::Unretained(this))));
  subscriptions_.push_back(
      browser->RegisterDidBecomeInactive(base::BindRepeating(
          &TabStripComboButton::DidBecomeInactive, base::Unretained(this))));

  Edge tab_search_button_flat_edge = Edge::kNone;
  std::unique_ptr<TabSearchButton> tab_search_button =
      std::make_unique<TabSearchButton>(tab_strip->controller(), browser,
                                        tab_search_button_flat_edge,
                                        Edge::kNone, tab_strip);
  tab_search_button->SetFlatEdgeFactor(1);
  tab_search_button->SetProperty(views::kCrossAxisAlignmentKey,
                                 views::LayoutAlignment::kCenter);
  tab_search_button->SetProperty(views::kMarginsKey, button_margins);
  subscriptions_.push_back(tab_search_button->AddStateChangedCallback(
      base::BindRepeating(&TabStripComboButton::OnTabSearchButtonStateChanged,
                          base::Unretained(this))));

  auto* button_container = AddChildView(std::make_unique<views::View>());
  button_container->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  if (features::HasTabstripComboButtonWithReverseButtonOrder()) {
    tab_search_button_ =
        button_container->AddChildView(std::move(tab_search_button));
    separator_ = button_container->AddChildView(std::move(separator));
    new_tab_button_ = button_container->AddChildView(std::move(new_tab_button));
  } else {
    new_tab_button_ = button_container->AddChildView(std::move(new_tab_button));
    separator_ = button_container->AddChildView(std::move(separator));
    tab_search_button_ =
        button_container->AddChildView(std::move(tab_search_button));
  }
  separator_->SetPreferredSize(gfx::Size(2, 18));
  separator_->SetVisible(true);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetNotifyEnterExitOnChild(true);
}

TabStripComboButton::~TabStripComboButton() = default;

void TabStripComboButton::OnNewTabButtonStateChanged() {
  if (new_tab_button_->GetState() == views::Button::STATE_PRESSED) {
    new_tab_button_last_pressed_ = base::TimeTicks::Now();
    base::UmaHistogramEnumeration(kNewTabButtonAccidentalClickName,
                                  AccidentalClickType::kClick);
    if (!tab_search_button_last_pressed_.is_null() &&
        (new_tab_button_last_pressed_ - tab_search_button_last_pressed_) <
            kAccidentalClickThreshold) {
      base::UmaHistogramEnumeration(kTabSearchAccidentalClickName,
                                    AccidentalClickType::kAccidentalClick);
    }
  }
  separator_->SetVisible(true);
}

void TabStripComboButton::OnTabSearchButtonStateChanged() {
  if (tab_search_button_->GetState() == views::Button::STATE_PRESSED) {
    tab_search_button_last_pressed_ = base::TimeTicks::Now();
    base::UmaHistogramEnumeration(kTabSearchAccidentalClickName,
                                  AccidentalClickType::kClick);
    if (!new_tab_button_last_pressed_.is_null() &&
        (tab_search_button_last_pressed_ - new_tab_button_last_pressed_) <
            kAccidentalClickThreshold) {
      base::UmaHistogramEnumeration(kNewTabButtonAccidentalClickName,
                                    AccidentalClickType::kAccidentalClick);
    }
  }

  separator_->SetVisible(true);
}

void TabStripComboButton::DidBecomeActive(BrowserWindowInterface* browser) {
  separator_->SetVisible(true);
}

void TabStripComboButton::DidBecomeInactive(BrowserWindowInterface* browser) {
  separator_->SetVisible(true);
  separator_->SetColorId(kColorTabDividerFrameActive);
}

void TabStripComboButton::OnThemeChanged() {
  views::View::OnThemeChanged();
  using_custom_theme_ = GetThemeProvider()->HasCustomImage(IDR_THEME_FRAME);

  ui::ColorId foreground_active_color;
  ui::ColorId foreground_inactive_color;
  ui::ColorId background_active_color;
  ui::ColorId background_inactive_color;
  if (using_custom_theme_ /*|| features::HasTabstripComboButtonWithBackground()*/) {
    foreground_active_color = kColorNewTabButtonForegroundFrameActive;
    foreground_inactive_color = kColorNewTabButtonForegroundFrameInactive;
    background_active_color = kColorNewTabButtonCRBackgroundFrameActive;
    background_inactive_color = kColorNewTabButtonCRBackgroundFrameInactive;
  } else {
    foreground_active_color = kColorTabForegroundInactiveFrameActive;
    foreground_inactive_color = kColorNewTabButtonCRForegroundFrameInactive;
    background_active_color = kColorNewTabButtonBackgroundFrameActive;
    background_inactive_color = kColorNewTabButtonBackgroundFrameInactive;
  }
  new_tab_button_->SetForegroundFrameActiveColorId(foreground_active_color);
  new_tab_button_->SetForegroundFrameInactiveColorId(foreground_inactive_color);
  new_tab_button_->SetBackgroundFrameActiveColorId(background_active_color);
  new_tab_button_->SetBackgroundFrameInactiveColorId(background_inactive_color);
  tab_search_button_->SetForegroundFrameActiveColorId(foreground_active_color);
  tab_search_button_->SetForegroundFrameInactiveColorId(
      foreground_inactive_color);
  tab_search_button_->SetBackgroundFrameActiveColorId(background_active_color);
  tab_search_button_->SetBackgroundFrameInactiveColorId(
      background_inactive_color);

  separator_->SetVisible(true);
}

void TabStripComboButton::UpdateSeparatorVisibility() {
  separator_->SetVisible(true);
}

BEGIN_METADATA(TabStripComboButton)
END_METADATA