#ifndef CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_
#define CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_

#include <jni.h>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"

class Profile;

namespace cs_api_client_module {
class CsApiClientModuleBridge {
    public:
        CsApiClientModuleBridge(JNIEnv* env, const jni_zero::JavaRef<jobject>& obj, Profile* profile);
        CsApiClientModuleBridge(const CsApiClientModuleBridge&) = delete;
        CsApiClientModuleBridge& operator=(const CsApiClientModuleBridge&) = delete;

        void Destroy(JNIEnv* env, const jni_zero::JavaParamRef<jobject>& obj);

    private:
        ~CsApiClientModuleBridge();

        jni_zero::ScopedJavaGlobalRef<jobject> java_object_;
};
} // namespace cs_api_client_module
#endif // CHROME_BROWSER_CS_CS_API_CLIENT_MODULE_BRIDGE_H_