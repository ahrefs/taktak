// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H
#define CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H

#include <string>

#include "base/functional/callback_forward.h"
#include "chrome/common/page_content_extractor/page_content_extractor.mojom.h"
#include "chrome/renderer/chat/page_content_extractor.h" // nogncheck

namespace content {
    class WebContents;
}  // namespace content

class PageContentExtractorHelper{
 public:
  explicit PageContentExtractorHelper(content::WebContents* web_contents);
  ~PageContentExtractorHelper();
  PageContentExtractorHelper(const PageContentExtractorHelper&) = delete;
  PageContentExtractorHelper& operator=(const PageContentExtractorHelper&) =
      delete;
  void ExtractPageContent(
      base::OnceCallback<void(std::string content, std::string url)> callback,
      bool includesHTML);

 private:
  base::WeakPtr<content::WebContents> web_contents_;
};

#endif  // CHROMIUM_PAGE_CONTENT_EXTRACTOR_HELPER_H
