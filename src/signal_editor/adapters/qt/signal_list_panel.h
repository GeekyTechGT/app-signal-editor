#pragma once

#include <QWidget>

#include <cstddef>
#include <vector>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QEvent;

namespace signal_editor {
class SignalLibrary;
}  // namespace signal_editor

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief Signal sidebar widget bound to the active workspace document.
 */

/**
 * @brief Sidebar that exposes the signals contained in the active document.
 *
 * `SignalListPanel` is intentionally a thin presentation and interaction
 * widget. It reflects three pieces of state supplied by the controller:
 *
 * - the library currently loaded for the active document
 * - the set of signals visible in plot/table
 * - the single active editable signal
 *
 * The panel does not own signal data and does not decide persistence or
 * editing rules. Its job is to render the list, make the active signal obvious
 * to the user, and emit semantic events when the user selects, renames, adds,
 * removes, or toggles visibility of a signal.
 *
 * This separation is especially important because "selected in the Qt list",
 * "visible in the plot", and "active editable signal" are related but not
 * identical concepts in the workspace model.
 */
class SignalListPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Creates the list panel and its inline action controls.
     * @param parent Optional owning widget supplied by Qt.
     */
    explicit SignalListPanel(QWidget* parent = nullptr);

    /**
     * @brief Binds the panel to a library owned elsewhere.
     * @param library Library currently shown in the workspace, or `nullptr`.
     */
    void set_library(const SignalLibrary* library);
    /** @brief Updates which signals are currently plotted. */
    void set_visible_signal_indices(const std::vector<int>& indices);
    /** @brief Marks one signal as the active editable signal. */
    void set_active_signal_index(int index);

    /**
     * @brief Rebuilds the list from the currently bound library.
     */
    void refresh();

    /**
     * @brief Selects a specific signal row if it exists.
     * @param index Zero-based signal index.
     */
    void select(int index);

    /**
     * @brief Enters in-place rename mode for the active signal.
     */
    void begin_rename_current();

    /**
     * @brief Returns the selected signal index.
     * @return The current row or `-1` when the list is empty.
     */
    [[nodiscard]] int current_index() const;

signals:
    /** @brief Emitted when the active signal row changes. */
    void selectionChanged(int index);
    /** @brief Emitted when the user confirms an in-place signal rename. */
    void renameRequested(int index, const QString& new_name);
    /** @brief Emitted when the user requests creation of a new signal. */
    void addRequested();
    /** @brief Emitted when the user requests removal of the active signal. */
    void removeRequested(int index);
    /** @brief Emitted when the user requests options for the active signal. */
    void optionsRequested(int index);
    /** @brief Emitted when the user toggles plot visibility for one signal. */
    void visibilityChanged(int index, bool visible);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onCurrentRowChanged(int row);
    void onItemChanged(QListWidgetItem* item);
    void onAddClicked();
    void onRemoveClicked();
    void onOptionsClicked();

private:
    const SignalLibrary* library_{nullptr};
    QLabel* title_label_{nullptr};
    QLabel* summary_label_{nullptr};
    QLabel* detail_label_{nullptr};
    QListWidget* list_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    QPushButton* options_button_{nullptr};
    bool suppress_item_changed_{false};
    std::vector<int> visible_signal_indices_;
    int active_signal_index_{-1};

    /** @brief Retranslates labels, tooltips, and action buttons. */
    void retranslate_ui();
    /** @brief Reapplies title/body typography derived from the active theme. */
    void refresh_typography();
    /** @brief Recomputes the sidebar summary for the current selection. */
    void refresh_summary();
};

}  // namespace signal_editor::adapters::qt
