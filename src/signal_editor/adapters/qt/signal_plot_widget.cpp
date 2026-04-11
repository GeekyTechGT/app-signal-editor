#include "signal_editor/adapters/qt/signal_plot_widget.h"

#include "signal_editor/core/domain/signal.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>
#include <limits>

namespace myprj::signal_editor::adapters::qt {

namespace {
constexpr QColor kBgTop(10, 17, 24);
constexpr QColor kBgMid(15, 24, 34);
constexpr QColor kBgBot(22, 33, 45);
constexpr QColor kPlotCard(7, 12, 18, 210);
constexpr QColor kPlotFrame(116, 139, 162, 90);
constexpr QColor kAxis(119, 136, 156);
constexpr QColor kGrid(57, 72, 89, 170);
constexpr QColor kCurve(93, 211, 196);
constexpr QColor kCurveFill(93, 211, 196, 55);
constexpr QColor kHandle(244, 166, 74);
constexpr QColor kHandleEdge(68, 43, 9);
constexpr QColor kSelectedHandle(255, 124, 92);
constexpr QColor kSelectedHandleEdge(255, 238, 214);
constexpr QColor kInfoBubble(17, 25, 35, 235);
constexpr QColor kInfoBorder(244, 166, 74);
constexpr QColor kInfoText(237, 242, 247);
constexpr QColor kPinnedMarker(255, 208, 137);
constexpr QColor kCrosshair(244, 166, 74, 95);
constexpr QColor kText(221, 229, 237);
constexpr QColor kPlaceholder(129, 145, 161);
constexpr int kPlotMargin = 48;

void format_axis_label(double v, QString& out) {
    if (std::fabs(v) >= 1000.0 || (v != 0.0 && std::fabs(v) < 0.01)) {
        out = QString::number(v, 'g', 4);
    } else {
        out = QString::number(v, 'f', 3);
    }
}

QString format_coordinates(double t, double y) {
    return QStringLiteral("x=%1  y=%2")
        .arg(t, 0, 'f', 4)
        .arg(y, 0, 'f', 4);
}

bool point_inside_plot(const QPointF& point, int width, int height) {
    return point.x() >= static_cast<double>(kPlotMargin) &&
           point.x() <= static_cast<double>(width - kPlotMargin) &&
           point.y() >= static_cast<double>(kPlotMargin) &&
           point.y() <= static_cast<double>(height - kPlotMargin);
}

void draw_coordinate_bubble(QPainter& painter,
                            const QRectF& bounds,
                            const QPointF& anchor,
                            const QString& text) {
    QFontMetrics metrics(painter.font());
    QRect text_rect = metrics.boundingRect(text).adjusted(-8, -6, 8, 6);
    QRectF bubble(text_rect);
    bubble.moveBottomLeft(QPointF(anchor.x() + 12.0, anchor.y() - 12.0));

    if (bubble.right() > bounds.right() - 4.0) {
        bubble.moveRight(bounds.right() - 4.0);
    }
    if (bubble.left() < bounds.left() + 4.0) {
        bubble.moveLeft(bounds.left() + 4.0);
    }
    if (bubble.top() < bounds.top() + 4.0) {
        bubble.moveTop(anchor.y() + 12.0);
    }

    painter.setPen(QPen(kInfoBorder, 1.0));
    painter.setBrush(kInfoBubble);
    painter.drawRoundedRect(bubble, 6.0, 6.0);
    painter.setPen(kInfoText);
    painter.drawText(bubble, Qt::AlignCenter, text);
}

void draw_badge(QPainter& painter,
                const QPointF& top_left,
                const QString& text,
                const QColor& fill,
                const QColor& outline,
                const QColor& text_color) {
    QFontMetrics metrics(painter.font());
    QRectF badge(metrics.boundingRect(text).adjusted(-10, -5, 10, 5));
    badge.moveTopLeft(top_left);
    painter.setPen(QPen(outline, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(badge, 8.0, 8.0);
    painter.setPen(text_color);
    painter.drawText(badge, Qt::AlignCenter, text);
}
}  // namespace

SignalPlotWidget::SignalPlotWidget(QWidget* parent) : QWidget(parent) {
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
}

void SignalPlotWidget::set_signal(Signal* signal) {
    signal_ = signal;
    drag_mode_ = DragMode::None;
    clear_selection();
    clear_hovered_index();
    pinned_data_point_visible_ = false;
    auto_fit_view();
    update();
}

void SignalPlotWidget::refresh() {
    auto_fit_view();
    update();
}

bool SignalPlotWidget::is_drag_active() const noexcept {
    return drag_mode_ != DragMode::None;
}

bool SignalPlotWidget::has_selection() const noexcept {
    return signal_ != nullptr && selected_index_ < signal_->size();
}

bool SignalPlotWidget::has_hovered_handle() const noexcept {
    return signal_ != nullptr && hovered_index_ < signal_->size();
}

void SignalPlotWidget::set_selected_index(std::size_t index) {
    selected_index_ = index;
}

void SignalPlotWidget::clear_selection() {
    selected_index_ = std::numeric_limits<std::size_t>::max();
}

void SignalPlotWidget::set_hovered_index(std::size_t index) {
    hovered_index_ = index;
}

void SignalPlotWidget::clear_hovered_index() {
    hovered_index_ = std::numeric_limits<std::size_t>::max();
}

void SignalPlotWidget::auto_fit_view() {
    if (signal_ == nullptr || signal_->empty()) {
        view_t_min_ = 0.0;
        view_t_max_ = 1.0;
        view_y_min_ = -1.0;
        view_y_max_ = 1.0;
        return;
    }

    view_t_min_ = signal_->t_min();
    view_t_max_ = signal_->t_max();
    view_y_min_ = signal_->y_min();
    view_y_max_ = signal_->y_max();

    if (view_t_max_ - view_t_min_ < 1e-12) {
        view_t_max_ = view_t_min_ + 1.0;
    }
    if (view_y_max_ - view_y_min_ < 1e-12) {
        view_y_max_ = view_y_min_ + 1.0;
    }

    const double t_pad = 0.05 * (view_t_max_ - view_t_min_);
    const double y_pad = 0.10 * (view_y_max_ - view_y_min_);
    view_t_min_ -= t_pad;
    view_t_max_ += t_pad;
    view_y_min_ -= y_pad;
    view_y_max_ += y_pad;
}

QPointF SignalPlotWidget::data_to_pixel(double t, double y) const {
    const double w = static_cast<double>(width() - 2 * kMargin);
    const double h = static_cast<double>(height() - 2 * kMargin);
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double px = static_cast<double>(kMargin) + (t - view_t_min_) / dt * w;
    const double py = static_cast<double>(kMargin) + (1.0 - (y - view_y_min_) / dy) * h;
    return {px, py};
}

QPointF SignalPlotWidget::pixel_to_data(const QPointF& pixel) const {
    const double w = static_cast<double>(width() - 2 * kMargin);
    const double h = static_cast<double>(height() - 2 * kMargin);
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double t = view_t_min_ + (pixel.x() - static_cast<double>(kMargin)) / w * dt;
    const double y = view_y_min_ + (1.0 - (pixel.y() - static_cast<double>(kMargin)) / h) * dy;
    return {t, y};
}

bool SignalPlotWidget::find_handle_near(const QPointF& pixel, std::size_t& out_index) const {
    if (signal_ == nullptr || signal_->empty()) {
        return false;
    }

    double best = kPickRadius;
    bool found = false;
    for (std::size_t i = 0; i < signal_->size(); ++i) {
        const auto& sample = signal_->samples()[i];
        const QPointF point = data_to_pixel(sample.t, sample.y);
        const double distance = std::hypot(point.x() - pixel.x(), point.y() - pixel.y());
        if (distance <= best) {
            best = distance;
            out_index = i;
            found = true;
        }
    }
    return found;
}

void SignalPlotWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void SignalPlotWidget::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    clear_hovered_index();
    update();
}

void SignalPlotWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, kBgTop);
    background.setColorAt(0.45, kBgMid);
    background.setColorAt(1.0, kBgBot);
    painter.fillRect(rect(), background);

