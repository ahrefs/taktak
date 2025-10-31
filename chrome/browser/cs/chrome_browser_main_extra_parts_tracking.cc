// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome_browser_main_extra_parts_tracking.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/task/current_thread.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/hang_watcher.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace base {
class FilePath;
}  // namespace base

namespace taktak_run_tracking {
namespace {
constexpr char kOpenToday[] = "open_today";
constexpr char kFirstOpen[] = "first_open";
constexpr char kOpenInNext7Days[] = "open_in_next_7_days";

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

class TrackingFileHelper {
 public:
  TrackingFileHelper() = delete;
  TrackingFileHelper(const TrackingFileHelper&) = delete;
  TrackingFileHelper& operator=(const TrackingFileHelper&) = delete;

  // Get the path of the file; returns false on failure.
  static bool TryGetFilePath(FileType file_type, base::FilePath* path) {
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
  static bool IsFilePresent(FileType file_type) {
    base::FilePath file_path;
    return !TryGetFilePath(file_type, &file_path) ||
           base::PathExists(file_path);
  }

  static void CreateFile(FileType file_type) {
    std::string file_type_name;
    switch (file_type) {
      case TAKTAK_FIRST_RUN_FILE:
        file_type_name = "taktak_first_run";
        break;
      case TAKTAK_OPEN_IN_SEVEN_DAY_FILE:
        file_type_name = "taktak_open_in_seven_day";
        break;
      case TAKTAK_OPEN_TODAY_FILE:
        file_type_name = "taktak_open_today";
        break;
      default:
        file_type_name = "";
        break;
    }
    if (TryCreateFile(file_type)) {
      DVLOG(0) << "|>> " << file_type_name << " is created.";
    } else {
      DVLOG(0) << "|>> Error at creating file: " << file_type_name;
    }
  }

  static bool TryCreateFile(FileType file_type) {
    base::FilePath file_path;
    if (!TryGetFilePath(file_type, &file_path)) {
      return false;
    }

    if (base::PathExists(file_path)) {
      return false;
    }

    if (!base::WriteFile(file_path, "")) {
      return false;
    }

    return true;
  }

  static void UpdateTodayOpenFile(std::string today_date) {
    base::FilePath file_path;
    if (TryGetFilePath(FileType::TAKTAK_OPEN_TODAY_FILE, &file_path) &&
        base::WriteFile(file_path, today_date)) {
      VLOG(0) << "|>> today date " << today_date
              << " is written in to tatak_today_open file.";
    } else {
      VLOG(0) << "|>> error writting " << today_date
              << " to tatak_today_open file.";
    }
  }

  static bool TryReadFile(FileType file_type, std::string* content) {
    base::FilePath file_path;
    if (TryGetFilePath(file_type, &file_path) && base::PathExists(file_path)) {
      VLOG(0) << "|>> file path for file type " << file_type << ": "
              << file_path.value();
      return base::ReadFileToString(file_path, content);
    }

    VLOG(0) << "|>> Path not found for file type: " << file_type;
    return false;
  }

  // Reads the creation time of the taktak_run file.
  // If the file does not exist, it will return base::Time().
  static base::Time ReadFileCreationTime(FileType file_type) {
    base::Time file_creation_time = base::Time();
    base::FilePath taktak_run_file_path;
    if (TryGetFilePath(file_type, &taktak_run_file_path)) {
      base::File::Info info;
      if (base::GetFileInfo(taktak_run_file_path, &info)) {
        file_creation_time = info.creation_time;
      }
    }
    return file_creation_time;
  }
};
}  // namespace

ChromeBrowserMainExtraPartsTracking::ChromeBrowserMainExtraPartsTracking() =
    default;

ChromeBrowserMainExtraPartsTracking::~ChromeBrowserMainExtraPartsTracking() =
    default;

void ChromeBrowserMainExtraPartsTracking::OnTrackOpenTodayEvent(
    const std::string today_date,
    web_request_helper::WebRequestResult result) {
  if (result.response_code() != 200) {
    VLOG(0) << __func__ << " |>> Error at tracking open-today custom event: "
            << result.response_code();
    return;
  }

  VLOG(0) << "|>> Custom Event: Open today";
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&TrackingFileHelper::UpdateTodayOpenFile, today_date));
}

