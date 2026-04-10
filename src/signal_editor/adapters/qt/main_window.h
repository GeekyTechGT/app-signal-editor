#pragma once

#include <QMainWindow>

namespace myprj::signal_editor {
class SignalEditorService;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

class SignalListPanel;
class SignalPlotWidget;

// --- Driving adapter (Qt) ---------------------------------------------------
// Top-level window. Wires the list panel, the plot widget and the menu/tool
// bar to the `SignalEditorService` (use case orchestrator). The window owns
// no domain state — every action goes through the service.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(SignalEditorService& service, QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onOpen();
    void onSave();
    void onNewSignal();
    void onRemoveSignal();
    void onAbout();

    void onSelectionChanged(int index);
    void onRenameRequested(int index, const QString& new_name);
    void onAddRequested();
    void onRemoveRequested(int index);
    void onPlotChanged();
    void onCursorMoved(double t, double y);

private:
    SignalEditorService& service_;
    SignalListPanel*     list_panel_{nullptr};
    SignalPlotWidget*    plot_{nullptr};

    void load_csv(const QString& path);
    void rebind_plot();
    void refresh_status();
    void show_error(const QString& title, const QString& message);
};

}  // namespace myprj::signal_editor::adapters::qt
