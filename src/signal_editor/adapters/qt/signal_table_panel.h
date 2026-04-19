#pragma once

#include "signal_editor/core/domain/sample_point.h"
#include "signal_editor/core/domain/signal.h"

#include <QWidget>

#include <vector>

class QLabel;
class QPushButton;
class QStyledItemDelegate;
class QTableWidget;
class QTableWidgetItem;

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief Editable sample table bound to the active signal.
 */

/**
 * @brief Editable sample table bound to the currently selected signal.
 *
 * The widget exposes the discrete points of a signal as tabular data and lets
 * the caller react to edits, insertions, removals, and interpolation changes.
 * It never owns the signal instance and only mirrors the currently bound one.
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
     * @brief Binds the panel to a signal instance owned elsewhere.
     * @param signal Signal currently shown in the table, or `nullptr`.
     */
    void set_signal(const Signal* signal);

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

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onItemChanged(QTableWidgetItem* item);
    void onAddClicked();
    void onRemoveClicked();

private:
    const Signal* signal_{nullptr};
    QLabel* title_label_{nullptr};
    QLabel* stats_label_{nullptr};
    QLabel* hint_label_{nullptr};
    QTableWidget* table_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    QStyledItemDelegate* item_delegate_{nullptr};
    bool suppress_item_changed_{false};

    /** @brief Retranslates labels, hints, buttons, and header copy. */
    void retranslate_ui();
    /** @brief Reapplies title/body typography derived from the active theme. */
    void refresh_typography();
    /** @brief Rebuilds all visible rows from the bound signal. */
    void repopulate();
    /** @brief Writes a `(t, y)` pair into a specific table row. */
    void set_row_values(int row, double t, double y);
    /** @brief Returns the suggested insertion time for a new row. */
    [[nodiscard]] double default_insert_time() const;
    /** @brief Returns the suggested insertion value for a new row. */
    [[nodiscard]] double default_insert_value() const;
    /** @brief Refreshes the sample/interpolation summary labels. */
    void refresh_summary() const;
};

}  // namespace signal_editor::adapters::qt