    const QRectF plot_rect(kMargin, kMargin,
                           width() - 2 * kMargin,
                           height() - 2 * kMargin);

    painter.setPen(Qt::NoPen);
    painter.setBrush(kPlotCard);
    painter.drawRoundedRect(plot_rect.adjusted(-12.0, -12.0, 12.0, 12.0), 18.0, 18.0);

    painter.setPen(QPen(kPlotFrame, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(plot_rect, 14.0, 14.0);

    painter.setPen(QPen(kGrid, 1.0, Qt::DashLine));
    constexpr int kDivs = 8;
    for (int i = 1; i < kDivs; ++i) {
        const double frac = static_cast<double>(i) / kDivs;
        const double x = plot_rect.left() + frac * plot_rect.width();
        const double y = plot_rect.top() + frac * plot_rect.height();
        painter.drawLine(QPointF(x, plot_rect.top()), QPointF(x, plot_rect.bottom()));
        painter.drawLine(QPointF(plot_rect.left(), y), QPointF(plot_rect.right(), y));
    }

    painter.setPen(kText);
    QFont font = painter.font();
    font.setPointSizeF(8.5);
    painter.setFont(font);
    QFontMetrics metrics(font);
    QString label;
    for (int i = 0; i <= kDivs; ++i) {
        const double frac = static_cast<double>(i) / kDivs;
        const double t = view_t_min_ + frac * (view_t_max_ - view_t_min_);
        const double y = view_y_max_ - frac * (view_y_max_ - view_y_min_);
        format_axis_label(t, label);
        painter.drawText(QPointF(plot_rect.left() + frac * plot_rect.width() - metrics.horizontalAdvance(label) / 2.0,
                                 plot_rect.bottom() + metrics.ascent() + 4),
                         label);
        format_axis_label(y, label);
        painter.drawText(QPointF(plot_rect.left() - metrics.horizontalAdvance(label) - 6,
                                 plot_rect.top() + frac * plot_rect.height() + metrics.ascent() / 2.0),
                         label);
    }

    painter.drawText(QPointF(plot_rect.center().x() - metrics.horizontalAdvance("time [s]") / 2.0,
                             static_cast<double>(height() - 6)),
                     "time [s]");
    painter.save();
    painter.translate(14, plot_rect.center().y());
    painter.rotate(-90);
    painter.drawText(QPointF(-metrics.horizontalAdvance("amplitude") / 2.0, 0), "amplitude");
    painter.restore();

    if (signal_ == nullptr || signal_->empty()) {
        QFont placeholder_font = painter.font();
        placeholder_font.setPointSizeF(12.0);
        placeholder_font.setItalic(true);
        painter.setFont(placeholder_font);
        painter.setPen(kPlaceholder);
        painter.drawText(plot_rect.adjusted(0.0, -18.0, 0.0, 0.0), Qt::AlignCenter,
                         QStringLiteral("Load or drop a CSV to start shaping signals"));
        painter.setFont(font);
        draw_badge(painter,
                   QPointF(plot_rect.left() + 18.0, plot_rect.top() + 18.0),
                   QStringLiteral("Interactive workspace"),
                   QColor(20, 30, 42, 220),
                   QColor(93, 211, 196, 110),
                   QColor(226, 235, 244));
        return;
    }

    const auto& samples = signal_->samples();
    const QString signal_name = QString::fromStdString(signal_->name());
    const QString interpolation = signal_->interpolation() == Signal::InterpolationMode::Step
        ? QStringLiteral("Step interpolation")
        : QStringLiteral("Linear interpolation");
    draw_badge(painter,
               QPointF(plot_rect.left() + 18.0, plot_rect.top() + 18.0),
               signal_name,
               QColor(18, 28, 39, 226),
               QColor(93, 211, 196, 110),
               QColor(242, 246, 250));
    draw_badge(painter,
               QPointF(plot_rect.left() + 18.0, plot_rect.top() + 52.0),
               QStringLiteral("%1 | %2 samples").arg(interpolation).arg(samples.size()),
               QColor(18, 28, 39, 214),
               QColor(244, 166, 74, 120),
               QColor(223, 231, 239));
    draw_badge(painter,
               QPointF(plot_rect.left() + 18.0, plot_rect.bottom() - 36.0),
               QStringLiteral("Drag points | Double-click to add | Shift+drag to brush"),
               QColor(18, 28, 39, 205),
               QColor(116, 139, 162, 80),
               QColor(183, 197, 211));

    QPainterPath path;
    QPainterPath fill;
    const QPointF first = data_to_pixel(samples.front().t, samples.front().y);
    path.moveTo(first);
    fill.moveTo(QPointF(first.x(), plot_rect.bottom()));
    fill.lineTo(first);
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const QPointF prev_point = data_to_pixel(samples[i - 1].t, samples[i - 1].y);
        const QPointF point = data_to_pixel(samples[i].t, samples[i].y);
        if (signal_->interpolation() == Signal::InterpolationMode::Step) {
            const QPointF corner(point.x(), prev_point.y());
            path.lineTo(corner);
            path.lineTo(point);
            fill.lineTo(corner);
            fill.lineTo(point);
        } else {
            path.lineTo(point);
            fill.lineTo(point);
        }
    }
    fill.lineTo(QPointF(data_to_pixel(samples.back().t, samples.back().y).x(), plot_rect.bottom()));
    fill.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(kCurveFill);
    painter.drawPath(fill);

    painter.setPen(QPen(kCurve, 2.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    if (pinned_data_point_visible_) {
        const QPointF pinned_pixel = data_to_pixel(pinned_data_point_.x(), pinned_data_point_.y());
        painter.setPen(QPen(kCrosshair, 1.0, Qt::DashLine));
        painter.drawLine(QPointF(plot_rect.left(), pinned_pixel.y()), QPointF(plot_rect.right(), pinned_pixel.y()));
        painter.drawLine(QPointF(pinned_pixel.x(), plot_rect.top()), QPointF(pinned_pixel.x(), plot_rect.bottom()));
        painter.setPen(QPen(kPinnedMarker, 1.5, Qt::DashLine));
        painter.drawLine(QPointF(pinned_pixel.x() - 8.0, pinned_pixel.y()), QPointF(pinned_pixel.x() + 8.0, pinned_pixel.y()));
        painter.drawLine(QPointF(pinned_pixel.x(), pinned_pixel.y() - 8.0), QPointF(pinned_pixel.x(), pinned_pixel.y() + 8.0));
        draw_coordinate_bubble(painter, plot_rect, pinned_pixel,
                               format_coordinates(pinned_data_point_.x(), pinned_data_point_.y()));
    }

    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto& sample = samples[i];
        const QPointF point = data_to_pixel(sample.t, sample.y);
        const bool selected = has_selection() && selected_index_ == i;
        const bool hovered = has_hovered_handle() && hovered_index_ == i;
        painter.setBrush(selected ? kSelectedHandle : kHandle);
        painter.setPen(QPen(selected ? kSelectedHandleEdge : kHandleEdge,
                            (selected || hovered) ? 1.8 : 1.0));
        const double radius = selected ? kSelectedHandleRadius : (hovered ? kSelectedHandleRadius - 1.0 : kHandleRadius);
        painter.drawEllipse(point, radius, radius);
    }

    if (has_hovered_handle()) {
        const auto& sample = samples[hovered_index_];
        const QPointF hovered_pixel = data_to_pixel(sample.t, sample.y);
        painter.setPen(QPen(kCrosshair, 1.0, Qt::DashLine));
        painter.drawLine(QPointF(plot_rect.left(), hovered_pixel.y()), QPointF(plot_rect.right(), hovered_pixel.y()));
        painter.drawLine(QPointF(hovered_pixel.x(), plot_rect.top()), QPointF(hovered_pixel.x(), plot_rect.bottom()));
        draw_coordinate_bubble(painter, plot_rect, hovered_pixel,
                               format_coordinates(sample.t, sample.y));
    }
}

void SignalPlotWidget::mousePressEvent(QMouseEvent* event) {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }

    const QPointF data_point = pixel_to_data(event->position());
    std::size_t index = 0;
    const bool handle_hit = find_handle_near(event->position(), index);

    if (event->button() == Qt::LeftButton) {
        pinned_data_point_ = data_point;
        pinned_data_point_visible_ = point_inside_plot(event->position(), width(), height());

        if (handle_hit) {
            emit editStarted();
            set_selected_index(index);
            set_hovered_index(index);
            drag_mode_ = DragMode::MoveSample;
            drag_index_ = index;
            update();
            return;
        }

        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            emit editStarted();
            clear_selection();
            drag_mode_ = DragMode::GaussianBrush;
            drag_start_data_ = data_point;
            update();
            return;
        }

        clear_selection();
        update();
        return;
    }

