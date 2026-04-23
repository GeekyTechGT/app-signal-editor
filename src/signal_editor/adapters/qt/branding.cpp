#include "signal_editor/adapters/qt/branding.h"

#include "signal_editor/adapters/qt/constants.hpp"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

#include <array>

namespace signal_editor::adapters::qt {
namespace ui = signal_editor::adapters::qt::constants;

namespace {

constexpr QColor kBrandTint(0x22, 0xD3, 0xEE);

QByteArray load_logo_svg_data() {
    QFile svg_file(QStringLiteral(":/img/app_logo.svg"));
    if (!svg_file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return svg_file.readAll();
}

QPixmap tint_logo_pixmap(QPixmap pixmap, const QColor& color) {
    if (pixmap.isNull()) {
        return {};
    }

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    return pixmap;
}

QPixmap render_svg_logo(const QSize& size, const QColor& color = kBrandTint) {
    const QByteArray svg_data = load_logo_svg_data();
    QSvgRenderer renderer(svg_data);
    if (!renderer.isValid()) {
        return {};
    }

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    return tint_logo_pixmap(std::move(pixmap), color);
}

}  // namespace

QIcon build_application_icon() {
    QIcon icon(ui::window_icon_resource_path());
    if (!icon.isNull()) {
        return icon;
    }

    constexpr std::array<int, 7> kIconSizes = {16, 20, 24, 32, 48, 64, 128};
    for (const int edge : kIconSizes) {
        const QPixmap pixmap = render_svg_logo(QSize(edge, edge));
        if (!pixmap.isNull()) {
            icon.addPixmap(pixmap);
        }
    }
    return icon;
}

QPixmap render_application_logo(const QSize& size) {
    return render_svg_logo(size);
}

QString splash_logo_path() {
    return ui::logo_resource_path();
}

}  // namespace signal_editor::adapters::qt
