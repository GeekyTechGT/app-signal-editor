#pragma once

#include "signal_editor/core/domain/sample_point.h"
#include "signal_editor/core/domain/signal.h"

#include <QPoint>
#include <QWidget>

#include <vector>

class QLabel;
class QLineEdit;
class QPushButton;
class QStyledItemDelegate;
class QTableWidget;
class QTableWidgetItem;

namespace signal_editor {
class SignalLibrary;
}

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief Editable multi-signal sample table bound to the plotted signals.
 */

/**
 * @brief Editable sample table showing the active and plotted signals.
 *
 * `SignalTablePanel` presents the same logical workspace data as the plot, but
 * in a table-oriented form that is better suited for precise inspection and
 * exact numeric edits. The table intentionally follows the current plot
 * visibility state:
 *
 * - one shared time column is shown for the file-level time axis
 * - one value column is shown for every signal currently plotted
 * - only the active signal column remains editable
 * - additional plotted signals stay visible as read-only context
 *
 * This design lets contributors preserve a single conceptual workspace while
 * supporting both "shape-first" editing in the plot and "value-first" editing
 * in the table. The search line edit above the table is intentionally local to
 * the widget and only filters visible rows; it does not change the underlying
 * data model.
 */
class SignalTablePanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Creates the sample table and action buttons.
     * @param parent Optional owning widget supplied by Qt.
     */
    explicit SignalTablePanel(QWidget* parent = nullptr);

    /**
     * @brief Binds the panel to the active library and plotted signals.
     *
     * The panel assumes the bound signals belong to one file-level library that
     * shares a common time axis. It does not take ownership of the library.
     */
    void set_library(const SignalLibrary* library,
                     int active_signal_index,
                     const std::vector<int>& visible_signal_indices);

    /**
     * @brief Repopulates the table using the bound signal state.
     */
    void refresh();

    /**
     * @brief Extracts the current table content as domain samples.
     * @return Editable snapshot of the rows currently displayed.
     */
    [[nodiscard]] std::vector<SamplePoint> samples() const;

signals:
    /** @brief Emitted when the user begins an edit that should create undo state. */
    void editStarted();
    /** @brief Emitted after table content has materially changed. */
    void contentChanged();
    /** @brief Emitted when one table row edits the active sample. */
    void sampleEdited(int row, double t, double y);
    /** @brief Emitted when the user inserts a new shared sample row. */
    void sampleInserted(double t, double y);
    /** @brief Emitted when the user removes a shared sample row. */
    void sampleRemoved(int row);
    /** @brief Emitted when the user requests a file-wide offset. */
    void offsetAllRequested(double delta_y);
    /** @brief Emitted when the user requests an offset on one row. */
    void sampleOffsetRequested(int row, double delta_y);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onItemChanged(QTableWidgetItem* item);
    void onAddClicked();
    void onRemoveClicked();
    void onContextMenuRequested(const QPoint& pos);

private:
    const SignalLibrary* library_{nullptr};
    int active_signal_index_{-1};
    std::vector<int> visible_signal_indices_;
    QLabel* title_label_{nullptr};
    QLabel* stats_label_{nullptr};
    QLabel* hint_label_{nullptr};
    QLineEdit* search_edit_{nullptr};
    QTableWidget* table_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    QStyledItemDelegate* item_delegate_{nullptr};
    bool suppress_item_changed_{false};
    int displayed_row_count_{0};

    /** @brief Retranslates labels, hints, buttons, and header copy. */
    void retranslate_ui();
    /** @brief Reapplies title/body typography derived from the active theme. */
    void refresh_typography();
    /** @brief Rebuilds all visible rows from the bound signal. */
    void repopulate();
    /** @brief Writes one table row using the current plotted signal set. */
    void set_row_values(int row);
    /** @brief Returns whether the table is showing a bounded prefix of a large signal. */
    [[nodiscard]] bool is_row_display_truncated() const;
    /** @brief Returns the suggested insertion time for a new row. */
    [[nodiscard]] double default_insert_time() const;
    /** @brief Returns the suggested insertion value for a new row. */
    [[nodiscard]] double default_insert_value() const;
    /** @brief Returns the plotted signal indices currently displayed as columns. */
    [[nodiscard]] std::vector<int> displayed_signal_indices() const;
    /** @brief Returns the reference signal used for shared time values. */
    [[nodiscard]] const Signal* reference_signal() const;
    /** @brief Returns the active signal if it is valid inside the bound library. */
    [[nodiscard]] const Signal* active_signal() const;
    /** @brief Returns whether the active signal is currently displayed in the table. */
    [[nodiscard]] bool is_active_signal_displayed() const;
    /** @brief Returns the table column that edits the active signal value, or -1. */
    [[nodiscard]] int active_value_column() const;
    /** @brief Returns the signal shown in a given value column, or nullptr. */
    [[nodiscard]] const Signal* signal_for_value_column(int column) const;
    /** @brief Refreshes the sample/interpolation summary labels. */
    void refresh_summary() const;
    /** @brief Applies the current search filter to the visible rows. */
    void apply_search_filter();
};

}  // namespace signal_editor::adapters::qt
