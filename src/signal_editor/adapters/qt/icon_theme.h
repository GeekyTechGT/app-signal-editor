#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace signal_editor::adapters::qt {

enum class SvgIconMode {
    Original,
    LightMonochrome,
};

/**
 * @brief Builds a themed icon from an SVG resource.
 *
 * `Original` returns the SVG with its embedded colors. `LightMonochrome`
 * rasterizes the SVG at common UI sizes and tints it with `tint`, which is
 * suitable for dark themes that need bright toolbar icons.
 */
QIcon load_svg_icon(const QString& resource_path,
                    SvgIconMode mode = SvgIconMode::Original,
                    const QColor& tint = QColor(245, 245, 245));

}  // namespace signal_editor::adapters::qt
