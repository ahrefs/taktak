// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "taktak_run_handler.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/cs/cs_handler.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"

namespace {

constexpr char kFirstOpen[] = "first_open";
constexpr char kOpenInNext7Days[] = "open_in_next_7_days";
// constexpr char kOpenToday[] = "open_today";

constexpr base::FilePath::CharType kTaktakFirstRunFileName[] =
    FILE_PATH_LITERAL("taktak_first_run");

// Get the path of taktak_first_run file; returns false on failure.
bool GetTaktakFirstRunFilePath(base::FilePath* path) {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir)) {
    return false;
  }
  *path = user_data_dir.Append(kTaktakFirstRunFileName);
  return true;
}

// Returns true if the taktak_first_run file exists (or the path cannot be
// obtained).
bool IsTaktakFirstRunFilePresent() {
  base::FilePath taktak_first_run;
  return !GetTaktakFirstRunFilePath(&taktak_first_run) ||
         base::PathExists(taktak_first_run);
}

// Create the taktak_first_run file; return true if succeed, otherwise; false
bool CreateTaktakFirstRunFile() {
  base::FilePath taktak_first_run_filepath;
  if (!GetTaktakFirstRunFilePath(&taktak_first_run_filepath)) {
    return false;
  }

  if (base::PathExists(taktak_first_run_filepath)) {
    return false;
  }

  if (!base::WriteFile(taktak_first_run_filepath, "")) {
    return false;
  }

  return true;
}

// Reads the creation time of the taktak_first_run file.
// If the file does not exist, it will return base::Time().
base::Time ReadTaktakFirstRunFileCreationTime() {
  base::Time taktak_first_run_file_creation_time = base::Time();
  base::FilePath taktak_first_run_file_path;
  if (GetTaktakFirstRunFilePath(&taktak_first_run_file_path)) {
    base::File::Info info;
    if (base::GetFileInfo(taktak_first_run_file_path, &info)) {
      taktak_first_run_file_creation_time = info.creation_time;
    }
  }
  return taktak_first_run_file_creation_time;
}

}  // namespace

namespace taktak_run_handler {
void Handle(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  std::unique_ptr<cs_handler::CSHandler> cs_handler_ =
      std::make_unique<cs_handler::CSHandler>(url_loader_factory);

  if (IsTaktakFirstRunFilePresent()) {
    base::Time taktak_first_run_file_creation_time =
        ReadTaktakFirstRunFileCreationTime();
    base::Time seven_days_from_first_run_file_creation =
        taktak_first_run_file_creation_time + base::Days(7);
    base::Time now = base::Time::Now();
    if (now > taktak_first_run_file_creation_time &&
        now <= seven_days_from_first_run_file_creation) {
      // todo: to create second-time launch file to not track the event more than one time
      cs_handler_->HandleCustomEvent(kOpenInNext7Days);
      DVLOG(0) << "|>> Custom Event: Open in next 7 days";
    }
  } else if (CreateTaktakFirstRunFile()) {
    cs_handler_->HandleCustomEvent(kFirstOpen);
    DVLOG(0) << "|>> Custom Event: First open";
  }
}
}  // namespace taktak_run_handler
