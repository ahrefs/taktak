// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/encryptedmedia/navigator_request_media_key_system_access.h"

#include <algorithm>
#include <vector>

#include "base/memory/ptr_util.h"
#include "components/drm/taktak_drm.mojom-blink.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom-blink.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_encrypted_media_client.h"
#include "third_party/blink/public/platform/web_encrypted_media_request.h"
#include "third_party/blink/public/platform/web_media_key_system_configuration.h"
#include "third_party/blink/public/platform/web_media_key_system_media_capability.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/modules/encryptedmedia/encrypted_media_utils.h"
#include "third_party/blink/renderer/modules/encryptedmedia/media_key_session.h"
#include "third_party/blink/renderer/modules/encryptedmedia/media_key_system_access.h"
#include "third_party/blink/renderer/modules/encryptedmedia/media_key_system_access_initializer_base.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/encrypted_media_request.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/network/mime/content_type.h"
#include "third_party/blink/renderer/platform/network/parsed_content_type.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
class LocalFrame;

namespace {

// This class allows capabilities to be checked and a MediaKeySystemAccess
// object to be created asynchronously.
class MediaKeySystemAccessInitializer final
    : public MediaKeySystemAccessInitializerBase {
 public:
  MediaKeySystemAccessInitializer(
      ExecutionContext*,
      ScriptPromiseResolverBase*,
      const String& key_system,
      const HeapVector<Member<MediaKeySystemConfiguration>>&
          supported_configurations);

  MediaKeySystemAccessInitializer(const MediaKeySystemAccessInitializer&) =
      delete;
  MediaKeySystemAccessInitializer& operator=(
      const MediaKeySystemAccessInitializer&) = delete;

  ~MediaKeySystemAccessInitializer() override = default;

  // EncryptedMediaRequest implementation.
  void RequestSucceeded(
      std::unique_ptr<WebContentDecryptionModuleAccess>) override;
  void RequestNotSupported(const WebString& error_message) override;

  void StartRequestAsync();

  void Trace(Visitor* visitor) const override {
    MediaKeySystemAccessInitializerBase::Trace(visitor);
  }
};

MediaKeySystemAccessInitializer::MediaKeySystemAccessInitializer(
    ExecutionContext* context,
    ScriptPromiseResolverBase* resolver,
    const String& key_system,
    const HeapVector<Member<MediaKeySystemConfiguration>>&
        supported_configurations)
    : MediaKeySystemAccessInitializerBase(
          context,
          resolver,
          key_system,
          supported_configurations,
          /*is_from_media_capabilities=*/false) {}

void MediaKeySystemAccessInitializer::RequestSucceeded(
    std::unique_ptr<WebContentDecryptionModuleAccess> access) {
  DVLOG(3) << __func__;

  if (!IsExecutionContextValid())
    return;

  resolver_->DowncastTo<MediaKeySystemAccess>()->Resolve(
      MakeGarbageCollected<MediaKeySystemAccess>(std::move(access)));
  resolver_.Clear();
}

void MediaKeySystemAccessInitializer::RequestNotSupported(
    const WebString& error_message) {
  DVLOG(3) << __func__ << " error: " << error_message.Ascii();

  if (!IsExecutionContextValid())
    return;

  resolver_->Reject(MakeGarbageCollected<DOMException>(
      DOMExceptionCode::kNotSupportedError, error_message));
  resolver_.Clear();
}

void MediaKeySystemAccessInitializer::StartRequestAsync() {
  if (!IsExecutionContextValid() || !DomWindow())
    return;

  // 6. Asynchronously determine support, and if allowed, create and
  //    initialize the MediaKeySystemAccess object.
  DCHECK(!DomWindow()->document()->IsPrerendering());

  WebEncryptedMediaClient* media_client =
      EncryptedMediaUtils::GetEncryptedMediaClientFromLocalDOMWindow(
          DomWindow());
  media_client->RequestMediaKeySystemAccess(WebEncryptedMediaRequest(this));
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
// Notifies Taktak about the widevine key system access request.
// This allows Taktak browser to handle DRM-related permissions and settings
// when a webpage attempts to use Widevine DRM.
void NotifyWidevineRequest(MediaKeySystemAccessInitializer* initializer,
                            LocalFrame* frame) {
  if (initializer->KeySystem() == "com.widevine.alpha") {
    if (frame->Client()->GetRemoteNavigationAssociatedInterfaces()) {
      DVLOG(0) << "||> Notifying Taktak about Widevine DRM request.";
      mojo::AssociatedRemote<taktak_drm::mojom::blink::TaktakDRM>
          taktak_drm_binding;
      frame->Client()->GetRemoteNavigationAssociatedInterfaces()->GetInterface(
          &taktak_drm_binding);
      DCHECK(taktak_drm_binding.is_bound());
      taktak_drm_binding->HandleWidevineKeySystemRequest();
    }
  }
}
#endif

}  // namespace

ScriptPromise<MediaKeySystemAccess>
NavigatorRequestMediaKeySystemAccess::requestMediaKeySystemAccess(
    ScriptState* script_state,
    Navigator& navigator,
    const String& key_system,
    const HeapVector<Member<MediaKeySystemConfiguration>>&
        supported_configurations,
    ExceptionState& exception_state) {
  DVLOG(3) << __func__;

  LocalDOMWindow* window = LocalDOMWindow::From(script_state);
  if (!window->IsFeatureEnabled(
          network::mojom::PermissionsPolicyFeature::kEncryptedMedia,
          ReportOptions::kReportOnFailure)) {
    UseCounter::Count(window,
                      WebFeature::kEncryptedMediaDisabledByFeaturePolicy);
    window->AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::ConsoleMessageSource::kJavaScript,
        mojom::ConsoleMessageLevel::kWarning,
        kEncryptedMediaPermissionsPolicyConsoleWarning));
    exception_state.ThrowSecurityError(
        "requestMediaKeySystemAccess is disabled by permissions policy.");
    return EmptyPromise();
  }

  // From https://w3c.github.io/encrypted-media/#requestMediaKeySystemAccess
  // When this method is invoked, the user agent must run the following steps:
  // 1. If keySystem is the empty string, return a promise rejected with a
  //    newly created TypeError.
  if (key_system.empty()) {
    exception_state.ThrowTypeError("The keySystem parameter is empty.");
    return EmptyPromise();
  }

  // 2. If supportedConfigurations is empty, return a promise rejected with
  //    a newly created TypeError.
  if (!supported_configurations.size()) {
    exception_state.ThrowTypeError(
        "The supportedConfigurations parameter is empty.");
    return EmptyPromise();
  }

  // 3. Let document be the calling context's Document.
  //    (Done at the begining of this function.)
  if (!window->GetFrame()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidStateError,
        "The context provided is not associated with a page.");
    return EmptyPromise();
  }

  UseCounter::Count(*window, WebFeature::kEncryptedMediaSecureOrigin);
  window->CountUseOnlyInCrossOriginIframe(
      WebFeature::kEncryptedMediaCrossOriginIframe);

  // 4. Let origin be the origin of document.
  //    (Passed with the execution context.)

  // 5. Let promise be a new promise.
  auto* resolver =
      MakeGarbageCollected<ScriptPromiseResolver<MediaKeySystemAccess>>(
          script_state);
  MediaKeySystemAccessInitializer* initializer =
      MakeGarbageCollected<MediaKeySystemAccessInitializer>(
          window, resolver, key_system, supported_configurations);
  auto promise = resolver->Promise();

  // Defer to determine support until the prerendering page is activated.
  if (window->document()->IsPrerendering()) {
    window->document()->AddPostPrerenderingActivationStep(
        WTF::BindOnce(&MediaKeySystemAccessInitializer::StartRequestAsync,
                      WrapWeakPersistent(initializer)));
    return promise;
  }

  // 6. Asynchronously determine support, and if allowed, create and
  //    initialize the MediaKeySystemAccess object.
  WebEncryptedMediaClient* media_client =
      EncryptedMediaUtils::GetEncryptedMediaClientFromLocalDOMWindow(window);
  media_client->RequestMediaKeySystemAccess(
      WebEncryptedMediaRequest(initializer));

// Taktak MVP doesn't support DRM playback on Windows.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
  NotifyWidevineRequest(initializer, window->GetFrame());
#endif

  // 7. Return promise.
  return promise;
}

}  // namespace blink