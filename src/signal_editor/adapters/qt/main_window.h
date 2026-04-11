#pragma once

#include "signal_editor/core/domain/signal_library.h"

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <vector>

class QAction;
class QLabel;

namespace myprj::signal_editor {
class SignalEditorService;
}  // namespace myprj::signal_editor

namespace myprj::signal_editor::adapters::qt {

class FileListPanel;
class SignalListPanel;
class SignalPlotWidget;
class SignalTablePanel;

/**
 * @brief Top-level Qt shell for the Signal Editor desktop experience.
 *
 * The window coordinates the workspace-level concerns of the application:
 * multi-file management, undo snapshots, dialog flows, and rebinding the
 * currently selected signal into the dedicated plot and table widgets.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Creates the main editor window around the supplied service.
     * @param service Use-case orchestrator used to mutate the active library.
     * @param parent Optional owning widget supplied by Qt.
     */
    explicit MainWindow(SignalEditorService& service, QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onOpen();
    void onSave();
    void onUndo();
    void onNewSignal();
    void onRemoveSignal();
    void onRenameSignal();
    void onAbout();

    void onFileSelectionChanged(int index);
    void onFileRemoveRequested(int index);
    void onFileDetailsRequested(int index);
    void onSignalSelectionChanged(int index);
    void onRenameRequested(int index, const QString& new_name);
    void onAddRequested();
    void onRemoveRequested(int index);
    void onPlotEditStarted();
    void onPlotChanged();
    void onTableEditStarted();
    void onTableChanged();
    void onSignalInterpolationChanged(int mode);
    void onCursorMoved(double t, double y);

private:
    struct UndoState {
        SignalLibrary library;
        int selected_signal_index{0};
    };

    struct LoadedDocument {
        QString path;
        QString display_name;
        SignalLibrary library;
        std::vector<UndoState> undo_stack;
        bool dirty{false};
    };

    SignalEditorService& service_;
    QLabel* workspace_title_label_{nullptr};
    QLabel* workspace_meta_label_{nullptr};
    QLabel* workspace_hint_label_{nullptr};
    FileListPanel* file_panel_{nullptr};
    SignalListPanel* list_panel_{nullptr};
    SignalPlotWidget* plot_{nullptr};
    SignalTablePanel* table_panel_{nullptr};
    QAction* undo_action_{nullptr};
    QAction* rename_action_{nullptr};
    std::vector<LoadedDocument> documents_;
    int active_document_index_{-1};
    bool switching_document_{false};

    void open_paths(const QStringList& paths);
    void load_csv(const QString& path);
    void sync_active_document_from_service();
    void activate_document(int index, int preferred_signal_index = 0);
    void mark_active_document_dirty(bool dirty = true);
    void push_undo_state();
    void discard_last_undo_state();
    void clear_undo_history();
    void refresh_file_panel();
    void show_file_details(int index);
    void rebind_plot();
    void update_undo_action();
    void refresh_status(const QString& transient_message = {});
    void show_error(const QString& title, const QString& message);
};

}  // namespace myprj::signal_editor::adapters::qt
