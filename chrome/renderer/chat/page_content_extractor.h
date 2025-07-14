// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_PAGE_CONTENT_EXTRACTOR_H
#define CHROMIUM_PAGE_CONTENT_EXTRACTOR_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "chrome/common/page_content_extractor/page_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class RenderFrame;
}

namespace ai_chat {

class PageContentExtractor
    : public chat::mojom::PageContentExtractor,
      public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PageContentExtractor> {
 public:
  PageContentExtractor(content::RenderFrame* render_frame,
                       service_manager::BinderRegistry* registry,
                       int32_t isolated_world_id);

  PageContentExtractor(const PageContentExtractor&) = delete;
  PageContentExtractor& operator=(const PageContentExtractor&) = delete;
  ~PageContentExtractor() override;

  base::WeakPtr<PageContentExtractor> GetWeakPtr();

 private:
  // chat::mojom::PageContentExtractor implementation:
  void ExtractPageContent(
      bool includesHTML,
      chat::mojom::PageContentExtractor::ExtractPageContentCallback callback)
      override;

  // RenderFrameObserver implementation:
  void OnDestruct() override;

  void BindReceiver(
      mojo::PendingReceiver<chat::mojom::PageContentExtractor> receiver);

  mojo::Receiver<chat::mojom::PageContentExtractor> receiver_{this};

  int32_t isolated_world_id_;

  void ExtractPageText(
      content::RenderFrame* render_frame,
      int32_t isolated_world_id,
      base::OnceCallback<void(const std::optional<std::string>& text,
                              const std::optional<std::string>& url)>,
      bool includesHTML);

  void OnPageTextExtracted(
      chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
      const std::optional<std::string>& text,
      const std::optional<std::string>& url);

  base::WeakPtrFactory<PageContentExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // CHROMIUM_PAGE_CONTENT_EXTRACTOR_H
