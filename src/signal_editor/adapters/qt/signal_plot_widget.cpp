#include "signal_editor/adapters/qt/signal_plot_widget.h"

#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QAction>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QInputDialog>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <vector>

namespace signal_editor::adapters::qt {

#define tr qt_signal_plot_widget_tr

namespace {
QString qt_signal_plot_widget_tr(const char* source_text,
                                 const char* disambiguation = nullptr,
                                 int n = -1) {
    return QCoreApplication::translate("SignalPlotWidget", source_text,
                                       disambiguation, n);
}

constexpr int kPlotMargin = 48;
constexpr double kMinimumVisibleTimeSpan = 1e-6;
constexpr double kHandleInset = 10.0;

struct PlotColors {
    QColor bg_top;
    QColor bg_mid;
    QColor bg_bot;
    QColor plot_card;
    QColor plot_frame;
    QColor axis_text;
    QColor grid;
    QColor curve;
    QColor curve_fill;
    QColor handle;
    QColor handle_edge;
    QColor selected_handle;
    QColor selected_handle_edge;
    QColor info_bubble;
    QColor info_border;
    QColor info_text;
    QColor pinned_marker;
    QColor crosshair;
    QColor placeholder;
    QColor badge_fill;
    QColor badge_outline;
    QColor badge_text;
    QColor hint_badge_fill;
    QColor hint_badge_outline;
    QColor hint_badge_text;
};

PlotColors plot_colors_for(const QPalette& palette) {
    const bool is_dark = palette.color(QPalette::Window).lightness() < 128;
    if (is_dark) {
        return {
            QColor(0, 0, 0),
            QColor(6, 10, 16),
            QColor(11, 17, 24),
            QColor(7, 12, 18, 228),
            QColor(120, 143, 167, 92),
            QColor(214, 225, 236),
            QColor(59, 75, 93, 165),
            QColor(102, 227, 210),
            QColor(102, 227, 210, 58),
            QColor(255, 180, 92),
            QColor(82, 51, 8),
            QColor(255, 123, 95),
            QColor(255, 240, 220),
            QColor(13, 20, 29, 236),
            QColor(255, 180, 92),
            QColor(241, 246, 252),
            QColor(255, 214, 145),
            QColor(255, 180, 92, 90),
            QColor(130, 146, 164),
            QColor(18, 28, 39, 226),
            QColor(102, 227, 210, 112),
            QColor(242, 246, 250),
            QColor(18, 28, 39, 210),
            QColor(120, 143, 167, 82),
            QColor(197, 209, 221),
        };
    }

    return {
        QColor(250, 252, 255),
        QColor(243, 247, 251),
        QColor(236, 242, 248),
        QColor(255, 255, 255, 244),
        QColor(151, 171, 191, 120),
        QColor(68, 89, 112),
        QColor(174, 190, 205, 140),
        QColor(20, 126, 160),
        QColor(20, 126, 160, 38),
        QColor(255, 155, 64),
        QColor(120, 82, 24),
        QColor(224, 95, 66),
        QColor(255, 239, 230),
        QColor(255, 255, 255, 238),
        QColor(255, 155, 64),
        QColor(34, 49, 65),
        QColor(255, 155, 64),
        QColor(255, 155, 64, 72),
        QColor(107, 124, 144),
        QColor(255, 255, 255, 232),
        QColor(20, 126, 160, 78),
        QColor(34, 49, 65),
        QColor(250, 251, 253, 230),
        QColor(151, 171, 191, 90),
        QColor(90, 108, 126),
    };
}

void format_axis_label(double v, QString& out) {
    if (std::fabs(v) >= 1000.0 || (v != 0.0 && std::fabs(v) < 0.01)) {
        out = QString::number(v, 'g', 4);
    } else {
        out = QString::number(v, 'f', 3);
    }
}

QString format_signal_value(const Signal* signal, double y) {
    if (signal != nullptr && signal->is_enumerated()) {
        const std::string label = signal->label_for_value(y);
        if (!label.empty()) {
            return QCoreApplication::translate("SignalPlotWidget", "%1 (%2)")
                .arg(QString::fromStdString(label))
                .arg(y, 0, 'f', 4);
        }
    }
    return QString::number(y, 'f', 4);
}

QString format_coordinates(const Signal* signal, double t, double y) {
    return QCoreApplication::translate("SignalPlotWidget", "x=%1  y=%2")
        .arg(t, 0, 'f', 4)
        .arg(format_signal_value(signal, y));
}

bool point_inside_plot(const QPointF& point, int width, int height) {
    return point.x() >= static_cast<double>(kPlotMargin) &&
           point.x() <= static_cast<double>(width - kPlotMargin) &&
           point.y() >= static_cast<double>(kPlotMargin) &&
           point.y() <= static_cast<double>(height - kPlotMargin);
}

QRectF plot_content_rect(const QRectF& plot_rect) {
    return plot_rect.adjusted(kHandleInset, kHandleInset, -kHandleInset, -kHandleInset);
}

bool point_inside_content(const QPointF& point, const QRectF& content_rect, double radius = 0.0) {
    return content_rect.adjusted(-radius, -radius, radius, radius).contains(point);
}

void draw_coordinate_bubble(QPainter& painter,
                            const QRectF& bounds,
                            const QPointF& anchor,
                            const QString& text,
                            const PlotColors& colors) {
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

    painter.setPen(QPen(colors.info_border, 1.0));
    painter.setBrush(colors.info_bubble);
    painter.drawRoundedRect(bubble, 6.0, 6.0);
    painter.setPen(colors.info_text);
    painter.drawText(bubble, Qt::AlignCenter, text);
}

QRectF layout_badge_rect(const QPointF& top_left,
                         const QString& text,
                         const QFontMetrics& metrics) {
    QRectF badge(metrics.boundingRect(text).adjusted(-10, -5, 10, 5));
    badge.moveTopLeft(top_left);
    return badge;
}

void draw_badge(QPainter& painter,
                const QRectF& badge,
                const QString& text,
                const QColor& fill,
                const QColor& outline,
                const QColor& text_color) {
    painter.setPen(QPen(outline, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(badge, 8.0, 8.0);
    painter.setPen(text_color);
    painter.drawText(badge, Qt::AlignCenter, text);
}

bool contains_index(const std::vector<int>& indices, int index) {
    return std::find(indices.begin(), indices.end(), index) != indices.end();
}

QPainterPath build_signal_path(const Signal& signal,
                               const std::function<QPointF(double, double)>& map_point) {
    QPainterPath path;
    const auto& samples = signal.samples();
    if (samples.empty()) {
        return path;
    }

    path.moveTo(map_point(samples.front().t, samples.front().y));
    for (std::size_t index = 1; index < samples.size(); ++index) {
        const QPointF prev_point = map_point(samples[index - 1].t, samples[index - 1].y);
        const QPointF point = map_point(samples[index].t, samples[index].y);
        if (signal.interpolation() == Signal::InterpolationMode::Step) {
            path.lineTo(QPointF(point.x(), prev_point.y()));
        }
        path.lineTo(point);
    }
    return path;
}

QColor signal_stroke_color(const PlotColors& colors, int signal_index, bool active) {
    QColor stroke = colors.curve;
    const int hue_shift = (signal_index * 32) % 160;
    stroke = stroke.toHsv();
    stroke.setHsv((stroke.hue() + hue_shift + 360) % 360,
                  std::clamp(stroke.saturation() + (active ? 35 : -10), 0, 255),
                  std::clamp(stroke.value() + (active ? 20 : -5), 0, 255),
                  active ? 255 : 180);
    return stroke.toRgb();
}

QColor signal_handle_fill(const QColor& stroke, bool active) {
    QColor fill = stroke;
    fill.setAlpha(active ? 245 : 170);
    return fill.lighter(active ? 118 : 110);
}

QColor signal_handle_edge(const QColor& stroke, bool active) {
    QColor edge = stroke.darker(active ? 138 : 122);
    edge.setAlpha(active ? 255 : 190);
    return edge;
}
}  // namespace

SignalPlotWidget::SignalPlotWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(360, 180);
}

void SignalPlotWidget::set_library(const SignalLibrary* library,
                                   int active_signal_index,
                                   const std::vector<int>& visible_signal_indices) {
    library_ = library;
    active_signal_index_ = active_signal_index;
    visible_signal_indices_.clear();
    if (library_ != nullptr) {
        for (int index : visible_signal_indices) {
            if (index >= 0 &&
                index < static_cast<int>(library_->size()) &&
                !contains_index(visible_signal_indices_, index)) {
                visible_signal_indices_.push_back(index);
            }
        }
    }
    if (library_ != nullptr &&
        active_signal_index_ >= 0 &&
        active_signal_index_ < static_cast<int>(library_->size())) {
        signal_ = const_cast<Signal*>(&library_->at(static_cast<std::size_t>(active_signal_index_)));
    } else {
        signal_ = nullptr;
    }
    drag_mode_ = DragMode::None;
    drag_segment_start_index_ = std::numeric_limits<std::size_t>::max();
    hovered_segment_start_index_ = std::numeric_limits<std::size_t>::max();
    clear_selection();
    clear_hovered_index();
    pinned_data_point_visible_ = false;
    if (signal_ == nullptr) {
        title_badge_anchor_ratio_ = {0.04, 0.08};
        meta_badge_anchor_ratio_ = {0.04, 0.18};
    }
    auto_fit_view();
    update();
}

void SignalPlotWidget::set_signal(Signal* signal) {
    library_ = nullptr;
    active_signal_index_ = signal == nullptr ? -1 : 0;
    visible_signal_indices_.clear();
    signal_ = signal;
    drag_mode_ = DragMode::None;
    drag_segment_start_index_ = std::numeric_limits<std::size_t>::max();
    hovered_segment_start_index_ = std::numeric_limits<std::size_t>::max();
    clear_selection();
    clear_hovered_index();
    pinned_data_point_visible_ = false;
    if (signal_ == nullptr) {
        title_badge_anchor_ratio_ = {0.04, 0.08};
        meta_badge_anchor_ratio_ = {0.04, 0.18};
    }
    auto_fit_view();
    update();
}

void SignalPlotWidget::refresh() {
    update_y_view_for_current_time_window();
    update();
}

void SignalPlotWidget::zoom_in_time() {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }
    const double center = 0.5 * (view_t_min_ + view_t_max_);
    const double half_span = std::max(kMinimumVisibleTimeSpan,
                                      0.25 * (view_t_max_ - view_t_min_));
    const bool applied = set_time_view(center - half_span, center + half_span);
    (void)applied;
}

void SignalPlotWidget::zoom_out_time() {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }
    const double center = 0.5 * (view_t_min_ + view_t_max_);
    const double half_span = view_t_max_ - view_t_min_;
    const bool applied = set_time_view(center - half_span, center + half_span);
    (void)applied;
}

