#ifndef CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_TEL_TOGGLE_HANDLER_H_
#define CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_TEL_TOGGLE_HANDLER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class PrefService;

class TelToggleHandler : public settings::SettingsPageUIHandler {
 public:
  explicit TelToggleHandler(raw_ptr<PrefService> prefs);
  ~TelToggleHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  base::Value::List GetTelToggle();

 private:
  void HandleSetToggle(const base::Value::List& args);
  void HandleGetToggle(const base::Value::List& args);

  raw_ptr<PrefService> prefs_;
};

#endif //CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_TEL_TOGGLE_HANDLER_H_
