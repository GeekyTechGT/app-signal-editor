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

namespace myprj::signal_editor::adapters::qt {

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
    void editStarted();
    void contentChanged();

private slots:
    void onItemChanged(QTableWidgetItem* item);
    void onAddClicked();
    void onRemoveClicked();

private:
    const Signal* signal_{nullptr};
    QLabel* stats_label_{nullptr};
    QLabel* hint_label_{nullptr};
    QTableWidget* table_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    QStyledItemDelegate* item_delegate_{nullptr};
    bool suppress_item_changed_{false};

    void repopulate();
    void set_row_values(int row, double t, double y);
    [[nodiscard]] double default_insert_time() const;
    [[nodiscard]] double default_insert_value() const;
    void refresh_summary() const;
};

}  // namespace myprj::signal_editor::adapters::qt
