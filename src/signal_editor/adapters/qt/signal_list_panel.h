#pragma once

#include <QWidget>

#include <cstddef>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace myprj::signal_editor {
class SignalLibrary;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

/**
 * @brief Sidebar that exposes the signals contained in the active document.
 *
 * The panel mirrors the bound library without taking ownership of it. It keeps
 * the list presentation, surfaces quick metadata about the current selection,
 * and forwards user intent back to the window controller via Qt signals.
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
    void selectionChanged(int index);
    void renameRequested(int index, const QString& new_name);
    void addRequested();
    void removeRequested(int index);

private slots:
    void onCurrentRowChanged(int row);
    void onItemChanged(QListWidgetItem* item);
    void onAddClicked();
    void onRemoveClicked();

private:
    const SignalLibrary* library_{nullptr};
    QLabel* summary_label_{nullptr};
    QLabel* detail_label_{nullptr};
    QListWidget* list_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    bool suppress_item_changed_{false};

    void refresh_summary();
};

}  // namespace myprj::signal_editor::adapters::qt
