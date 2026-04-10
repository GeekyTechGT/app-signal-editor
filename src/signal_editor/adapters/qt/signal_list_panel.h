#pragma once

#include <QWidget>

#include <cstddef>

class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace myprj::signal_editor {
class SignalLibrary;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

// Sidebar that shows the names of all signals in the bound `SignalLibrary`
// and forwards user actions (selection, rename, add, remove) as Qt signals
// to the controller.
//
// Like the plot widget, the panel does NOT own the library — it only reads
// from it. The controller decides how to mutate it via use cases.
class SignalListPanel : public QWidget {
    Q_OBJECT

public:
    explicit SignalListPanel(QWidget* parent = nullptr);

    void set_library(const SignalLibrary* library);
    void refresh();
    void select(int index);

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
    QListWidget*         list_{nullptr};
    QPushButton*         add_button_{nullptr};
    QPushButton*         remove_button_{nullptr};
    bool                 suppress_item_changed_{false};
};

}  // namespace myprj::signal_editor::adapters::qt
