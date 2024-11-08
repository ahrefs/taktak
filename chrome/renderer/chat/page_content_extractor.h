#ifndef CHROMIUM_PAGE_CONTENT_EXTRACTOR_H
#define CHROMIUM_PAGE_CONTENT_EXTRACTOR_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "chrome/common/chat/page_content_extractor.mojom.h"
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
                       service_manager::BinderRegistry* registry);

  PageContentExtractor(const PageContentExtractor&) = delete;
  PageContentExtractor& operator=(const PageContentExtractor&) = delete;
  ~PageContentExtractor() override;

  base::WeakPtr<PageContentExtractor> GetWeakPtr();

 private:
  // PageContentExtractor implementation:
  void ExtractPageContent(
      chat::mojom::PageContentExtractor::ExtractPageContentCallback callback)
      override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::PageContentExtractor> receiver);

  chat::mojo::Receiver<chat::mojom::PageContentExtractor> receiver_{this};

  base::WeakPtrFactory<PageContentExtractor> weak_ptr_factory_{this};

  void ExtractPageText(
      content::RenderFrame* render_frame,
      int32_t isolated_world_id,
      base::OnceCallback<void(const std::optional<std::string>& text)>);
};

}  // namespace ai_chat
#endif  // CHROMIUM_PAGE_CONTENT_EXTRACTOR_H
