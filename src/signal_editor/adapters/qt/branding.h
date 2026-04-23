#pragma once

#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace signal_editor::adapters::qt {

/**
 * @brief Builds the application window icon from packaged branding assets.
 *
 * The preferred source is the configured icon resource. When Qt cannot load it
 * directly, the function falls back to rasterizing the packaged SVG logo at
 * common window-icon sizes.
 *
 * @return Application icon suitable for windows, dialogs, and task switchers.
 */
QIcon build_application_icon();

/**
 * @brief Renders the packaged SVG logo to a pixmap of the requested size.
 *
 * The helper is used by splash screens, About dialogs, and other branded UI
 * surfaces that need a concrete pixmap rather than a scalable icon object.
 *
 * @param size Target pixel size for the generated pixmap.
 * @return Rendered logo pixmap, or a null pixmap when the SVG cannot be read.
 */
QPixmap render_application_logo(const QSize& size);

/**
 * @brief Returns the logo path consumed by the splash screen widget.
 *
 * This indirection keeps splash setup independent from the concrete resource
 * chosen for the current branding pack.
 *
 * @return Qt resource or filesystem path for the splash logo.
 */
QString splash_logo_path();

}  // namespace signal_editor::adapters::qt
