// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "taktak_run_tracker.h"

#include <string>

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
constexpr char kOpenToday[] = "open_today";

enum FileType {
  TAKTAK_FIRST_RUN_FILE,
  TAKTAK_OPEN_IN_SEVEN_DAY_FILE,
  TAKTAK_OPEN_TODAY_FILE,
};

constexpr base::FilePath::CharType kTaktakFirstRunFileName[] =
    FILE_PATH_LITERAL("taktak_first_run");
constexpr base::FilePath::CharType kTaktakOpenInSevenDayFileName[] =
    FILE_PATH_LITERAL("taktak_open_in_seven_day");
constexpr base::FilePath::CharType kTaktakOpenTodayFileName[] =
    FILE_PATH_LITERAL("taktak_open_today");

// Get the path of the file; returns false on failure.
bool GetFilePath(FileType file_type, base::FilePath* path) {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir)) {
    return false;
  }

  switch (file_type) {
    case TAKTAK_FIRST_RUN_FILE:
      *path = user_data_dir.Append(kTaktakFirstRunFileName);
      return true;
    case TAKTAK_OPEN_IN_SEVEN_DAY_FILE:
      *path = user_data_dir.Append(kTaktakOpenInSevenDayFileName);
      return true;
    case TAKTAK_OPEN_TODAY_FILE:
      *path = user_data_dir.Append(kTaktakOpenTodayFileName);
      return true;
    default:
      return false;
  }
}

// Returns true if the taktak_first_run file exists (or the path cannot be
// obtained).
bool IsFilePresent(FileType file_type) {
  base::FilePath file_path;
  return !GetFilePath(file_type, &file_path) || base::PathExists(file_path);
}

// Create file; return true if succeed, otherwise; false
bool CreateFile(FileType file_type, std::string data = "") {
  base::FilePath file_path;
  if (!GetFilePath(file_type, &file_path)) {
    return false;
  }

  if (base::PathExists(file_path)) {
    return false;
  }

  if (!base::WriteFile(file_path, data)) {
    return false;
  }

  return true;
}

bool ReadFile(FileType file_type, std::string* content) {
  base::FilePath file_path;
  if (!GetFilePath(file_type, &file_path) || !base::PathExists(file_path)) {
    return false;
  }

  return base::ReadFileToString(file_path, content);
}

// Reads the creation time of the taktak_run file.
// If the file does not exist, it will return base::Time().
base::Time ReadFileCreationTime(FileType file_type) {
  base::Time taktak_run_file_creation_time = base::Time();
  base::FilePath taktak_run_file_path;
  if (GetFilePath(file_type, &taktak_run_file_path)) {
    base::File::Info info;
    if (base::GetFileInfo(taktak_run_file_path, &info)) {
      taktak_run_file_creation_time = info.creation_time;
    }
  }
  return taktak_run_file_creation_time;
}

}  // namespace

namespace taktak_run_tracker {

void Handle(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  std::unique_ptr<cs_handler::CSHandler> cs_handler_ =
      std::make_unique<cs_handler::CSHandler>(url_loader_factory);

  // Get current local date in "YYYY-MM-DD" format
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded_now;
  now.LocalExplode(&exploded_now);

  std::string today_date = base::NumberToString(exploded_now.year) + "-" +
                           base::NumberToString(exploded_now.month) + "-" +
                           base::NumberToString(exploded_now.day_of_month);

  std::string stored_date;
  // The following if-else is to trigger the 'open-today' event for today if not yet.
  if (ReadFile(FileType::TAKTAK_OPEN_TODAY_FILE, &stored_date)) {
    if (stored_date != today_date) {
      cs_handler_->HandleCustomEvent(kOpenToday);
      DVLOG(0) << "|>> Custom Event: Open today";
      if (CreateFile(FileType::TAKTAK_OPEN_TODAY_FILE, today_date)) {
        DVLOG(0) << "|>> Write today date to open_today file.";
      } else {
        DVLOG(0) << "|>> Error Writing today date to open_today file.";
      }
    }
  } else {
    cs_handler_->HandleCustomEvent(kOpenToday);
    DVLOG(0) << "|>> Custom Event: Open today";
    if (CreateFile(FileType::TAKTAK_OPEN_TODAY_FILE, today_date)) {
      DVLOG(0) << "|>> Write today date to open_today file.";
    } else {
      DVLOG(0) << "|>> Error Writing today date to open_today file.";
    }
  }

  // Checks if the TAKTAK_FIRST_RUN_FILE exists, indicating a prior application
  // run. If present, retrieves the file's creation time and calculates a
  // seven-day window from it. Compares the current time to determine if it
  // falls within this window. If within seven days and
  // TAKTAK_OPEN_IN_SEVEN_DAY_FILE does not exist, triggers a custom event
  // (kOpenInNext7Days). And Attempts to create TAKTAK_OPEN_IN_SEVEN_DAY_FILE.
  // If TAKTAK_FIRST_RUN_FILE does not exist, creates it, triggers a custom
  // event (kFirstOpen).
  if (IsFilePresent(FileType::TAKTAK_FIRST_RUN_FILE)) {
    base::Time taktak_first_run_file_creation_time =
        ReadFileCreationTime(FileType::TAKTAK_FIRST_RUN_FILE);
    base::Time seven_days_from_first_run_file_creation =
        taktak_first_run_file_creation_time + base::Days(7);
    const bool within_seven_days_since =
        now > taktak_first_run_file_creation_time &&
        now <= seven_days_from_first_run_file_creation;
    if (within_seven_days_since &&
        !IsFilePresent(FileType::TAKTAK_OPEN_IN_SEVEN_DAY_FILE)) {
      cs_handler_->HandleCustomEvent(kOpenInNext7Days);
      DVLOG(0) << "|>> Custom Event: Open in next 7 days";
      if (CreateFile(FileType::TAKTAK_OPEN_IN_SEVEN_DAY_FILE)) {
        DVLOG(0) << "|>> taktak_open_in_seven_day_file created.";
      }
    }
  } else if (CreateFile(FileType::TAKTAK_FIRST_RUN_FILE)) {
    cs_handler_->HandleCustomEvent(kFirstOpen);
    DVLOG(0) << "|>> Custom Event: First open";
  }
}

}  // namespace taktak_run_tracker