void SignalPlotWidget::reset_time_view() {
    auto_fit_view();
    emit timeViewChanged(view_t_min_, view_t_max_);
    update();
}

bool SignalPlotWidget::set_time_view(double t_start, double t_end) {
    if (signal_ == nullptr || signal_->empty()) {
        return false;
    }
    if (!(t_end > t_start)) {
        return false;
    }

    const double signal_t_min = signal_->t_min();
    const double signal_t_max = signal_->t_max();
    const double bounded_start = std::clamp(t_start, signal_t_min, signal_t_max);
    const double bounded_end = std::clamp(t_end, signal_t_min, signal_t_max);
    if (!((bounded_end - bounded_start) > kMinimumVisibleTimeSpan)) {
        return false;
    }

    view_t_min_ = bounded_start;
    view_t_max_ = bounded_end;
    update_y_view_for_current_time_window();
    emit timeViewChanged(view_t_min_, view_t_max_);
    update();
    return true;
}

void SignalPlotWidget::set_navigation_mode(NavigationMode mode) {
    navigation_mode_ = mode;

    if (drag_mode_ == DragMode::PanView || drag_mode_ == DragMode::RectZoom) {
        drag_mode_ = DragMode::None;
    }

    switch (navigation_mode_) {
    case NavigationMode::Edit:
        unsetCursor();
        break;
    case NavigationMode::Pan:
        setCursor(Qt::OpenHandCursor);
        break;
    case NavigationMode::RectZoom:
        setCursor(Qt::CrossCursor);
        break;
    }

    update();
}