void ChromeBrowserMainExtraPartsTracking::OnTrackFirstRunEvent(
    web_request_helper::WebRequestResult result) {
  if (result.response_code() != 200) {
    VLOG(0) << "|>> Error at tracking first-open custom event: "
            << result.response_code();
    return;
  }
  VLOG(0) << "|>> Custom Event: First open";
  base::ThreadPool::PostTask(FROM_HERE, {base::MayBlock()},
                             base::BindOnce(&TrackingFileHelper::CreateFile,
                                            FileType::TAKTAK_FIRST_RUN_FILE));
}

void ChromeBrowserMainExtraPartsTracking::OnTrackOpenWithinSevenDaysEvent(
    web_request_helper::WebRequestResult result) {
  if (result.response_code() != 200) {
    VLOG(0) << __func__ << " |>> Error at tracking open-today custom event: "
            << result.response_code();
    return;
  }

  VLOG(0) << "|>> Custom Event: Open in next 7 days";
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&TrackingFileHelper::CreateFile,
                     FileType::TAKTAK_OPEN_IN_SEVEN_DAY_FILE));
}

void ChromeBrowserMainExtraPartsTracking::PostProfileInit(
    Profile* profile,
    bool is_initial_profile) {
  if (!cs_handler_) {
    auto* sncm = g_browser_process->system_network_context_manager();
    scoped_refptr<network::SharedURLLoaderFactory> factory =
        sncm ? sncm->GetSharedURLLoaderFactory() : nullptr;
    if (!factory) {
      return;
    }
    cs_handler_ = std::make_unique<cs_handler::CSHandler>(std::move(factory));
  }

  // Get current local date in "YYYY-MM-DD" format
  base::Time now = base::Time::Now();
  base::Time::Exploded exploded_now;
  now.LocalExplode(&exploded_now);

  std::string today_date = base::NumberToString(exploded_now.year) + "-" +
                           base::NumberToString(exploded_now.month) + "-" +
                           base::NumberToString(exploded_now.day_of_month);

  // Track the 'open-today' event if this is the first open today (either no
  // stored date exists, or the stored date differs from today's date).
  std::string stored_date;
  bool should_track_open_today = false;
  if (!TrackingFileHelper::TryReadFile(FileType::TAKTAK_OPEN_TODAY_FILE,
                                       &stored_date)) {
    VLOG(0) << "|>> taktak_open_today file doesn't exist. It will be created after open_today count is sent.";
  }
  should_track_open_today = stored_date != today_date;

  if (should_track_open_today) {
    cs_handler_->HandleLaunchingCustomEvent(
        kOpenToday,
        base::BindOnce(
            &ChromeBrowserMainExtraPartsTracking::OnTrackOpenTodayEvent,
            base::Unretained(this), today_date));
  }

  // Checks if the TAKTAK_FIRST_RUN_FILE exists, indicating a prior application
  // run. If present, retrieves the file's creation time and calculates a
  // seven-day window from it. Compares the current time to determine if it
  // falls within this window. If within seven days and
  // TAKTAK_OPEN_IN_SEVEN_DAY_FILE does not exist, triggers a custom event
  // (kOpenInNext7Days). And Attempts to create TAKTAK_OPEN_IN_SEVEN_DAY_FILE.
  // If TAKTAK_FIRST_RUN_FILE does not exist, creates it, triggers a custom
  // event (kFirstOpen).
  if (TrackingFileHelper::IsFilePresent(FileType::TAKTAK_FIRST_RUN_FILE)) {
    base::Time taktak_first_run_file_creation_time =
        TrackingFileHelper::ReadFileCreationTime(
            FileType::TAKTAK_FIRST_RUN_FILE);
    base::Time seven_days_from_first_run_file_creation =
        taktak_first_run_file_creation_time + base::Days(7);
    const bool within_seven_days_since =
        now > taktak_first_run_file_creation_time &&
        now <= seven_days_from_first_run_file_creation;
    if (within_seven_days_since &&
        !TrackingFileHelper::IsFilePresent(
            FileType::TAKTAK_OPEN_IN_SEVEN_DAY_FILE)) {
      cs_handler_->HandleLaunchingCustomEvent(
          kOpenInNext7Days,
          base::BindOnce(&ChromeBrowserMainExtraPartsTracking::
                             OnTrackOpenWithinSevenDaysEvent,
                         base::Unretained(this)));
    }
  } else {
    cs_handler_->HandleLaunchingCustomEvent(
        kFirstOpen,
        base::BindOnce(
            &ChromeBrowserMainExtraPartsTracking::OnTrackFirstRunEvent,
            base::Unretained(this)));
  }
}
}  // namespace taktak_run_tracking
