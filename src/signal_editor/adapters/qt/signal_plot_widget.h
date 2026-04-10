#pragma once

#include <QPointF>
#include <QWidget>

#include <cstddef>

namespace myprj::signal_editor {
class Signal;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

// --- Driving adapter (Qt) ---------------------------------------------------
// A custom QWidget that paints a `Signal` and lets the user manipulate its
// samples directly with the mouse.
//
// Interaction model:
//   * Left click on a sample handle and drag        -> change y of that sample.
//   * Left click + drag on the curve away from any
//     handle, holding `Shift`                       -> Gaussian deformation
//                                                      around the click point.
//   * Double click on the canvas                    -> insert a new sample.
//   * Right click near a sample                     -> remove it.
//   * Hovering                                      -> emits `cursorMoved` so
//                                                      the main window can show
//                                                      the (t, y) in the
//                                                      status bar.
//
// The widget never owns the `Signal`. It holds a non-owning pointer that
// the controller updates when the user picks a new signal in the list panel.
class SignalPlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit SignalPlotWidget(QWidget* parent = nullptr);

    // Bind the widget to a signal. `nullptr` clears the view.
    void set_signal(Signal* signal);

    // Notify the widget that the bound signal was mutated externally.
    void refresh();

signals:
    // Emitted whenever the bound signal was modified by user interaction.
    void signalChanged();

    // Emitted whenever the cursor hovers over a new (t, y) pair.
    void cursorMoved(double t, double y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    static constexpr int    kMargin       = 48;
    static constexpr double kHandleRadius = 6.0;
    static constexpr double kPickRadius   = 10.0;

    Signal* signal_{nullptr};

    // Cached view rectangle in data coordinates.
    double view_t_min_{0.0};
    double view_t_max_{1.0};
    double view_y_min_{-1.0};
    double view_y_max_{1.0};

    // Drag state.
    enum class DragMode { None, MoveSample, GaussianBrush };
    DragMode    drag_mode_{DragMode::None};
    std::size_t drag_index_{0};
    QPointF     drag_start_data_;

    // --- Coordinate helpers --------------------------------------------
    QPointF data_to_pixel(double t, double y) const;
    QPointF pixel_to_data(const QPointF& pixel) const;

    // Recompute view bounds to fit the bound signal (with a margin).
    void auto_fit_view();
    bool find_handle_near(const QPointF& pixel, std::size_t& out_index) const;
};

}  // namespace myprj::signal_editor::adapters::qt
