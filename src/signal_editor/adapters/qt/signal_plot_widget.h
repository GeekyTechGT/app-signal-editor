#pragma once

#include <QPointF>
#include <QRectF>
#include <QWidget>

#include <cstddef>
#include <utility>

namespace signal_editor {
class Signal;
}  // namespace signal_editor

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief Interactive Qt waveform plot used for direct signal editing.
 */

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
     * @brief Interaction modes exposed by the plot toolbar.
     */
    enum class NavigationMode { Edit, Pan, RectZoom };

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
     * @brief Narrows the current visible time range around its centre.
     */
    void zoom_in_time();

    /**
     * @brief Expands the current visible time range around its centre.
     */
    void zoom_out_time();

    /**
     * @brief Restores the full time range of the active signal.
     */
    void reset_time_view();

    /**
     * @brief Applies an explicit visible time range.
     * @param t_start Inclusive lower bound of the visible window.
     * @param t_end Inclusive upper bound of the visible window.
     * @return `true` when the range was accepted and applied.
     */
    [[nodiscard]] bool set_time_view(double t_start, double t_end);

    /**
     * @brief Sets the active interaction mode for the plot canvas.
     * @param mode Navigation/edit mode to activate.
     */
    void set_navigation_mode(NavigationMode mode);

    /**
     * @brief Reports the currently active interaction mode.
     * @return Active navigation/edit mode.
     */
    [[nodiscard]] NavigationMode navigation_mode() const noexcept;

    /**
     * @brief Reports the currently visible time range.
     * @return Pair containing the visible lower and upper bounds.
     */
    [[nodiscard]] std::pair<double, double> time_view() const noexcept;

    /**
     * @brief Reports whether the user is currently in a drag interaction.
     * @return `true` while a sample or brush gesture is active.
     */
    [[nodiscard]] bool is_drag_active() const noexcept;

signals:
    /** @brief Emitted when an interaction should create an undo snapshot. */
    void editStarted();
    /** @brief Emitted after the bound signal changes through direct manipulation. */
    void signalChanged();
    /** @brief Emitted whenever the cursor position over the plot changes. */
    void cursorMoved(double t, double y);
    /** @brief Emitted after the visible time range changes. */
    void timeViewChanged(double t_start, double t_end);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
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

    enum class DragMode {
        None,
        MoveSample,
        GaussianBrush,
        MoveTitleBadge,
        MoveMetaBadge,
        PanView,
        RectZoom
    };
    DragMode drag_mode_{DragMode::None};
    NavigationMode navigation_mode_{NavigationMode::Edit};
    std::size_t drag_index_{0};
    std::size_t selected_index_{static_cast<std::size_t>(-1)};
    std::size_t hovered_index_{static_cast<std::size_t>(-1)};
    QPointF drag_start_data_;
    QPointF drag_anchor_offset_;
    QPointF drag_start_pixel_;
    QPointF drag_current_pixel_;
    QPointF last_context_data_;
    QPointF pinned_data_point_;
    bool pinned_data_point_visible_{false};
    double pan_anchor_t_min_{0.0};
    double pan_anchor_t_max_{1.0};
    QPointF title_badge_anchor_ratio_{0.04, 0.08};
    QPointF meta_badge_anchor_ratio_{0.04, 0.18};
    QRectF title_badge_rect_;
    QRectF meta_badge_rect_;

    /** @brief Converts a data-space point into plot-local pixel coordinates. */
    QPointF data_to_pixel(double t, double y) const;
    /** @brief Converts plot-local pixels back into data-space coordinates. */
    QPointF pixel_to_data(const QPointF& pixel) const;
    /** @brief Returns the drawable content rect inside plot margins. */
    [[nodiscard]] QRectF plot_rect() const;
    /** @brief Fits the viewport to the full signal time range. */
    void auto_fit_view();
    /** @brief Recomputes the Y range for the currently visible time window. */
    void update_y_view_for_current_time_window();
    /** @brief Keeps draggable overlay badges anchored inside the content rect. */
    void clamp_badge_anchors();
    /** @brief Updates overlay badge geometry after a repaint or drag. */
    void update_badge_rects(const QString& signal_name,
                            const QString& interpolation,
                            const QRectF& plot_rect,
                            const QFontMetrics& metrics);
    /** @brief Finds the nearest visible handle inside the configured pick radius. */
    bool find_handle_near(const QPointF& pixel, std::size_t& out_index) const;
    /** @brief Returns whether a sample is currently selected. */
    [[nodiscard]] bool has_selection() const noexcept;
    /** @brief Returns whether the cursor currently hovers a visible handle. */
    [[nodiscard]] bool has_hovered_handle() const noexcept;
    /** @brief Marks a sample as selected and repaints handle emphasis. */
    void set_selected_index(std::size_t index);
    /** @brief Clears the current selection state. */
    void clear_selection();
    /** @brief Marks a sample as hovered for visual feedback. */
    void set_hovered_index(std::size_t index);
    /** @brief Clears the hovered-handle state. */
    void clear_hovered_index();
};

}  // namespace signal_editor::adapters::qt