    if (event->button() == Qt::RightButton && handle_hit) {
        set_selected_index(index);
        set_hovered_index(index);
        update();
    }
}

void SignalPlotWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF data_point = pixel_to_data(event->position());
    emit cursorMoved(data_point.x(), data_point.y());

    if (signal_ == nullptr) {
        clear_hovered_index();
        update();
        return;
    }

    if (drag_mode_ == DragMode::MoveSample) {
        if (!event->buttons().testFlag(Qt::LeftButton)) {
            drag_mode_ = DragMode::None;
            emit signalChanged();
            update();
            return;
        }

        drag_index_ = signal_->move_sample(drag_index_, data_point.x(), data_point.y());
        set_selected_index(drag_index_);
        set_hovered_index(drag_index_);
        pinned_data_point_ = data_point;
        pinned_data_point_visible_ = true;
        emit signalChanged();
        update();
        return;
    }

    if (drag_mode_ == DragMode::GaussianBrush) {
        if (!event->buttons().testFlag(Qt::LeftButton)) {
            drag_mode_ = DragMode::None;
            emit signalChanged();
            update();
            return;
        }

        const double delta_y = data_point.y() - drag_start_data_.y();
        const double sigma = std::max(1e-6, 0.05 * (view_t_max_ - view_t_min_));
        signal_->apply_gaussian_brush(drag_start_data_.x(), delta_y, sigma);
        drag_start_data_ = data_point;
        emit signalChanged();
        update();
        return;
    }

    std::size_t hovered = 0;
    if (find_handle_near(event->position(), hovered)) {
        set_hovered_index(hovered);
    } else {
        clear_hovered_index();
    }

    update();
}

void SignalPlotWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (drag_mode_ == DragMode::None) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        drag_mode_ = DragMode::None;
        emit signalChanged();
        update();
    }
}

void SignalPlotWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (signal_ == nullptr || !point_inside_plot(event->position(), width(), height())) {
        return;
    }

    emit editStarted();
    const QPointF data_point = pixel_to_data(event->position());
    const std::size_t new_index = signal_->insert_sample(data_point.x(), data_point.y());
    pinned_data_point_ = data_point;
    pinned_data_point_visible_ = true;
    set_selected_index(new_index);
    set_hovered_index(new_index);
    emit signalChanged();
    auto_fit_view();
    update();
}

void SignalPlotWidget::contextMenuEvent(QContextMenuEvent* event) {
    if (signal_ == nullptr || !point_inside_plot(QPointF(event->pos()), width(), height())) {
        return;
    }

    last_context_data_ = pixel_to_data(QPointF(event->pos()));
    pinned_data_point_ = last_context_data_;
    pinned_data_point_visible_ = true;

    std::size_t hit_index = 0;
    const bool handle_hit = find_handle_near(QPointF(event->pos()), hit_index);
    if (handle_hit) {
        set_selected_index(hit_index);
        set_hovered_index(hit_index);
    }

    QMenu menu(this);
    QAction* add_action = menu.addAction(QStringLiteral("Add waypoint here"));
    QAction* remove_action = menu.addAction(QStringLiteral("Remove selected waypoint"));
    remove_action->setEnabled(handle_hit || has_selection());

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == nullptr) {
        update();
        return;
    }

    if (chosen == add_action) {
        emit editStarted();
        const std::size_t new_index = signal_->insert_sample(last_context_data_.x(), last_context_data_.y());
        set_selected_index(new_index);
        set_hovered_index(new_index);
        emit signalChanged();
        auto_fit_view();
        update();
        return;
    }

    if (chosen == remove_action && has_selection()) {
        emit editStarted();
        signal_->remove_sample(selected_index_);
        if (signal_->empty()) {
            clear_selection();
            clear_hovered_index();
        } else {
            const std::size_t bounded = std::min(selected_index_, signal_->size() - 1);
            set_selected_index(bounded);
            set_hovered_index(bounded);
        }
        emit signalChanged();
        auto_fit_view();
        update();
    }
}

}  // namespace myprj::signal_editor::adapters::qt