SignalPlotWidget::NavigationMode SignalPlotWidget::navigation_mode() const noexcept {
    return navigation_mode_;
}

std::pair<double, double> SignalPlotWidget::time_view() const noexcept {
    return {view_t_min_, view_t_max_};
}

QRectF SignalPlotWidget::plot_rect() const {
    return QRectF(kMargin, kMargin,
                  width() - 2 * kMargin,
                  height() - 2 * kMargin);
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

bool SignalPlotWidget::has_hovered_segment() const noexcept {
    return signal_ != nullptr &&
           hovered_segment_start_index_ != std::numeric_limits<std::size_t>::max() &&
           hovered_segment_start_index_ + 1 < signal_->size();
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

void SignalPlotWidget::clear_hovered_segment() noexcept {
    hovered_segment_start_index_ = std::numeric_limits<std::size_t>::max();
}

Signal* SignalPlotWidget::active_signal() const noexcept {
    return signal_;
}

bool SignalPlotWidget::is_signal_visible(int signal_index) const noexcept {
    if (library_ == nullptr || signal_index < 0 ||
        signal_index >= static_cast<int>(library_->size())) {
        return false;
    }
    return contains_index(visible_signal_indices_, signal_index);
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

    double y_min = signal_->y_min();
    double y_max = signal_->y_max();
    if (library_ != nullptr) {
        for (int signal_index : visible_signal_indices_) {
            if (signal_index < 0 ||
                signal_index >= static_cast<int>(library_->size())) {
                continue;
            }
            const auto& candidate = library_->at(static_cast<std::size_t>(signal_index));
            if (candidate.empty()) {
                continue;
            }
            y_min = std::min(y_min, candidate.y_min());
            y_max = std::max(y_max, candidate.y_max());
            view_t_min_ = std::min(view_t_min_, candidate.t_min());
            view_t_max_ = std::max(view_t_max_, candidate.t_max());
        }
    }
    view_y_min_ = y_min;
    view_y_max_ = y_max;

    if (view_t_max_ - view_t_min_ < 1e-12) {
        view_t_max_ = view_t_min_ + 1.0;
    }
    if (view_y_max_ - view_y_min_ < 1e-12) {
        view_y_max_ = view_y_min_ + 1.0;
    }

    const double t_pad = 0.05 * (view_t_max_ - view_t_min_);
    view_t_min_ -= t_pad;
    view_t_max_ += t_pad;
    update_y_view_for_current_time_window();
}

void SignalPlotWidget::update_y_view_for_current_time_window() {
    if (signal_ == nullptr || signal_->empty()) {
        view_y_min_ = -1.0;
        view_y_max_ = 1.0;
        return;
    }

    bool have_values = false;
    double y_min = 0.0;
    double y_max = 0.0;
    const auto accumulate_signal = [&](const Signal& candidate) {
        if (candidate.empty()) {
            return;
        }
        double local_min = candidate.interpolate(view_t_min_);
        double local_max = local_min;
        for (const auto& sample : candidate.samples()) {
            if (sample.t < view_t_min_ || sample.t > view_t_max_) {
                continue;
            }
            local_min = std::min(local_min, sample.y);
            local_max = std::max(local_max, sample.y);
        }
        const double y_at_end = candidate.interpolate(view_t_max_);
        local_min = std::min(local_min, y_at_end);
        local_max = std::max(local_max, y_at_end);
        if (!have_values) {
            y_min = local_min;
            y_max = local_max;
            have_values = true;
        } else {
            y_min = std::min(y_min, local_min);
            y_max = std::max(y_max, local_max);
        }
    };

    if (library_ != nullptr && !visible_signal_indices_.empty()) {
        for (int signal_index : visible_signal_indices_) {
            if (signal_index < 0 ||
                signal_index >= static_cast<int>(library_->size())) {
                continue;
            }
            accumulate_signal(library_->at(static_cast<std::size_t>(signal_index)));
        }
    }
    if (!have_values) {
        accumulate_signal(*signal_);
    }

    if (std::fabs(y_max - y_min) < 1e-12) {
        y_min -= 0.5;
        y_max += 0.5;
    }

    const double y_pad = 0.10 * (y_max - y_min);
    view_y_min_ = y_min - y_pad;
    view_y_max_ = y_max + y_pad;
    if (std::fabs(view_y_max_ - view_y_min_) < 1e-12) {
        view_y_min_ -= 0.5;
        view_y_max_ += 0.5;
    }
}

void SignalPlotWidget::clamp_badge_anchors() {
    title_badge_anchor_ratio_.setX(std::clamp(title_badge_anchor_ratio_.x(), 0.02, 0.80));
    title_badge_anchor_ratio_.setY(std::clamp(title_badge_anchor_ratio_.y(), 0.02, 0.80));
    meta_badge_anchor_ratio_.setX(std::clamp(meta_badge_anchor_ratio_.x(), 0.02, 0.80));
    meta_badge_anchor_ratio_.setY(std::clamp(meta_badge_anchor_ratio_.y(), 0.02, 0.88));
}

void SignalPlotWidget::update_badge_rects(const QString& signal_name,
                                          const QString& interpolation,
                                          const QRectF& plot_bounds,
                                          const QFontMetrics& metrics) {
    clamp_badge_anchors();
    title_badge_rect_ = layout_badge_rect(
        QPointF(plot_bounds.left() + title_badge_anchor_ratio_.x() * plot_bounds.width(),
                plot_bounds.top() + title_badge_anchor_ratio_.y() * plot_bounds.height()),
        signal_name,
        metrics);
    meta_badge_rect_ = layout_badge_rect(
        QPointF(plot_bounds.left() + meta_badge_anchor_ratio_.x() * plot_bounds.width(),
                plot_bounds.top() + meta_badge_anchor_ratio_.y() * plot_bounds.height()),
        interpolation,
        metrics);
}

QPointF SignalPlotWidget::data_to_pixel(double t, double y) const {
    const QRectF bounds = plot_content_rect(plot_rect());
    const double w = bounds.width();
    const double h = bounds.height();
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double px = bounds.left() + (t - view_t_min_) / dt * w;
    const double py = bounds.top() + (1.0 - (y - view_y_min_) / dy) * h;
    return {px, py};
}

QPointF SignalPlotWidget::pixel_to_data(const QPointF& pixel) const {
    const QRectF bounds = plot_content_rect(plot_rect());
    const double w = bounds.width();
    const double h = bounds.height();
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double clamped_x = std::clamp(pixel.x(), bounds.left(), bounds.right());
    const double clamped_y = std::clamp(pixel.y(), bounds.top(), bounds.bottom());
    const double t = view_t_min_ + (clamped_x - bounds.left()) / w * dt;
    const double y = view_y_min_ + (1.0 - (clamped_y - bounds.top()) / h) * dy;
    return {t, y};
}

bool SignalPlotWidget::find_handle_near(const QPointF& pixel, std::size_t& out_index) const {
    if (signal_ == nullptr || signal_->empty()) {
        return false;
    }

    const QRectF content_rect = plot_content_rect(plot_rect());
    double best = kPickRadius;
    bool found = false;
    for (std::size_t i = 0; i < signal_->size(); ++i) {
        const auto& sample = signal_->samples()[i];
        const QPointF point = data_to_pixel(sample.t, sample.y);
        if (!point_inside_content(point, content_rect, kPickRadius)) {
            continue;
        }
        const double distance = std::hypot(point.x() - pixel.x(), point.y() - pixel.y());
        if (distance <= best) {
            best = distance;
            out_index = i;
            found = true;
        }
    }
    return found;
}

bool SignalPlotWidget::find_segment_near(const QPointF& pixel, std::size_t& out_start_index) const {
    if (signal_ == nullptr ||
        signal_->size() < 2 ||
        signal_->interpolation() == Signal::InterpolationMode::Step) {
        return false;
    }

    const QRectF content_rect = plot_content_rect(plot_rect());
    double best_distance = kPickRadius;
    bool found = false;
    for (std::size_t index = 0; index + 1 < signal_->size(); ++index) {
        const QPointF a = data_to_pixel(signal_->samples()[index].t, signal_->samples()[index].y);
        const QPointF b = data_to_pixel(signal_->samples()[index + 1].t, signal_->samples()[index + 1].y);
        if (!point_inside_content(a, content_rect, kPickRadius) &&
            !point_inside_content(b, content_rect, kPickRadius)) {
            continue;
        }

        const QLineF line(a, b);
        const double line_length_sq = line.dx() * line.dx() + line.dy() * line.dy();
        if (line_length_sq <= 1e-12) {
            continue;
        }

        const QPointF ap = pixel - a;
        const QPointF ab = b - a;
        const double projection = std::clamp((ap.x() * ab.x() + ap.y() * ab.y()) / line_length_sq, 0.0, 1.0);
        const QPointF closest = a + projection * ab;
        const double distance = std::hypot(closest.x() - pixel.x(), closest.y() - pixel.y());
        if (distance <= best_distance) {
            best_distance = distance;
            out_start_index = index;
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
    clear_hovered_segment();
    update();
}

void SignalPlotWidget::wheelEvent(QWheelEvent* event) {
    if (signal_ == nullptr || signal_->empty()) {
        QWidget::wheelEvent(event);
        return;
    }

    if (event->angleDelta().y() > 0) {
        zoom_in_time();
    } else if (event->angleDelta().y() < 0) {
        zoom_out_time();
    }
    event->accept();
}

void SignalPlotWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const PlotColors colors = plot_colors_for(palette());

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, colors.bg_top);
    background.setColorAt(0.45, colors.bg_mid);
    background.setColorAt(1.0, colors.bg_bot);
    painter.fillRect(rect(), background);

    const QRectF plot_rect = this->plot_rect();

    painter.setPen(Qt::NoPen);
    painter.setBrush(colors.plot_card);
    painter.drawRoundedRect(plot_rect.adjusted(-12.0, -12.0, 12.0, 12.0), 18.0, 18.0);

    painter.setPen(QPen(colors.plot_frame, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(plot_rect, 14.0, 14.0);

    painter.setPen(QPen(colors.grid, 1.0, Qt::DashLine));
    constexpr int kDivs = 8;
    for (int i = 1; i < kDivs; ++i) {
        const double frac = static_cast<double>(i) / kDivs;
        const double x = plot_rect.left() + frac * plot_rect.width();
        painter.drawLine(QPointF(x, plot_rect.top()), QPointF(x, plot_rect.bottom()));
    }
    if (signal_ != nullptr && signal_->is_enumerated()) {
        for (const auto& entry : signal_->enumeration()) {
            const double y = data_to_pixel(view_t_min_, entry.value).y();
            painter.drawLine(QPointF(plot_rect.left(), y), QPointF(plot_rect.right(), y));
        }
    } else {
        for (int i = 1; i < kDivs; ++i) {
            const double frac = static_cast<double>(i) / kDivs;
            const double y = plot_rect.top() + frac * plot_rect.height();
            painter.drawLine(QPointF(plot_rect.left(), y), QPointF(plot_rect.right(), y));
        }
    }

    painter.setPen(colors.axis_text);
    QFont font = painter.font();
    font.setPointSizeF(8.5);
    painter.setFont(font);
    QFontMetrics metrics(font);
    QString label;
    for (int i = 0; i <= kDivs; ++i) {
        const double frac = static_cast<double>(i) / kDivs;
        const double t = view_t_min_ + frac * (view_t_max_ - view_t_min_);
        format_axis_label(t, label);
        painter.drawText(QPointF(plot_rect.left() + frac * plot_rect.width() - metrics.horizontalAdvance(label) / 2.0,
                                 plot_rect.bottom() + metrics.ascent() + 4),
                         label);
    }

    if (signal_ != nullptr && signal_->is_enumerated()) {
        for (const auto& entry : signal_->enumeration()) {
            label = QString::fromStdString(entry.label);
            const double y = data_to_pixel(view_t_min_, entry.value).y();
            painter.drawText(QPointF(plot_rect.left() - metrics.horizontalAdvance(label) - 6,
                                     y + metrics.ascent() / 2.0),
                             label);
        }
    } else {
        for (int i = 0; i <= kDivs; ++i) {
            const double frac = static_cast<double>(i) / kDivs;
            const double y = view_y_max_ - frac * (view_y_max_ - view_y_min_);
            format_axis_label(y, label);
            painter.drawText(QPointF(plot_rect.left() - metrics.horizontalAdvance(label) - 6,
                                     plot_rect.top() + frac * plot_rect.height() + metrics.ascent() / 2.0),
                             label);
        }
    }

    const QString x_axis_title = tr("time [s]");
    painter.drawText(QPointF(plot_rect.center().x() - metrics.horizontalAdvance(x_axis_title) / 2.0,
                             static_cast<double>(height() - 6)),
                     x_axis_title);
    painter.save();
    painter.translate(14, plot_rect.center().y());
    painter.rotate(-90);
    const QString y_axis_title = signal_ != nullptr && signal_->is_enumerated()
        ? tr("state")
        : tr("amplitude");
    painter.drawText(QPointF(-metrics.horizontalAdvance(y_axis_title) / 2.0, 0), y_axis_title);
    painter.restore();

    if (signal_ == nullptr || signal_->empty()) {
        QFont placeholder_font = painter.font();
        placeholder_font.setPointSizeF(12.0);
        placeholder_font.setItalic(true);
        painter.setFont(placeholder_font);
        painter.setPen(colors.placeholder);
        painter.drawText(plot_rect.adjusted(0.0, -18.0, 0.0, 0.0), Qt::AlignCenter,
                         tr("Load or drop a CSV to start shaping signals"));
        painter.setFont(font);
        const QRectF intro_badge = layout_badge_rect(
            QPointF(plot_rect.left() + 18.0, plot_rect.top() + 18.0),
                   tr("Interactive workspace"),
            metrics);
        draw_badge(painter, intro_badge,
                   tr("Interactive workspace"),
                   colors.badge_fill,
                   colors.badge_outline,
                   colors.badge_text);
        return;
    }

    const QRectF content_rect = plot_content_rect(plot_rect);
    const QRectF clip_rect = content_rect.adjusted(-1.0, -1.0, 1.0, 1.0);
    const bool active_visible =
        library_ == nullptr ? true : is_signal_visible(active_signal_index_);
    const auto* active_samples = active_visible ? &signal_->samples() : nullptr;
    QPainterPath path;
    QPainterPath fill;
    if (active_samples != nullptr && !active_samples->empty()) {
        path = build_signal_path(*signal_,
                                 [this](double t, double y) { return data_to_pixel(t, y); });
        const QPointF first = data_to_pixel(active_samples->front().t, active_samples->front().y);
        fill.moveTo(QPointF(first.x(), content_rect.bottom()));
        fill.lineTo(first);
        for (std::size_t i = 1; i < active_samples->size(); ++i) {
            const QPointF prev_point = data_to_pixel((*active_samples)[i - 1].t,
                                                     (*active_samples)[i - 1].y);
            const QPointF point = data_to_pixel((*active_samples)[i].t,
                                                (*active_samples)[i].y);
            if (signal_->interpolation() == Signal::InterpolationMode::Step) {
                const QPointF corner(point.x(), prev_point.y());
                fill.lineTo(corner);
                fill.lineTo(point);
            } else {
                fill.lineTo(point);
            }
        }
        fill.lineTo(QPointF(data_to_pixel(active_samples->back().t,
                                          active_samples->back().y).x(),
                            content_rect.bottom()));
        fill.closeSubpath();
    }

    painter.save();
    painter.setClipRect(clip_rect);

    if (library_ != nullptr) {
        for (std::size_t signal_index = 0; signal_index < library_->size(); ++signal_index) {
            if (static_cast<int>(signal_index) == active_signal_index_ ||
                !is_signal_visible(static_cast<int>(signal_index))) {
                continue;
            }
            const auto& background_signal = library_->at(signal_index);
            if (background_signal.empty()) {
                continue;
            }

            const auto& background_samples = background_signal.samples();
            const QPainterPath background_path = build_signal_path(
                background_signal,
                [this](double t, double y) { return data_to_pixel(t, y); });
            const QColor background_stroke =
                signal_stroke_color(colors, static_cast<int>(signal_index), false);
            painter.setPen(QPen(background_stroke, 1.8, Qt::SolidLine,
                                Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(background_path);

            const QColor background_handle_fill = signal_handle_fill(background_stroke, false);
            const QColor background_handle_edge = signal_handle_edge(background_stroke, false);
            for (const auto& sample : background_samples) {
                const QPointF point = data_to_pixel(sample.t, sample.y);
                constexpr double kBackgroundRadius = 2.6;
                if (!point_inside_content(point, content_rect, kBackgroundRadius)) {
                    continue;
                }
                painter.setBrush(background_handle_fill);
                painter.setPen(QPen(background_handle_edge, 0.9));
                painter.drawEllipse(point, kBackgroundRadius, kBackgroundRadius);
            }
        }
    }

    QColor active_stroke = signal_stroke_color(colors, active_signal_index_, true);
    QColor active_handle_fill = signal_handle_fill(active_stroke, true);
    QColor active_handle_edge = signal_handle_edge(active_stroke, true);
    if (active_samples != nullptr) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.curve_fill);
        painter.drawPath(fill);

        painter.setPen(QPen(active_stroke, 2.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    if (pinned_data_point_visible_) {
        const QPointF pinned_pixel = data_to_pixel(pinned_data_point_.x(), pinned_data_point_.y());
        painter.setPen(QPen(colors.crosshair, 1.0, Qt::DashLine));
        painter.drawLine(QPointF(plot_rect.left(), pinned_pixel.y()), QPointF(plot_rect.right(), pinned_pixel.y()));
        painter.drawLine(QPointF(pinned_pixel.x(), plot_rect.top()), QPointF(pinned_pixel.x(), plot_rect.bottom()));
        painter.setPen(QPen(colors.pinned_marker, 1.5, Qt::DashLine));
        painter.drawLine(QPointF(pinned_pixel.x() - 8.0, pinned_pixel.y()), QPointF(pinned_pixel.x() + 8.0, pinned_pixel.y()));
        painter.drawLine(QPointF(pinned_pixel.x(), pinned_pixel.y() - 8.0), QPointF(pinned_pixel.x(), pinned_pixel.y() + 8.0));
    }

    if (active_samples != nullptr) {
        for (std::size_t i = 0; i < active_samples->size(); ++i) {
            const auto& sample = (*active_samples)[i];
            const QPointF point = data_to_pixel(sample.t, sample.y);
            const bool selected = has_selection() && selected_index_ == i;
            const bool hovered = has_hovered_handle() && hovered_index_ == i;
            const double radius = selected ? kSelectedHandleRadius
                                           : (hovered ? kSelectedHandleRadius - 1.0
                                                      : kHandleRadius);
            if (!point_inside_content(point, content_rect, radius)) {
                continue;
            }
            painter.setBrush(selected ? colors.selected_handle : active_handle_fill);
            painter.setPen(QPen(selected ? colors.selected_handle_edge : active_handle_edge,
                                (selected || hovered) ? 1.8 : 1.0));
            painter.drawEllipse(point, radius, radius);
        }

        if (has_hovered_segment()) {
            painter.setPen(QPen(colors.selected_handle, 3.0));
            painter.drawLine(data_to_pixel((*active_samples)[hovered_segment_start_index_].t,
                                           (*active_samples)[hovered_segment_start_index_].y),
                             data_to_pixel((*active_samples)[hovered_segment_start_index_ + 1].t,
                                           (*active_samples)[hovered_segment_start_index_ + 1].y));
        }
    }

    if (drag_mode_ == DragMode::RectZoom) {
        const QRectF selection = QRectF(drag_start_pixel_, drag_current_pixel_).normalized()
            .intersected(content_rect);
        if (!selection.isEmpty()) {
            painter.setPen(QPen(colors.crosshair, 1.5, Qt::DashLine));
            painter.setBrush(QColor(colors.crosshair.red(),
                                    colors.crosshair.green(),
                                    colors.crosshair.blue(),
                                    40));
            painter.drawRect(selection);
        }
    }

    painter.restore();

    if (!active_visible) {
        const QRectF hidden_badge = layout_badge_rect(
            QPointF(plot_rect.left() + 18.0, plot_rect.top() + 18.0),
            tr("Active signal hidden"),
            metrics);
        draw_badge(painter,
                   hidden_badge,
                   tr("Active signal hidden"),
                   colors.badge_fill,
                   colors.info_border,
                   colors.badge_text);
    } else {
        const QString signal_name = QString::fromStdString(signal_->name());
        const QString interpolation = signal_->interpolation() == Signal::InterpolationMode::Step
            ? tr("Step interpolation")
            : tr("Linear interpolation");
        const QString meta_text = tr("%1 | %2 samples")
            .arg(interpolation).arg(active_samples == nullptr ? 0 : active_samples->size());
        update_badge_rects(signal_name, meta_text, plot_rect, metrics);
        draw_badge(painter,
                   title_badge_rect_,
                   signal_name,
                   colors.badge_fill,
                   colors.badge_outline,
                   colors.badge_text);
        draw_badge(painter,
                   meta_badge_rect_,
                   meta_text,
                   colors.badge_fill,
                   colors.info_border,
                   colors.badge_text);
        const QRectF hint_badge = layout_badge_rect(
                   QPointF(plot_rect.left() + 18.0, plot_rect.bottom() - 36.0),
                   signal_->is_enumerated()
                       ? tr("Drag states | Double-click to add | Labels drive the Y axis")
                       : tr("Drag points | Double-click to add | Shift+drag to brush"),
                   metrics);
        draw_badge(painter,
                   hint_badge,
                   signal_->is_enumerated()
                       ? tr("Drag states | Double-click to add | Labels drive the Y axis")
                       : tr("Drag points | Double-click to add | Shift+drag to brush"),
                   colors.hint_badge_fill,
                   colors.hint_badge_outline,
                   colors.hint_badge_text);
    }

    if (pinned_data_point_visible_) {
        const QPointF pinned_pixel = data_to_pixel(pinned_data_point_.x(), pinned_data_point_.y());
        draw_coordinate_bubble(painter, plot_rect, pinned_pixel,
                               format_coordinates(signal_, pinned_data_point_.x(), pinned_data_point_.y()),
                               colors);
    }

    if (active_samples != nullptr && has_hovered_handle()) {
        const auto& sample = (*active_samples)[hovered_index_];
        const QPointF hovered_pixel = data_to_pixel(sample.t, sample.y);
        if (!point_inside_content(hovered_pixel, content_rect)) {
            return;
        }
        painter.setPen(QPen(colors.crosshair, 1.0, Qt::DashLine));
        painter.drawLine(QPointF(plot_rect.left(), hovered_pixel.y()), QPointF(plot_rect.right(), hovered_pixel.y()));
        painter.drawLine(QPointF(hovered_pixel.x(), plot_rect.top()), QPointF(hovered_pixel.x(), plot_rect.bottom()));
        draw_coordinate_bubble(painter, plot_rect, hovered_pixel,
                               format_coordinates(signal_, sample.t, sample.y),
                               colors);
    }
}

void SignalPlotWidget::mousePressEvent(QMouseEvent* event) {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }

    const QRectF content_rect = plot_content_rect(plot_rect());
    const QPointF data_point = pixel_to_data(event->position());
    std::size_t index = 0;
    const bool handle_hit = find_handle_near(event->position(), index);
    std::size_t segment_start_index = 0;
    const bool segment_hit = !handle_hit && find_segment_near(event->position(), segment_start_index);

    if (event->button() == Qt::LeftButton) {
        if ((navigation_mode_ == NavigationMode::Pan ||
             navigation_mode_ == NavigationMode::RectZoom) &&
            !point_inside_content(event->position(), content_rect)) {
            return;
        }

        if (navigation_mode_ == NavigationMode::Pan) {
            drag_mode_ = DragMode::PanView;
            drag_start_pixel_ = event->position();
            pan_anchor_t_min_ = view_t_min_;
            pan_anchor_t_max_ = view_t_max_;
            setCursor(Qt::ClosedHandCursor);
            update();
            return;
        }

        if (navigation_mode_ == NavigationMode::RectZoom) {
            drag_mode_ = DragMode::RectZoom;
            drag_start_pixel_ = event->position();
            drag_current_pixel_ = event->position();
            clear_hovered_index();
            update();
            return;
        }

        if (title_badge_rect_.contains(event->position())) {
            drag_mode_ = DragMode::MoveTitleBadge;
            drag_anchor_offset_ = event->position() - title_badge_rect_.topLeft();
            update();
            return;
        }
        if (meta_badge_rect_.contains(event->position())) {
            drag_mode_ = DragMode::MoveMetaBadge;
            drag_anchor_offset_ = event->position() - meta_badge_rect_.topLeft();
            update();
            return;
        }
        pinned_data_point_ = data_point;
        pinned_data_point_visible_ = point_inside_plot(event->position(), width(), height());

        if (handle_hit) {
            emit editStarted();
            set_selected_index(index);
            set_hovered_index(index);
            clear_hovered_segment();
            drag_mode_ = DragMode::MoveSample;
            drag_index_ = index;
            drag_start_data_ = data_point;
            update();
            return;
        }

        if (segment_hit) {
            emit editStarted();
            clear_selection();
            clear_hovered_index();
            hovered_segment_start_index_ = segment_start_index;
            drag_mode_ = DragMode::MoveSegment;
            drag_segment_start_index_ = segment_start_index;
            drag_start_data_ = data_point;
            update();
            return;
        }

        if (event->modifiers().testFlag(Qt::ShiftModifier) && !signal_->is_enumerated()) {
            emit editStarted();
            clear_selection();
            clear_hovered_segment();
            drag_mode_ = DragMode::GaussianBrush;
            drag_start_data_ = data_point;
            update();
            return;
        }

        clear_selection();
        clear_hovered_segment();
        update();
        return;
    }

    if (navigation_mode_ == NavigationMode::Edit &&
        event->button() == Qt::RightButton &&
        handle_hit) {
        set_selected_index(index);
        set_hovered_index(index);
        clear_hovered_segment();
        update();
    }
}

void SignalPlotWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF data_point = pixel_to_data(event->position());
    emit cursorMoved(data_point.x(), data_point.y());

    if (signal_ == nullptr) {
        clear_hovered_index();
        clear_hovered_segment();
        update();
        return;
    }

    if (drag_mode_ == DragMode::PanView) {
        if (!event->buttons().testFlag(Qt::LeftButton)) {
            drag_mode_ = DragMode::None;
            setCursor(Qt::OpenHandCursor);
            update();
            return;
        }

        const QRectF content_rect = plot_content_rect(plot_rect());
        const double dt = pan_anchor_t_max_ - pan_anchor_t_min_;
        if (content_rect.width() > 0.0 && dt > 0.0) {
            const double pixel_dx = event->position().x() - drag_start_pixel_.x();
            const double time_dx = pixel_dx / content_rect.width() * dt;
            double target_t_min = pan_anchor_t_min_ - time_dx;
            double target_t_max = pan_anchor_t_max_ - time_dx;
            const double signal_t_min = signal_->t_min();
            const double signal_t_max = signal_->t_max();
            if (target_t_min < signal_t_min) {
                target_t_max += signal_t_min - target_t_min;
                target_t_min = signal_t_min;
            }
            if (target_t_max > signal_t_max) {
                target_t_min -= target_t_max - signal_t_max;
                target_t_max = signal_t_max;
            }
            (void)set_time_view(target_t_min, target_t_max);
        }
        return;
    }

    if (drag_mode_ == DragMode::RectZoom) {
        if (!event->buttons().testFlag(Qt::LeftButton)) {
            drag_mode_ = DragMode::None;
            update();
            return;
        }

        const QRectF content_rect = plot_content_rect(plot_rect());
        drag_current_pixel_.setX(std::clamp(event->position().x(), content_rect.left(), content_rect.right()));
        drag_current_pixel_.setY(std::clamp(event->position().y(), content_rect.top(), content_rect.bottom()));
        update();
        return;
    }

    if (drag_mode_ == DragMode::MoveTitleBadge || drag_mode_ == DragMode::MoveMetaBadge) {
        const QRectF bounds = plot_rect();
        const QPointF top_left = event->position() - drag_anchor_offset_;
        const QPointF ratio((top_left.x() - bounds.left()) / bounds.width(),
                            (top_left.y() - bounds.top()) / bounds.height());
        if (drag_mode_ == DragMode::MoveTitleBadge) {
            title_badge_anchor_ratio_ = ratio;
        } else {
            meta_badge_anchor_ratio_ = ratio;
        }
        clamp_badge_anchors();
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

        emit sampleMoveRequested(drag_index_, data_point.x(), data_point.y());
        if (signal_ != nullptr) {
            std::size_t refreshed_index = std::min(drag_index_, signal_->size() - 1);
            for (std::size_t index = 0; index < signal_->size(); ++index) {
                const auto& sample = signal_->samples()[index];
                if (std::fabs(sample.t - data_point.x()) < 1e-9 &&
                    std::fabs(sample.y - signal_->snap_to_enumeration(data_point.y())) < 1e-9) {
                    refreshed_index = index;
                    break;
                }
            }
            drag_index_ = refreshed_index;
            set_selected_index(drag_index_);
            set_hovered_index(drag_index_);
        }
        pinned_data_point_ = data_point;
        pinned_data_point_visible_ = true;
        update_y_view_for_current_time_window();
        emit signalChanged();
        update();
        return;
    }

    if (drag_mode_ == DragMode::MoveSegment) {
        if (!event->buttons().testFlag(Qt::LeftButton)) {
            drag_mode_ = DragMode::None;
            emit signalChanged();
            update();
            return;
        }

        const double delta_y = data_point.y() - drag_start_data_.y();
        if (std::fabs(delta_y) > 1e-12) {
            emit segmentMoveRequested(drag_segment_start_index_, delta_y);
            drag_start_data_ = data_point;
            pinned_data_point_ = data_point;
            pinned_data_point_visible_ = true;
            update_y_view_for_current_time_window();
            emit signalChanged();
        }
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
        emit gaussianBrushRequested(drag_start_data_.x(), delta_y, sigma);
        drag_start_data_ = data_point;
        update_y_view_for_current_time_window();
        emit signalChanged();
        update();
        return;
    }

    if (navigation_mode_ != NavigationMode::Edit) {
        clear_hovered_index();
        clear_hovered_segment();
        update();
        return;
    }

    std::size_t hovered = 0;
    if (find_handle_near(event->position(), hovered)) {
        set_hovered_index(hovered);
        clear_hovered_segment();
    } else if (find_segment_near(event->position(), hovered)) {
        clear_hovered_index();
        hovered_segment_start_index_ = hovered;
    } else {
        clear_hovered_index();
        clear_hovered_segment();
    }

    update();
}

void SignalPlotWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (drag_mode_ == DragMode::None) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const DragMode completed_mode = drag_mode_;
        drag_mode_ = DragMode::None;
        if (completed_mode == DragMode::PanView) {
            if (navigation_mode_ == NavigationMode::Pan) {
                setCursor(Qt::OpenHandCursor);
            } else {
                unsetCursor();
            }
            update();
            return;
        }
        if (completed_mode == DragMode::RectZoom) {
            const QRectF content_rect = plot_content_rect(plot_rect());
            const QRectF selection = QRectF(drag_start_pixel_, drag_current_pixel_).normalized()
                .intersected(content_rect);
            constexpr double kMinimumRectZoomWidth = 12.0;
            if (selection.width() >= kMinimumRectZoomWidth) {
                const QPointF start_data = pixel_to_data(selection.topLeft());
                const QPointF end_data = pixel_to_data(selection.bottomRight());
                const double t_start = std::min(start_data.x(), end_data.x());
                const double t_end = std::max(start_data.x(), end_data.x());
                (void)set_time_view(t_start, t_end);
            }
            update();
            return;
        }
        if (signal_ != nullptr &&
            (completed_mode == DragMode::MoveSample ||
             completed_mode == DragMode::MoveSegment ||
             completed_mode == DragMode::GaussianBrush)) {
            emit signalChanged();
        }
        if (completed_mode != DragMode::MoveSegment) {
            clear_hovered_segment();
        }
        update();
    }
}

void SignalPlotWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (navigation_mode_ != NavigationMode::Edit ||
        signal_ == nullptr ||
        !point_inside_plot(event->position(), width(), height())) {
        return;
    }

    emit editStarted();
    const QPointF data_point = pixel_to_data(event->position());
    emit sampleInsertRequested(data_point.x(), data_point.y());
    pinned_data_point_ = data_point;
    pinned_data_point_visible_ = true;
    emit signalChanged();
    update_y_view_for_current_time_window();
    update();
}

void SignalPlotWidget::contextMenuEvent(QContextMenuEvent* event) {
    if (navigation_mode_ != NavigationMode::Edit ||
        signal_ == nullptr ||
        !point_inside_plot(QPointF(event->pos()), width(), height())) {
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
        hovered_segment_start_index_ = std::numeric_limits<std::size_t>::max();
    }

    QMenu menu(this);
    QAction* add_action = menu.addAction(tr("Add waypoint here"));
    QAction* remove_action = menu.addAction(tr("Remove selected waypoint"));
    QAction* offset_all_action = menu.addAction(tr("Apply offset to all samples in file"));
    QAction* offset_sample_action = menu.addAction(tr("Apply offset to selected sample"));
    remove_action->setEnabled(handle_hit || has_selection());
    offset_sample_action->setEnabled(handle_hit || has_selection());

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == nullptr) {
        update();
        return;
    }

    if (chosen == add_action) {
        emit editStarted();
        emit sampleInsertRequested(last_context_data_.x(), last_context_data_.y());
        emit signalChanged();
        update_y_view_for_current_time_window();
        update();
        return;
    }

    if (chosen == remove_action && has_selection()) {
        emit editStarted();
        emit sampleRemoveRequested(selected_index_);
        clear_selection();
        clear_hovered_index();
        emit signalChanged();
        update_y_view_for_current_time_window();
        update();
        return;
    }

    if (chosen == offset_all_action) {
        bool ok = false;
        const double delta = QInputDialog::getDouble(
            this,
            tr("Offset all samples"),
            tr("Offset to add to every sample in the current file"),
            0.0, -1e12, 1e12, 6, &ok);
        if (ok) {
            emit editStarted();
            emit offsetAllRequested(delta);
            emit signalChanged();
            update_y_view_for_current_time_window();
            update();
        }
        return;
    }

    if (chosen == offset_sample_action && has_selection()) {
        bool ok = false;
        const double delta = QInputDialog::getDouble(
            this,
            tr("Offset selected sample"),
            tr("Offset to add to the selected sample"),
            0.0, -1e12, 1e12, 6, &ok);
        if (ok) {
            emit editStarted();
            emit sampleOffsetRequested(selected_index_, delta);
            emit signalChanged();
            update_y_view_for_current_time_window();
            update();
        }
    }
}

#undef tr

}  // namespace signal_editor::adapters::qt
