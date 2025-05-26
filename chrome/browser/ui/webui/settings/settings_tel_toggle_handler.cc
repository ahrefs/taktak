#include "settings_tel_toggle_handler.h"

#include "components/prefs/pref_service.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "content/public/browser/web_ui.h"
#include "components/browsing_data/core/pref_names.h"

TelToggleHandler::TelToggleHandler(raw_ptr<PrefService> prefs)
    : prefs_(prefs) {}

TelToggleHandler::~TelToggleHandler() = default;

void TelToggleHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "setTelToggle",
      base::BindRepeating(&TelToggleHandler::HandleSetToggle,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getTelToggle",
      base::BindRepeating(&TelToggleHandler::HandleGetToggle,
                          base::Unretained(this)));
}

void TelToggleHandler::OnJavascriptAllowed() {}
void TelToggleHandler::OnJavascriptDisallowed() {}

void TelToggleHandler::HandleSetToggle(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1u);
  bool value = args[0].GetBool();
  prefs_->SetBoolean(browsing_data::prefs::kTaktakTelEnabled, value);
}


void TelToggleHandler::HandleGetToggle(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1u, args.size());
  const base::Value& callback_id = args[0];
  bool value = prefs_->GetBoolean(browsing_data::prefs::kTaktakTelEnabled);
  base::Value::List result;
  result.Append(value);
  ResolveJavascriptCallback(callback_id, result);
}
