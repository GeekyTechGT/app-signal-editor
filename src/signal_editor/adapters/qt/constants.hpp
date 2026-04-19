#pragma once

#include "signal_editor/version.h"

#include <QString>

#ifndef SIGNAL_EDITOR_UI_APP_ID
#define SIGNAL_EDITOR_UI_APP_ID "signal-editor"
#endif

#ifndef SIGNAL_EDITOR_UI_SETTINGS_VERSION
#define SIGNAL_EDITOR_UI_SETTINGS_VERSION "v1.0.0"
#endif

namespace signal_editor::adapters::qt::constants {

inline constexpr auto kAppId = SIGNAL_EDITOR_UI_APP_ID;
inline constexpr auto kDisplayName = "Signal Editor";
inline constexpr auto kCompanyName = "GeekyTech";
inline constexpr auto kLogoResourcePath = ":/img/app_logo.png";
inline constexpr auto kTranslationsDirName = "translations";
inline constexpr auto kTranslationBaseName = "signal_editor_";
inline constexpr auto kSettingsVersionScope = SIGNAL_EDITOR_UI_SETTINGS_VERSION;

inline constexpr int kSplashStepCount = 4;
inline constexpr int kSplashStepDelayMs = 400;
inline constexpr int kSplashMinDurationMs = 2000;
inline constexpr int kSplashWidth = 460;
inline constexpr int kSplashHeight = 360;

inline constexpr auto kSettingsKeyTheme = "ui/theme";
inline constexpr auto kSettingsKeyLanguage = "ui/language";
inline constexpr auto kSettingsKeyFontFamily = "ui/font_family";
inline constexpr auto kSettingsKeyFontSize = "ui/font_size";
inline constexpr auto kSettingsKeyHighContrast = "ui/high_contrast";
inline constexpr auto kSettingsKeyWidgetDensity = "ui/widget_density";
inline constexpr auto kSettingsKeyAnimationDuration = "ui/animation_duration_ms";
inline constexpr auto kSettingsKeyPrimaryColor = "ui/primary_color";

inline QString app_id() {
    return QString::fromUtf8(kAppId);
}

inline QString display_name() {
    return QString::fromUtf8(kDisplayName);
}

inline QString company_name() {
    return QString::fromUtf8(kCompanyName);
}

inline QString logo_resource_path() {
    return QString::fromUtf8(kLogoResourcePath);
}

inline QString translations_dir_name() {
    return QString::fromUtf8(kTranslationsDirName);
}

inline QString translation_base_name() {
    return QString::fromUtf8(kTranslationBaseName);
}

inline QString settings_version_scope() {
    return QString::fromUtf8(kSettingsVersionScope);
}

inline QString application_version() {
    return QString::fromUtf8(SIGNAL_EDITOR_VERSION);
}

}  // namespace signal_editor::adapters::qt::constants
