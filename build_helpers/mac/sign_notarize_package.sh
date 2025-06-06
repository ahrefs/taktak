
#!/bin/bash

# This script signs the Taktak app and its components, prepares it for notarization,
# and packages it into a DMG file.
# Replace the paths and identifiers with your own values as needed.
# IMPORTANT: Observed that "Taktak Helper.app" will not be spwaned into the separate process
# if the app is signed with `--options=restrict,library,runtime,kill` and DRM playback will not work cause Widevine will not be loaded into the process.
# Usage:
# chmod +x sign_package.sh
# ./sign_package.sh


set -e

ROOT_DIR="$(dirname "$(greadlink -f "$0")")"
APP_NAME="Taktak.app"
APP_PATH="out/Release/$APP_NAME"
FRAMEWORK_PATH="$APP_PATH/Contents/Frameworks/Taktak Framework.framework"
HELPERS_PATH="$FRAMEWORK_PATH/Helpers"
LIBRARIES_PATH="$FRAMEWORK_PATH/Libraries"
ENTITLEMENTS_DIR="/Users/nyinyithan/ahrefs/taktak/entitlements"
SIGN_ID="Developer ID Application: Nyi Than (XKJ6D65VW3)"
TEAM_ID="XKJ6D65VW3"
APPLE_ID="nyinyithann@gmail.com"
NOTARY_PROFILE="notarytool-profile"
DMG_PATH="$ROOT_DIR/build/taktak.dmg"
ZIP_PATH="taktak_notarize.zip"

xattr -cs "$APP_PATH"

# Sign helper binaries and apps
codesign --sign "$SIGN_ID" --force --timestamp --identifier chrome_crashpad_handler --options=restrict,library,runtime,kill "$HELPERS_PATH/chrome_crashpad_handler"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.helper "$HELPERS_PATH/Taktak Helper.app"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.helper.renderer  --options restrict,kill,runtime --entitlements "$ENTITLEMENTS_DIR/helper-renderer-entitlements.plist" "$HELPERS_PATH/Taktak Helper (Renderer).app"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.helper  --options restrict,kill,runtime --entitlements "$ENTITLEMENTS_DIR/helper-gpu-entitlements.plist" "$HELPERS_PATH/Taktak Helper (GPU).app"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.helper.plugin --options restrict,kill,runtime --entitlements "$ENTITLEMENTS_DIR/helper-plugin-entitlements.plist" "$HELPERS_PATH/Taktak Helper (Plugin).app"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.framework.AlertNotificationService --options restrict,library,kill,runtime  "$HELPERS_PATH/Taktak Helper (Alerts).app"
codesign --sign "$SIGN_ID" --force --timestamp --identifier app_mode_loader --options restrict,library,kill,runtime  "$HELPERS_PATH/app_mode_loader"
codesign --sign "$SIGN_ID" --force --timestamp --identifier web_app_shortcut_copier --options restrict,library,kill,runtime  "$HELPERS_PATH/web_app_shortcut_copier"

# Sign libraries
codesign --sign "$SIGN_ID" --force --timestamp --identifier libEGL "$LIBRARIES_PATH/libEGL.dylib"
codesign --sign "$SIGN_ID" --force --timestamp --identifier libGLESv2 "$LIBRARIES_PATH/libGLESv2.dylib"
codesign --sign "$SIGN_ID" --force --timestamp --identifier libvk_swiftshader "$LIBRARIES_PATH/libvk_swiftshader.dylib"

# Sign framework and main app
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak.framework "$FRAMEWORK_PATH"
codesign --sign "$SIGN_ID" --force --timestamp --identifier com.taktak.taktak --options restrict,library,runtime,kill --entitlements "$ENTITLEMENTS_DIR/app-entitlements.plist"  "$APP_PATH"

# Verify the binary signature
codesign --verify --deep --verbose=4 "$APP_PATH"

# Prepare app notarization
ditto -c -k --keepParent "$APP_PATH" "$ZIP_PATH"

# Notarize the app
xcrun notarytool store-credentials "$NOTARY_PROFILE" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "APP_SPECIFIC_PASSWORD"
xcrun notarytool submit "$ZIP_PATH" --keychain-profile "$NOTARY_PROFILE" --wait
xcrun stapler staple "$APP_PATH"

# Package the app
chrome/installer/mac/pkg-dmg \
  --sourcefile --source "$APP_PATH" \
  --target "$DMG_PATH" \
  --volname Taktak --symlink /Applications:/Applications \
  --format UDBZ --verbosity 2