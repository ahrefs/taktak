package org.chromium.chrome.browser.cs;

import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("cs_api_client_module")
public class CsApiClientModuleBridge {
    private long mCsApiClientModuleBridge;

    public CsApiClientModuleBridge(Profile profile) {
        mCsApiClientModuleBridge =
            CsApiClientModuleBridgeJni.get()
                .create(CsApiClientModuleBridge.this, profile);
    }

    void destroy() {
        if (mCsApiClientModuleBridge != 0) {
            CsApiClientModuleBridgeJni.get()
                .destroy(mCsApiClientModuleBridge, CsApiClientModuleBridge.this);
            mCsApiClientModuleBridge = 0;
        }
    }

    @NativeMethods
    interface Natives {
        long create(CsApiClientModuleBridge caller, @JniType("Profile*") Profile profile);

        void destroy(long nativeCsApiClientModuleBridge, CsApiClientModuleBridge caller);
    }
}
