#pragma once

#include <QPointF>
#include <QWidget>

#include <cstddef>

namespace myprj::signal_editor {
class Signal;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

/**
 * @brief Interactive waveform canvas used for direct manipulation editing.
 *
 * The widget renders the selected signal, supports waypoint dragging, context
 * insertion/removal, and a Gaussian brush interaction. Ownership stays in the
 * calling controller; the plot only keeps a raw pointer to the active signal.
 */
class SignalPlotWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Creates the plot widget with mouse interaction enabled.
     * @param parent Optional owning widget supplied by Qt.
     */
    explicit SignalPlotWidget(QWidget* parent = nullptr);

    /**
     * @brief Binds the plot to a signal instance.
     * @param signal Signal to render and edit, or `nullptr`.
     */
    void set_signal(Signal* signal);

    /**
     * @brief Recomputes the viewport and schedules a repaint.
     */
    void refresh();

    /**
     * @brief Reports whether the user is currently in a drag interaction.
     * @return `true` while a sample or brush gesture is active.
     */
    [[nodiscard]] bool is_drag_active() const noexcept;

signals:
    void editStarted();
    void signalChanged();
    void cursorMoved(double t, double y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    static constexpr int kMargin = 48;
    static constexpr double kHandleRadius = 6.0;
    static constexpr double kSelectedHandleRadius = 8.5;
    static constexpr double kPickRadius = 12.0;

    Signal* signal_{nullptr};
    double view_t_min_{0.0};
    double view_t_max_{1.0};
    double view_y_min_{-1.0};
    double view_y_max_{1.0};

    enum class DragMode { None, MoveSample, GaussianBrush };
    DragMode drag_mode_{DragMode::None};
    std::size_t drag_index_{0};
    std::size_t selected_index_{static_cast<std::size_t>(-1)};
    std::size_t hovered_index_{static_cast<std::size_t>(-1)};
    QPointF drag_start_data_;
    QPointF last_context_data_;
    QPointF pinned_data_point_;
    bool pinned_data_point_visible_{false};

    QPointF data_to_pixel(double t, double y) const;
    QPointF pixel_to_data(const QPointF& pixel) const;
    void auto_fit_view();
    bool find_handle_near(const QPointF& pixel, std::size_t& out_index) const;
    [[nodiscard]] bool has_selection() const noexcept;
    [[nodiscard]] bool has_hovered_handle() const noexcept;
    void set_selected_index(std::size_t index);
    void clear_selection();
    void set_hovered_index(std::size_t index);
    void clear_hovered_index();
};

}  // namespace myprj::signal_editor::adapters::qt
