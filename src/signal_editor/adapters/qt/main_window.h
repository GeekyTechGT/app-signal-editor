#pragma once

#include "signal_editor/adapters/qt/theme.h"
#include "signal_editor/core/domain/signal_library.h"

#include <QColor>
#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

class QAction;
class QComboBox;
class QCloseEvent;
class QLineEdit;
class QLabel;
class QMenu;
class QPushButton;
class QTabWidget;
class QToolBar;
class QToolButton;
class QTranslator;

#ifdef LIB_QT_UTILS_AVAILABLE
#include <app_state.hpp>
#endif

// lib-qt-custom-widgets (optional — present only when GUI is built with the dep)
#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
#include <app_settings.hpp>
#else
// Minimal stub so the class compiles without the external library.
namespace lib_qt_custom_widgets {
struct AppSettings {
    QString theme      {QStringLiteral("light")};
    QColor  primaryColor{0, 120, 212};
    bool    highContrastMode{false};
    QString fontFamily {QStringLiteral("Segoe UI")};
    int     fontSize   {10};
    QString widgetDensity{QStringLiteral("normal")};
    QString language   {QStringLiteral("it")};
    int     animationDurationMs{250};

    [[nodiscard]] bool operator==(const AppSettings& o) const noexcept {
        return theme            == o.theme
            && primaryColor     == o.primaryColor
            && highContrastMode == o.highContrastMode
            && fontFamily       == o.fontFamily
            && fontSize         == o.fontSize
            && widgetDensity    == o.widgetDensity
            && language         == o.language
            && animationDurationMs == o.animationDurationMs;
    }
    [[nodiscard]] bool operator!=(const AppSettings& o) const noexcept { return !(*this == o); }
};
}  // namespace lib_qt_custom_widgets
#endif

namespace signal_editor {
class SignalEditorService;
}  // namespace signal_editor

namespace signal_editor::adapters::qt {

class FileListPanel;
class SignalListPanel;
class SignalPlotWidget;
class SignalTablePanel;

/**
 * @brief Top-level Qt shell for the Signal Editor desktop experience.
 *
 * Coordinates multi-file management, undo snapshots, dialog flows, and
 * rebinding the currently selected signal into the dedicated plot and table
 * widgets.  Integrates with SettingsPanelDialog (lib-qt-custom-widgets) for
 * runtime theme / font / language switching.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(SignalEditorService& service, QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpen();
    void onSave();
    void onUndo();
    void onNewSignal();
    void onRemoveSignal();
    void onRenameSignal();
    void onAbout();
    void onOpenSettings();

    void onFileSelectionChanged(int index);
    void onFileRemoveRequested(int index);
    void onFileDetailsRequested(int index);
    void onFileRenameRequested(int index, const QString& new_name);
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
    void onPlotZoomInRequested();
    void onPlotZoomOutRequested();
    void onPlotPanModeToggled(bool checked);
    void onPlotRectModeToggled(bool checked);
    void onPlotRangeEditingFinished();
    void onPlotTimeViewChanged(double t_start, double t_end);

    void onSettingsApplied(const lib_qt_custom_widgets::AppSettings& settings);
    void onSettingsSaved(const lib_qt_custom_widgets::AppSettings& settings);

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

    // Workspace header labels
    QLabel* workspace_title_label_{nullptr};
    QLabel* workspace_meta_label_{nullptr};
    QLabel* workspace_hint_label_{nullptr};

    // Panels & editing widgets
    FileListPanel*    file_panel_{nullptr};
    SignalListPanel*  list_panel_{nullptr};
    SignalPlotWidget* plot_{nullptr};
    SignalTablePanel* table_panel_{nullptr};
    QComboBox*        interpolation_box_{nullptr};
    QLabel*           interp_label_{nullptr};
    QLabel*           plot_t_start_label_{nullptr};
    QLabel*           plot_t_end_label_{nullptr};
    QLineEdit*        plot_t_start_edit_{nullptr};
    QLineEdit*        plot_t_end_edit_{nullptr};
    QPushButton*      plot_zoom_in_button_{nullptr};
    QPushButton*      plot_zoom_out_button_{nullptr};
    QPushButton*      plot_pan_mode_button_{nullptr};
    QPushButton*      plot_rect_mode_button_{nullptr};
    QPushButton*      plot_reset_view_button_{nullptr};
    QTabWidget*       workspace_tabs_{nullptr};
    QToolBar*         main_toolbar_{nullptr};

    // Menus (stored for retranslation)
    QMenu* menu_file_{nullptr};
    QMenu* menu_signal_{nullptr};
    QMenu* menu_settings_{nullptr};
    QMenu* menu_help_{nullptr};

    // Actions (all stored for retranslation)
    QAction* act_open_{nullptr};
    QAction* act_save_{nullptr};
    QAction* act_quit_{nullptr};
    QAction* act_new_{nullptr};
    QAction* act_remove_{nullptr};
    QAction* act_about_{nullptr};
    QAction* undo_action_{nullptr};
    QAction* rename_action_{nullptr};
    QAction* settings_action_{nullptr};

    // Status bar settings button (kept in the permanent area so status
    // messages do not cover it).
    QToolButton* settings_btn_{nullptr};

    // Current application settings (persisted in memory; restored on next launch
    // via QSettings when the feature is wired further).
    lib_qt_custom_widgets::AppSettings app_settings_;
    lib_qt_custom_widgets::AppSettings visual_settings_;

    // Active translator installed for the current language.
    QTranslator* active_translator_{nullptr};

#ifdef LIB_QT_UTILS_AVAILABLE
    std::unique_ptr<QtUtils::AppState> app_state_;
#endif

    // Document state
    std::vector<LoadedDocument> documents_;
    int  active_document_index_{-1};
    bool switching_document_{false};

    // ── Internal helpers ──────────────────────────────────────────────────────
    void open_paths(const QStringList& paths);
    void load_document(const QString& path);
    int ensure_workspace_document();
    void sync_active_document_from_service();
    void activate_document(int index, int preferred_signal_index = 0);
    void mark_active_document_dirty(bool dirty = true);
    void push_undo_state();
    void discard_last_undo_state();
    void clear_undo_history();
    void refresh_file_panel();
    void show_file_details(int index);
    void rebind_plot();
    void sync_plot_view_controls();
    void sync_plot_navigation_mode_controls();
    void update_undo_action();
    void update_interpolation_box();
    void refresh_status(const QString& transient_message = {});
    void show_error(const QString& title, const QString& message);
    void retranslate_ui();
    void load_persisted_settings();
    void persist_settings();
    void apply_settings(const lib_qt_custom_widgets::AppSettings& settings, bool persist);
    void apply_visual_settings(const lib_qt_custom_widgets::AppSettings& settings,
                               bool include_density,
                               bool force_apply = false);
    void apply_density(const QString& density);
    void apply_language(const QString& language);
    void apply_behavior_settings(const lib_qt_custom_widgets::AppSettings& settings);
};

}  // namespace signal_editor::adapters::qt
