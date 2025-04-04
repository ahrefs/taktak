package org.chromium.chrome.browser.cs;

import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.url.GURL;

@JNINamespace("cs_api_client_module")
public class CSApiClientModuleBridge {
    private long mCSApiClientModuleBridge;

    public CSApiClientModuleBridge(Profile profile) {
        mCSApiClientModuleBridge =
            CSApiClientModuleBridgeJni.get()
                .create(CSApiClientModuleBridge.this, profile);
    }

    void destroy() {
        if (mCSApiClientModuleBridge != 0) {
            CSApiClientModuleBridgeJni.get()
                .destroy(mCSApiClientModuleBridge, CSApiClientModuleBridge.this);
            mCSApiClientModuleBridge = 0;
        }
    }

    void handle(String url) {
        if (mCSApiClientModuleBridge != 0) {
            CSApiClientModuleBridgeJni.get()
                .handle(mCSApiClientModuleBridge, CSApiClientModuleBridge.this, url);
        }
    }

    @NativeMethods
    interface Natives {
        long create(CSApiClientModuleBridge caller, @JniType("Profile*") Profile profile);

        void destroy(long nativeCSApiClientModuleBridge, CSApiClientModuleBridge caller);

        void handle(
            long nativeCSApiClientModuleBridge,
            CSApiClientModuleBridge caller,
            String url);
    }
}
