#include "cs_api_client_module_bridge.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/cs/android/cs_api_client_service.h"
#include "chrome/browser/cs/android/cs_api_client_service_factory.h"

#include "chrome/browser/cs/android/jni_headers/CsApiClientModuleBridge_jni.h"

using jni_zero::JavaParamRef;
using jni_zero::JavaRef;

namespace cs_api_client_module {
CsApiClientModuleBridge::CsApiClientModuleBridge(
  JNIEnv* env,
  const JavaRef<jobject>& jobj,
  Profile* profile
) : java_object_(env, jobj) {
  CHECK(!profile->IsOffTheRecord());
  cs_api_client_service_ =
        CSApiClientServiceFactory::GetInstance()->GetForBrowserContext(profile);
}

void CsApiClientModuleBridge::Destroy(
  JNIEnv* env,
  const JavaParamRef<jobject>& obj
) {
  delete this;
}

CsApiClientModuleBridge::~CsApiClientModuleBridge() = default;

static jlong JNI_CsApiClientModuleBridge_Create(
  JNIEnv* env,
  const JavaParamRef<jobject>& obj,
  Profile* profile
) {
  CsApiClientModuleBridge* native_bridge =
    new CsApiClientModuleBridge(env, obj, profile);
  return reinterpret_cast<intptr_t>(native_bridge);
}
} // namespace cs_api_client_module
