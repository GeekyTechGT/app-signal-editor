#include "signal_editor/adapters/qt/signal_plot_widget.h"

#include "signal_editor/core/domain/signal.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>

namespace myprj::signal_editor::adapters::qt {

namespace {

constexpr QColor kBgTop      (24, 26, 36);
constexpr QColor kBgBot      (16, 18, 26);
constexpr QColor kAxis       (90, 95, 115);
constexpr QColor kGrid       (45, 50, 70);
constexpr QColor kCurve      (90, 200, 250);
constexpr QColor kCurveFill  (90, 200, 250, 60);
constexpr QColor kHandle     (255, 200, 80);
constexpr QColor kHandleEdge (50, 30, 0);
constexpr QColor kText       (210, 215, 230);
constexpr QColor kPlaceholder(120, 125, 140);

void format_axis_label(double v, QString& out) {
    if (std::fabs(v) >= 1000.0 || (v != 0.0 && std::fabs(v) < 0.01)) {
        out = QString::number(v, 'g', 4);
    } else {
        out = QString::number(v, 'f', 3);
    }
}

}  // namespace

SignalPlotWidget::SignalPlotWidget(QWidget* parent) : QWidget(parent) {
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(::Qt::StrongFocus);
    setMinimumSize(400, 300);
}

void SignalPlotWidget::set_signal(Signal* signal) {
    signal_ = signal;
    drag_mode_ = DragMode::None;
    auto_fit_view();
    update();
}

void SignalPlotWidget::refresh() {
    auto_fit_view();
    update();
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
    const double w  = static_cast<double>(width()  - 2 * kMargin);
    const double h  = static_cast<double>(height() - 2 * kMargin);
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double px = static_cast<double>(kMargin) + (t - view_t_min_) / dt * w;
    const double py = static_cast<double>(kMargin) + (1.0 - (y - view_y_min_) / dy) * h;
    return {px, py};
}

QPointF SignalPlotWidget::pixel_to_data(const QPointF& pixel) const {
    const double w  = static_cast<double>(width()  - 2 * kMargin);
    const double h  = static_cast<double>(height() - 2 * kMargin);
    const double dt = view_t_max_ - view_t_min_;
    const double dy = view_y_max_ - view_y_min_;
    const double t  = view_t_min_ + (pixel.x() - static_cast<double>(kMargin)) / w * dt;
    const double y  = view_y_min_ + (1.0 - (pixel.y() - static_cast<double>(kMargin)) / h) * dy;
    return {t, y};
}

bool SignalPlotWidget::find_handle_near(const QPointF& pixel, std::size_t& out_index) const {
    if (signal_ == nullptr || signal_->empty()) {
        return false;
    }
    double best = kPickRadius;
    bool found = false;
    for (std::size_t i = 0; i < signal_->size(); ++i) {
        const auto& s = signal_->samples()[i];
        const QPointF p = data_to_pixel(s.t, s.y);
        const double d = std::hypot(p.x() - pixel.x(), p.y() - pixel.y());
        if (d <= best) {
            best = d;
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

void SignalPlotWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // --- Background -----------------------------------------------------
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, kBgTop);
    bg.setColorAt(1.0, kBgBot);
    p.fillRect(rect(), bg);

    const QRectF plot_rect(kMargin, kMargin,
                           width() - 2 * kMargin,
                           height() - 2 * kMargin);

    // --- Plot frame -----------------------------------------------------
    p.setPen(QPen(kAxis, 1.0));
    p.setBrush(::Qt::NoBrush);
    p.drawRect(plot_rect);

    // --- Grid -----------------------------------------------------------
    p.setPen(QPen(kGrid, 1.0, ::Qt::DashLine));
    constexpr int kDivs = 8;
    for (int i = 1; i < kDivs; ++i) {
        const double f = static_cast<double>(i) / kDivs;
        const double x = plot_rect.left() + f * plot_rect.width();
        const double y = plot_rect.top()  + f * plot_rect.height();
        p.drawLine(QPointF(x, plot_rect.top()), QPointF(x, plot_rect.bottom()));
        p.drawLine(QPointF(plot_rect.left(), y), QPointF(plot_rect.right(), y));
    }

    // --- Axis labels ----------------------------------------------------
    p.setPen(kText);
    QFont f = p.font();
    f.setPointSizeF(8.5);
    p.setFont(f);
    QFontMetrics fm(f);
    QString label;
    for (int i = 0; i <= kDivs; ++i) {
        const double frac = static_cast<double>(i) / kDivs;
        const double t    = view_t_min_ + frac * (view_t_max_ - view_t_min_);
        const double y    = view_y_max_ - frac * (view_y_max_ - view_y_min_);
        format_axis_label(t, label);
        p.drawText(QPointF(plot_rect.left() + frac * plot_rect.width() - fm.horizontalAdvance(label) / 2.0,
                           plot_rect.bottom() + fm.ascent() + 4),
                   label);
        format_axis_label(y, label);
        p.drawText(QPointF(plot_rect.left() - fm.horizontalAdvance(label) - 6,
                           plot_rect.top() + frac * plot_rect.height() + fm.ascent() / 2.0),
                   label);
    }

    // --- Axis titles ----------------------------------------------------
    p.drawText(QPointF(plot_rect.center().x() - fm.horizontalAdvance("time [s]") / 2.0,
                       static_cast<double>(height() - 6)),
               "time [s]");
    p.save();
    p.translate(14, plot_rect.center().y());
    p.rotate(-90);
    p.drawText(QPointF(-fm.horizontalAdvance("amplitude") / 2.0, 0), "amplitude");
    p.restore();

    if (signal_ == nullptr || signal_->empty()) {
        p.setPen(kPlaceholder);
        QFont placeholder_font = p.font();
        placeholder_font.setPointSizeF(11.0);
        placeholder_font.setItalic(true);
        p.setFont(placeholder_font);
        p.drawText(plot_rect, ::Qt::AlignCenter,
                   QStringLiteral("Drop a CSV here or use File ▸ Open…"));
        return;
    }

    // --- Curve ----------------------------------------------------------
    QPainterPath path;
    QPainterPath fill;
    const auto& samples = signal_->samples();
    const QPointF first = data_to_pixel(samples.front().t, samples.front().y);
    path.moveTo(first);
    fill.moveTo(QPointF(first.x(), plot_rect.bottom()));
    fill.lineTo(first);
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const QPointF q = data_to_pixel(samples[i].t, samples[i].y);
        path.lineTo(q);
        fill.lineTo(q);
    }
    fill.lineTo(QPointF(data_to_pixel(samples.back().t, samples.back().y).x(),
                        plot_rect.bottom()));
    fill.closeSubpath();
    p.setPen(::Qt::NoPen);
    p.setBrush(kCurveFill);
    p.drawPath(fill);

    p.setPen(QPen(kCurve, 2.0));
    p.setBrush(::Qt::NoBrush);
    p.drawPath(path);

    // --- Sample handles -------------------------------------------------
    p.setBrush(kHandle);
    p.setPen(QPen(kHandleEdge, 1.0));
    for (const auto& s : samples) {
        const QPointF q = data_to_pixel(s.t, s.y);
        p.drawEllipse(q, kHandleRadius, kHandleRadius);
    }
}

void SignalPlotWidget::mousePressEvent(QMouseEvent* event) {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }
    if (event->button() == ::Qt::LeftButton) {
        std::size_t idx = 0;
        if (find_handle_near(event->position(), idx)) {
            drag_mode_  = DragMode::MoveSample;
            drag_index_ = idx;
        } else if (event->modifiers().testFlag(::Qt::ShiftModifier)) {
            drag_mode_       = DragMode::GaussianBrush;
            drag_start_data_ = pixel_to_data(event->position());
        }
    }
}

void SignalPlotWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF data_pt = pixel_to_data(event->position());
    emit cursorMoved(data_pt.x(), data_pt.y());

    if (signal_ == nullptr || drag_mode_ == DragMode::None) {
        return;
    }
    if (drag_mode_ == DragMode::MoveSample) {
        signal_->set_sample_value(drag_index_, data_pt.y());
        emit signalChanged();
        update();
    } else if (drag_mode_ == DragMode::GaussianBrush) {
        const double delta_y = data_pt.y() - drag_start_data_.y();
        const double sigma   = std::max(1e-6, 0.05 * (view_t_max_ - view_t_min_));
        signal_->apply_gaussian_brush(drag_start_data_.x(), delta_y, sigma);
        drag_start_data_ = data_pt;
        emit signalChanged();
        update();
    }
}

void SignalPlotWidget::mouseReleaseEvent(QMouseEvent* /*event*/) {
    if (drag_mode_ != DragMode::None) {
        drag_mode_ = DragMode::None;
        emit signalChanged();
    }
}

void SignalPlotWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (signal_ == nullptr) {
        return;
    }
    const QPointF data_pt = pixel_to_data(event->position());
    signal_->insert_sample(data_pt.x(), data_pt.y());
    emit signalChanged();
    auto_fit_view();
    update();
}

void SignalPlotWidget::contextMenuEvent(QContextMenuEvent* event) {
    if (signal_ == nullptr || signal_->empty()) {
        return;
    }
    std::size_t idx = 0;
    if (find_handle_near(QPointF(event->pos()), idx)) {
        signal_->remove_sample(idx);
        emit signalChanged();
        auto_fit_view();
        update();
    }
}

}  // namespace myprj::signal_editor::adapters::qt
