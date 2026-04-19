#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace signal_editor::adapters::qt {

/**
 * @brief Available application themes.
 */
enum class AppTheme {
    Dark,
    Light,
    System ///< Use the OS palette (Fusion style, no QSS override).
};

/**
 * @brief Converts a BCP-47-style name to AppTheme.
 *
 * Recognised values: "dark", "light". Any other value (including "system")
 * returns AppTheme::System.
 */
[[nodiscard]] AppTheme theme_from_string(const QString& name);

/**
 * @brief Applies the selected visual theme to the running QApplication.
 *
 * Loads the relevant QSS from embedded Qt resources (:/themes/dark.qss or
 * :/themes/light.qss), sets the Fusion style, and configures a matching
 * QPalette. Call this once at startup and again whenever the user changes
 * the theme through the settings panel.
 *
 * @param app   QApplication instance to style.
 * @param theme Theme variant to apply.
 */
void apply_theme(QApplication& app, AppTheme theme,
                 const QColor& accent = QColor(0, 120, 212));

}  // namespace signal_editor::adapters::qt
