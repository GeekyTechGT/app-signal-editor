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

/**
 * @file
 * @brief Shared Qt-facing application constants and settings keys.
 */

/** @brief Stable application identifier used for settings and Qt metadata. */
inline constexpr auto kAppId = SIGNAL_EDITOR_UI_APP_ID;
/** @brief Human-readable product name shown by the GUI. */
inline constexpr auto kDisplayName = "Signal Editor";
/** @brief Company name exposed to Qt metadata and branded UI elements. */
inline constexpr auto kCompanyName = "GeekyTech";
/** @brief Branded logo embedded in the Qt resource collection for splash/about surfaces. */
inline constexpr auto kLogoResourcePath = ":/img/app_logo.svg";
/** @brief Application icon source embedded in the Qt resource collection. */
inline constexpr auto kWindowIconResourcePath = ":/img/app_logo.svg";
/** @brief Splash screen stylesheet embedded in the Qt resource collection. */
inline constexpr auto kSplashStylesheetResourcePath = ":/themes/splash_screen.qss";
/** @brief Directory name used when loading translation catalogs from disk. */
inline constexpr auto kTranslationsDirName = "translations";
/** @brief Translation basename prefix shared by packaged `.qm` catalogs. */
inline constexpr auto kTranslationBaseName = "signal_editor_";
/** @brief Versioned settings namespace that isolates persisted UI state. */
inline constexpr auto kSettingsVersionScope = SIGNAL_EDITOR_UI_SETTINGS_VERSION;

/** @brief Number of staged messages shown by the splash screen. */
inline constexpr int kSplashStepCount = 4;
/** @brief Delay between consecutive splash status updates in milliseconds. */
inline constexpr int kSplashStepDelayMs = 400;
/** @brief Minimum splash visibility to avoid abrupt startup flashes. */
inline constexpr int kSplashMinDurationMs = 2000;
/** @brief Default splash width in pixels. */
inline constexpr int kSplashWidth = 460;
/** @brief Default splash height in pixels. */
inline constexpr int kSplashHeight = 360;

/** @brief Persisted theme selection key. */
inline constexpr auto kSettingsKeyTheme = "ui/theme";
/** @brief Persisted language selection key. */
inline constexpr auto kSettingsKeyLanguage = "ui/language";
/** @brief Persisted font family key. */
inline constexpr auto kSettingsKeyFontFamily = "ui/font_family";
/** @brief Persisted font size key. */
inline constexpr auto kSettingsKeyFontSize = "ui/font_size";
/** @brief Persisted high-contrast preference key. */
inline constexpr auto kSettingsKeyHighContrast = "ui/high_contrast";
/** @brief Persisted widget density key. */
inline constexpr auto kSettingsKeyWidgetDensity = "ui/widget_density";
/** @brief Persisted animation duration key. */
inline constexpr auto kSettingsKeyAnimationDuration = "ui/animation_duration_ms";
/** @brief Persisted accent color key. */
inline constexpr auto kSettingsKeyPrimaryColor = "ui/primary_color";

/** @brief Returns the stable application identifier as a Qt string. */
inline QString app_id() {
    return QString::fromUtf8(kAppId);
}

/** @brief Returns the human-readable application name as a Qt string. */
inline QString display_name() {
    return QString::fromUtf8(kDisplayName);
}

/** @brief Returns the company name as a Qt string. */
inline QString company_name() {
    return QString::fromUtf8(kCompanyName);
}

/** @brief Returns the packaged logo resource path. */
inline QString logo_resource_path() {
    return QString::fromUtf8(kLogoResourcePath);
}

/** @brief Returns the packaged application icon resource path. */
inline QString window_icon_resource_path() {
    return QString::fromUtf8(kWindowIconResourcePath);
}

/** @brief Returns the packaged splash screen stylesheet resource path. */
inline QString splash_stylesheet_resource_path() {
    return QString::fromUtf8(kSplashStylesheetResourcePath);
}

/** @brief Returns the on-disk translations directory name. */
inline QString translations_dir_name() {
    return QString::fromUtf8(kTranslationsDirName);
}

/** @brief Returns the translation basename shared by `.qm` catalogs. */
inline QString translation_base_name() {
    return QString::fromUtf8(kTranslationBaseName);
}

/** @brief Returns the versioned settings namespace scope. */
inline QString settings_version_scope() {
    return QString::fromUtf8(kSettingsVersionScope);
}

/** @brief Returns the build-time application version. */
inline QString application_version() {
    return QString::fromUtf8(SIGNAL_EDITOR_VERSION);
}

}  // namespace signal_editor::adapters::qt::constants
