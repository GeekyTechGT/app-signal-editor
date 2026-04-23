#include "signal_editor/adapters/qt/icon_theme.h"

#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include <array>

namespace signal_editor::adapters::qt {
namespace {

QByteArray load_svg_bytes(const QString& resource_path) {
    QFile file(resource_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QPixmap tint_pixmap(QPixmap pixmap, const QColor& tint) {
    if (pixmap.isNull()) {
        return {};
    }

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), tint);
    return pixmap;
}

QIcon load_monochrome_svg_icon(const QString& resource_path, const QColor& tint) {
    const QByteArray svg_data = load_svg_bytes(resource_path);
    QSvgRenderer renderer(svg_data);
    if (!renderer.isValid()) {
        return {};
    }

    QIcon icon;
    constexpr std::array<int, 6> kIconSizes = {16, 18, 20, 24, 28, 32};
    for (const int edge : kIconSizes) {
        QPixmap pixmap(QSize(edge, edge));
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        renderer.render(&painter);
        icon.addPixmap(tint_pixmap(std::move(pixmap), tint));
    }
    return icon;
}

}  // namespace

QIcon load_svg_icon(const QString& resource_path,
                    SvgIconMode mode,
                    const QColor& tint) {
    if (mode == SvgIconMode::Original) {
        return QIcon(resource_path);
    }
    return load_monochrome_svg_icon(resource_path, tint);
}

}  // namespace signal_editor::adapters::qt
