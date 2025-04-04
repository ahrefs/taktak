#ifndef CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_
#define CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_

#include <jni.h>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/cs/android/cs_api_client_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "url/gurl.h"

class Profile;

namespace cs_api_client_module {
class CSApiClientModuleBridge {
  public:
    CSApiClientModuleBridge(JNIEnv* env, const jni_zero::JavaRef<jobject>& obj, Profile* profile);
    CSApiClientModuleBridge(const CSApiClientModuleBridge&) = delete;
    CSApiClientModuleBridge& operator=(const CSApiClientModuleBridge&) = delete;

    void Destroy(JNIEnv* env, const jni_zero::JavaParamRef<jobject>& obj);

    void Handle(JNIEnv* env,
                const jni_zero::JavaParamRef<jobject>& obj,
                const jni_zero::JavaParamRef<jstring>& url);

  private:
    ~CSApiClientModuleBridge();

    GURL last_committed_url_;
    raw_ptr<CSApiClientService> cs_api_client_service_;
    jni_zero::ScopedJavaGlobalRef<jobject> java_object_;

    const base::WeakPtrFactory<CSApiClientModuleBridge> weak_ptr_factory_{this};
};
} // namespace cs_api_client_module
#endif // CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_
