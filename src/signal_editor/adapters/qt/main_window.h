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

/**
 * @file
 * @brief Main Qt shell that coordinates the Signal Editor workspace.
 */

class FileListPanel;
class SignalListPanel;
class SignalPlotWidget;
class SignalTablePanel;

/**
 * @brief Top-level Qt shell for the Signal Editor desktop experience.
 *
 * `MainWindow` is the orchestration hub of the desktop application. It owns
 * the workspace-level document model, coordinates sidebar selections, drives
 * plot and table rebinding, and translates UI gestures into calls on
 * `SignalEditorService`.
 *
 * The class deliberately keeps business rules out of individual widgets.
 * Instead, the supporting widgets emit semantic signals such as
 * "sample moved", "visibility changed", or "reload requested", and the main
 * window decides how that intent affects the currently active workspace
 * document.
 *
 * Important responsibilities of this class include:
 *
 * - maintaining the multi-document workspace state
 * - tracking which signals are visible in the plot and table
 * - tracking the active editable signal independently from raw Qt selection
 * - preserving undo history at the document level
 * - preserving view state such as plot zoom across relevant rebinding flows
 * - mediating persistence, reload, and settings application workflows
 *
 * The resulting design keeps the UI modular while still giving contributors a
 * single, predictable place to inspect the end-to-end application flow.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Creates the top-level workspace window.
     * @param service Application service used to load, save, and edit signals.
     * @param parent Optional owning widget supplied by Qt.
     */
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
    void onExportPlotImage();
    void onUndo();
    void onNewSignal();
    void onRemoveSignal();
    void onRenameSignal();
    void onAbout();
    void onOpenSettings();

    void onFileSelectionChanged(int index);
    void onFileRemoveRequested(int index);
    void onFileReloadRequested(int index);
    void onFileDetailsRequested(int index);
    void onFileRenameRequested(int index, const QString& new_name);
    void onSignalSelectionChanged(int index);
    void onSignalVisibilityChanged(int index, bool visible);
    void onSignalOptionsRequested(int index);
    void onRenameRequested(int index, const QString& new_name);
    void onAddRequested();
    void onRemoveRequested(int index);
    void onPlotEditStarted();
    void onPlotChanged();
    void onPlotSampleMoveRequested(std::size_t sample_index, double t, double y);
    void onPlotSampleInsertRequested(double t, double y);
    void onPlotSampleRemoveRequested(std::size_t sample_index);
    void onPlotSegmentMoveRequested(std::size_t start_index, double delta_y);
    void onPlotGaussianBrushRequested(double t_center, double delta_y, double sigma);
    void onOffsetAllRequested(double delta_y);
    void onPlotSampleOffsetRequested(std::size_t sample_index, double delta_y);
    void onTableEditStarted();
    void onTableChanged();
    void onTableSampleEdited(int row, double t, double y);
    void onTableSampleInserted(double t, double y);
    void onTableSampleRemoved(int row);
    void onTableSampleOffsetRequested(int row, double delta_y);
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
    /**
     * @brief Undo snapshot tied to the active workspace document.
     *
     * The snapshot stores both the library content and the active signal index
     * because restoring waveform data without restoring selection context would
     * yield a confusing desktop experience after undo.
     */
    struct UndoState {
        SignalLibrary library;
        int selected_signal_index{0};
    };

    /**
     * @brief Workspace-level state tracked for each loaded document.
     *
     * Each loaded input file is represented as a self-contained document state.
     * The state carries:
     *
     * - the persisted source path and current display label
     * - a `SignalLibrary` snapshot used when the document becomes active
     * - the set of signals currently visible in the plot/table
     * - the active editable signal index
     * - the undo stack associated only with that document
     * - a dirty flag describing whether the in-memory state diverged from disk
     *
     * Keeping this state here, instead of scattering it across widgets, makes
     * document switching deterministic and simplifies onboarding for new
     * contributors.
     */
    struct LoadedDocument {
        QString path;
        QString display_name;
        SignalLibrary library;
        std::vector<int> visible_signal_indices;
        int active_signal_index{-1};
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
    QPushButton*      plot_export_button_{nullptr};
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
    QAction* act_export_plot_{nullptr};
    QAction* act_new_{nullptr};
    QAction* act_remove_{nullptr};
    QAction* act_about_{nullptr};
    QAction* undo_action_{nullptr};
    QAction* rename_action_{nullptr};
    QAction* settings_action_{nullptr};

    // Status bar settings button (kept in the permanent area so status
    // messages do not cover it).
    QToolButton* settings_btn_{nullptr};

    // Current application settings restored from and persisted to the versioned
    // QSettings namespace.
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

    /** @brief Loads one or more user-selected paths into the workspace. */
    void open_paths(const QStringList& paths);
    /** @brief Loads a persisted document and adds it to the workspace set. */
    void load_document(const QString& path);
    /** @brief Ensures there is always at least one editable workspace document. */
    int ensure_workspace_document();
    /** @brief Pushes the currently active document state back into the service. */
    void sync_active_document_from_service();
    /** @brief Activates a document and binds its selected signal to the editors. */
    void activate_document(int index, int preferred_signal_index = 0);
    /** @brief Marks the current document dirty or clean. */
    void mark_active_document_dirty(bool dirty = true);
    /** @brief Stores a reversible snapshot before a destructive edit. */
    void push_undo_state();
    /** @brief Drops the last snapshot when an edit could not be completed. */
    void discard_last_undo_state();
    /** @brief Clears undo history for the active workspace document. */
    void clear_undo_history();
    /** @brief Rebuilds the file sidebar from the current document set. */
    void refresh_file_panel();
    /** @brief Shows the details dialog for the selected document. */
    void show_file_details(int index);
    /** @brief Shows options for the addressed signal inside the active document. */
    void show_signal_options(int index);
    /** @brief Rebinds the active signal into the plot and table views. */
    void rebind_plot();
    /** @brief Keeps plotted-signal state valid for one document. */
    void normalize_visible_signal_indices(LoadedDocument& document) const;
    /** @brief Returns the plotted signals for the active document. */
    [[nodiscard]] std::vector<int> active_visible_signal_indices() const;
    /** @brief Returns the active signal index for the current document. */
    [[nodiscard]] int active_signal_index() const;
    /** @brief Synchronizes the time-range controls with the plot viewport. */
    void sync_plot_view_controls();
    /** @brief Synchronizes toolbar toggle state with the plot navigation mode. */
    void sync_plot_navigation_mode_controls();
    /** @brief Enables or disables undo according to the active document state. */
    void update_undo_action();
    /** @brief Updates the interpolation combo from the active signal. */
    void update_interpolation_box();
    /** @brief Refreshes status-bar copy and document counters. */
    void refresh_status(const QString& transient_message = {});
    /** @brief Shows a user-facing error dialog. */
    void show_error(const QString& title, const QString& message);
    /** @brief Retranslates all window-owned UI strings after a language change. */
    void retranslate_ui();
    /** @brief Restores persisted geometry and visual settings from QSettings. */
    void load_persisted_settings();
    /** @brief Persists the current UI settings and window state. */
    void persist_settings();
    /** @brief Applies the full settings payload, optionally persisting it. */
    void apply_settings(const lib_qt_custom_widgets::AppSettings& settings, bool persist);
    /** @brief Applies theme, accent, font, and density related settings. */
    void apply_visual_settings(const lib_qt_custom_widgets::AppSettings& settings,
                               bool include_density,
                               bool force_apply = false);
    /** @brief Applies the requested widget density to the workspace shell. */
    void apply_density(const QString& density);
    /** @brief Installs the selected translator and retranslates the window tree. */
    void apply_language(const QString& language);
    /** @brief Applies behavior-only settings that do not affect styling. */
    void apply_behavior_settings(const lib_qt_custom_widgets::AppSettings& settings);
};

}  // namespace signal_editor::adapters::qt
