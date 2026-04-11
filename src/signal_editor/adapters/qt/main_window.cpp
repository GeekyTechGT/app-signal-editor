#include "signal_editor/adapters/qt/main_window.h"

#include "signal_editor/adapters/qt/file_list_panel.h"
#include "signal_editor/adapters/qt/signal_list_panel.h"
#include "signal_editor/adapters/qt/signal_plot_widget.h"
#include "signal_editor/adapters/qt/signal_table_panel.h"
#include "signal_editor/core/usecases/signal_editor_service.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QKeyEvent>
#include <QComboBox>
#include <QDateTime>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace myprj::signal_editor::adapters::qt {

namespace {
constexpr double kPi = 3.14159265358979323846;

QString summarize_counts(const SignalLibrary& library) {
    return QStringLiteral("%1 signal(s)").arg(static_cast<qulonglong>(library.size()));
}

enum class WaveformKind {
    Constant = 0,
    Sine,
    Cosine,
    Pulse,
    Sawtooth,
    Triangle,
    Ramp,
    Enumerated,
};

const char* interpolation_label(Signal::InterpolationMode interpolation) {
    switch (interpolation) {
    case Signal::InterpolationMode::Linear:
        return "Linear";
    case Signal::InterpolationMode::Step:
        return "Step";
    }
    return "Linear";
}

double wrap_unit_phase(double x) {
    const double wrapped = std::fmod(x, 1.0);
    return wrapped < 0.0 ? wrapped + 1.0 : wrapped;
}

std::vector<double> build_uniform_time_axis(double t_start, double t_end, std::size_t num_samples) {
    if (num_samples < 2) {
        throw std::invalid_argument("num_samples must be >= 2");
    }
    if (!(t_end > t_start)) {
        throw std::invalid_argument("t end must be greater than t start");
    }

    std::vector<double> time;
    time.reserve(num_samples);
    const double dt = (t_end - t_start) / static_cast<double>(num_samples - 1);
    for (std::size_t i = 0; i < num_samples; ++i) {
        time.push_back(t_start + dt * static_cast<double>(i));
    }
    return time;
}

Signal generate_constant_signal(const QString& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double level) {
    return Signal::create_uniform(name.toStdString(), t_start, t_end, num_samples, level);
}

Signal generate_periodic_signal(const QString& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double amplitude,
                                double offset,
                                double frequency_hz,
                                double phase_deg,
                                bool use_cosine) {
    if (!(frequency_hz > 0.0)) {
        throw std::invalid_argument("frequency must be > 0");
    }

    const auto time = build_uniform_time_axis(t_start, t_end, num_samples);
    std::vector<double> values;
    values.reserve(time.size());
    const double phase_rad = phase_deg * kPi / 180.0;
    for (double t : time) {
        const double angle = 2.0 * kPi * frequency_hz * (t - t_start) + phase_rad;
        const double carrier = use_cosine ? std::cos(angle) : std::sin(angle);
        values.push_back(offset + amplitude * carrier);
    }
    return Signal::from_vectors(name.toStdString(), time, values);
}

Signal generate_pulse_signal(const QString& name,
                             double t_start,
                             double t_end,
                             std::size_t num_samples,
                             double low_level,
                             double high_level,
                             double frequency_hz,
                             double duty_cycle_pct,
                             double phase_deg) {
    if (!(frequency_hz > 0.0)) {
        throw std::invalid_argument("frequency must be > 0");
    }
    if (!(duty_cycle_pct > 0.0 && duty_cycle_pct < 100.0)) {
        throw std::invalid_argument("duty cycle must be between 0 and 100");
    }

    const auto time = build_uniform_time_axis(t_start, t_end, num_samples);
    std::vector<double> values;
    values.reserve(time.size());
    const double duty = duty_cycle_pct / 100.0;
    const double phase_cycles = phase_deg / 360.0;
    for (double t : time) {
        const double unit_phase = wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
        values.push_back(unit_phase < duty ? high_level : low_level);
    }
    return Signal::from_vectors(name.toStdString(), time, values);
}

Signal generate_sawtooth_signal(const QString& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double min_value,
                                double max_value,
                                double frequency_hz,
                                double phase_deg) {
    if (!(frequency_hz > 0.0)) {
        throw std::invalid_argument("frequency must be > 0");
    }

    const auto time = build_uniform_time_axis(t_start, t_end, num_samples);
    std::vector<double> values;
    values.reserve(time.size());
    const double span = max_value - min_value;
    const double phase_cycles = phase_deg / 360.0;
    for (double t : time) {
        const double unit_phase = wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
        values.push_back(min_value + span * unit_phase);
    }
    return Signal::from_vectors(name.toStdString(), time, values);
}

Signal generate_triangle_signal(const QString& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double min_value,
                                double max_value,
                                double frequency_hz,
                                double phase_deg) {
    if (!(frequency_hz > 0.0)) {
        throw std::invalid_argument("frequency must be > 0");
    }

    const auto time = build_uniform_time_axis(t_start, t_end, num_samples);
    std::vector<double> values;
    values.reserve(time.size());
    const double phase_cycles = phase_deg / 360.0;
    for (double t : time) {
        const double unit_phase = wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
        const double shape = unit_phase < 0.5
            ? unit_phase * 2.0
            : 2.0 - unit_phase * 2.0;
        values.push_back(min_value + (max_value - min_value) * shape);
    }
    return Signal::from_vectors(name.toStdString(), time, values);
}

Signal generate_ramp_signal(const QString& name,
                            double t_start,
                            double t_end,
                            std::size_t num_samples,
                            double start_value,
                            double end_value) {
    const auto time = build_uniform_time_axis(t_start, t_end, num_samples);
    std::vector<double> values;
    values.reserve(time.size());
    for (std::size_t i = 0; i < time.size(); ++i) {
        const double alpha = static_cast<double>(i) / static_cast<double>(time.size() - 1);
        values.push_back(start_value + (end_value - start_value) * alpha);
    }
    return Signal::from_vectors(name.toStdString(), time, values);
}

std::vector<Signal::EnumerationEntry> parse_enumeration_definition(const QString& text) {
    std::vector<Signal::EnumerationEntry> entries;
    const QStringList lines = text.split(QStringLiteral("\n"));
    for (const QString& raw_line : lines) {
        const QString line = raw_line.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const int separator = line.lastIndexOf(':');
        if (separator <= 0 || separator >= line.size() - 1) {
            throw std::invalid_argument("Each enumerated state must use the format LABEL:NUMERIC_VALUE");
        }

        bool ok = false;
        const QString label = line.left(separator).trimmed();
        const double value = line.mid(separator + 1).trimmed().toDouble(&ok);
        if (label.isEmpty() || !ok) {
            throw std::invalid_argument("Invalid enumerated state definition: " + line.toStdString());
        }
        entries.push_back(Signal::EnumerationEntry{label.toStdString(), value});
    }

    if (entries.empty()) {
        throw std::invalid_argument("Define at least one enumerated state");
    }
    return entries;
}

Signal generate_enumerated_signal(const QString& name,
                                  double t_start,
                                  double t_end,
                                  std::size_t num_samples,
                                  const std::vector<Signal::EnumerationEntry>& enumeration,
                                  const QString& initial_label) {
    QString resolved_label = initial_label.trimmed();
    if (resolved_label.isEmpty()) {
        resolved_label = QString::fromStdString(enumeration.front().label);
    }

    Signal signal = Signal::create_uniform(name.toStdString(), t_start, t_end, num_samples, 0.0,
                                           Signal::InterpolationMode::Step);
    signal.set_enumeration(enumeration);
    const double initial_value = signal.value_for_label(resolved_label.toStdString());
    for (std::size_t index = 0; index < signal.size(); ++index) {
        signal.set_sample_value(index, initial_value);
    }
    return signal;
}

QString format_signal_value(const Signal* signal, double y) {
    if (signal != nullptr && signal->is_enumerated()) {
        const std::string label = signal->label_for_value(y);
        if (!label.empty()) {
            return QStringLiteral("%1 (%2)")
                .arg(QString::fromStdString(label))
                .arg(y, 0, 'f', 4);
        }
    }
    return QString::number(y, 'f', 4);
}

QString describe_signal_line(const Signal& signal) {
    QString description = QStringLiteral("- %1 (%2 samples, %3)")
        .arg(QString::fromStdString(signal.name()))
        .arg(static_cast<qulonglong>(signal.size()))
        .arg(QString::fromLatin1(interpolation_label(signal.interpolation())));
    if (signal.is_enumerated()) {
        QStringList states;
        for (const auto& entry : signal.enumeration()) {
            states.push_back(QStringLiteral("%1:%2")
                .arg(QString::fromStdString(entry.label))
                .arg(entry.value, 0, 'f', 3));
        }
        description += QStringLiteral(" | enum [%1]").arg(states.join(QStringLiteral(", ")));
    }
    return description;
}

Signal generate_waveform_signal(WaveformKind kind,
                                const QString& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                const std::array<double, 4>& params_a,
                                const std::array<double, 4>& params_b) {
    switch (kind) {
    case WaveformKind::Constant:
        return generate_constant_signal(name, t_start, t_end, num_samples, params_a[0]);
    case WaveformKind::Sine:
        return generate_periodic_signal(name, t_start, t_end, num_samples,
                                        params_a[0], params_a[1], params_a[2], params_a[3], false);
    case WaveformKind::Cosine:
        return generate_periodic_signal(name, t_start, t_end, num_samples,
                                        params_a[0], params_a[1], params_a[2], params_a[3], true);
    case WaveformKind::Pulse:
        return generate_pulse_signal(name, t_start, t_end, num_samples,
                                     params_a[0], params_a[1], params_a[2], params_a[3], params_b[0]);
    case WaveformKind::Sawtooth:
        return generate_sawtooth_signal(name, t_start, t_end, num_samples,
                                        params_a[0], params_a[1], params_a[2], params_a[3]);
    case WaveformKind::Triangle:
        return generate_triangle_signal(name, t_start, t_end, num_samples,
                                        params_a[0], params_a[1], params_a[2], params_a[3]);
    case WaveformKind::Ramp:
        return generate_ramp_signal(name, t_start, t_end, num_samples, params_a[0], params_a[1]);
    case WaveformKind::Enumerated:
        break;
    }
    throw std::invalid_argument("unsupported waveform");
}
}  // namespace

MainWindow::MainWindow(SignalEditorService& service, QWidget* parent)
    : QMainWindow(parent), service_(service) {
    setWindowTitle(QStringLiteral("Signal Editor"));
    resize(1400, 820);
    setAcceptDrops(true);

    auto* central = new QWidget(this);
    auto* central_layout = new QVBoxLayout(central);
    central_layout->setContentsMargins(16, 16, 16, 16);
    central_layout->setSpacing(14);

    auto* workspace_header = new QWidget(central);
    workspace_header->setObjectName(QStringLiteral("WorkspaceHeader"));
    auto* workspace_layout = new QVBoxLayout(workspace_header);
    workspace_layout->setContentsMargins(20, 18, 20, 18);
    workspace_layout->setSpacing(4);

    workspace_title_label_ = new QLabel(QStringLiteral("Signal Editor Workspace"), workspace_header);
    workspace_title_label_->setObjectName(QStringLiteral("WorkspaceTitle"));
    workspace_meta_label_ = new QLabel(QStringLiteral("No active document"), workspace_header);
    workspace_meta_label_->setObjectName(QStringLiteral("WorkspaceMeta"));
    workspace_hint_label_ = new QLabel(
        QStringLiteral("Load CSV files, switch between workspace documents, and sculpt signals through the plot or table."),
        workspace_header);
    workspace_hint_label_->setObjectName(QStringLiteral("WorkspaceHint"));
    workspace_hint_label_->setWordWrap(true);
    workspace_layout->addWidget(workspace_title_label_);
    workspace_layout->addWidget(workspace_meta_label_);
    workspace_layout->addWidget(workspace_hint_label_);
    central_layout->addWidget(workspace_header);

    auto* outer_splitter = new QSplitter(Qt::Horizontal, central);
    auto* side_panel = new QWidget(outer_splitter);
    auto* side_layout = new QVBoxLayout(side_panel);
    side_layout->setContentsMargins(0, 0, 0, 0);
    side_layout->setSpacing(12);

    file_panel_ = new FileListPanel(side_panel);
    list_panel_ = new SignalListPanel(side_panel);
    side_layout->addWidget(file_panel_, 1);
    side_layout->addWidget(list_panel_, 2);

    auto* center_splitter = new QSplitter(Qt::Vertical, outer_splitter);
    plot_ = new SignalPlotWidget(center_splitter);
    table_panel_ = new SignalTablePanel(center_splitter);
    center_splitter->addWidget(plot_);
    center_splitter->addWidget(table_panel_);
    center_splitter->setStretchFactor(0, 3);
    center_splitter->setStretchFactor(1, 2);
    center_splitter->setSizes({540, 280});

    outer_splitter->addWidget(side_panel);
    outer_splitter->addWidget(center_splitter);
    outer_splitter->setStretchFactor(0, 0);
    outer_splitter->setStretchFactor(1, 1);
    outer_splitter->setSizes({340, 1080});
    central_layout->addWidget(outer_splitter, 1);
    setCentralWidget(central);

    list_panel_->set_library(nullptr);
    table_panel_->set_signal(nullptr);

    auto* menu_file = menuBar()->addMenu(QStringLiteral("&File"));
    auto* act_open = menu_file->addAction(QStringLiteral("&Open CSV files..."));
    auto* act_save = menu_file->addAction(QStringLiteral("&Save current CSV..."));
    undo_action_ = menu_file->addAction(QStringLiteral("&Undo"));
    menu_file->addSeparator();
    auto* act_quit = menu_file->addAction(QStringLiteral("&Quit"));
    act_open->setShortcut(QKeySequence::Open);
    act_save->setShortcut(QKeySequence::Save);
    undo_action_->setShortcut(QKeySequence::Undo);
    act_quit->setShortcut(QKeySequence::Quit);

    auto* menu_signal = menuBar()->addMenu(QStringLiteral("&Signal"));
    auto* act_new = menu_signal->addAction(QStringLiteral("&New from scratch..."));
    rename_action_ = menu_signal->addAction(QStringLiteral("Re&name selected"));
    auto* act_remove = menu_signal->addAction(QStringLiteral("&Remove selected"));
    act_new->setShortcut(QKeySequence::New);
    rename_action_->setShortcut(Qt::Key_F2);
    act_remove->setShortcut(QKeySequence::Delete);

    auto* menu_help = menuBar()->addMenu(QStringLiteral("&Help"));
    auto* act_about = menu_help->addAction(QStringLiteral("&About"));

    auto* tb = addToolBar(QStringLiteral("Main"));
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));
    tb->addAction(act_open);
    tb->addAction(act_save);
    tb->addAction(undo_action_);
    tb->addSeparator();
    tb->addAction(act_new);
    tb->addAction(rename_action_);
    tb->addAction(act_remove);

    connect(act_open, &QAction::triggered, this, &MainWindow::onOpen);
    connect(act_save, &QAction::triggered, this, &MainWindow::onSave);
    connect(undo_action_, &QAction::triggered, this, &MainWindow::onUndo);
    connect(act_new, &QAction::triggered, this, &MainWindow::onNewSignal);
    connect(rename_action_, &QAction::triggered, this, &MainWindow::onRenameSignal);
    connect(act_remove, &QAction::triggered, this, &MainWindow::onRemoveSignal);
    connect(act_about, &QAction::triggered, this, &MainWindow::onAbout);
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);

    connect(file_panel_, &FileListPanel::selectionChanged,
            this, &MainWindow::onFileSelectionChanged);
    connect(file_panel_, &FileListPanel::removeRequested,
            this, &MainWindow::onFileRemoveRequested);
    connect(file_panel_, &FileListPanel::detailsRequested,
            this, &MainWindow::onFileDetailsRequested);
    connect(list_panel_, &SignalListPanel::selectionChanged,
            this, &MainWindow::onSignalSelectionChanged);
    connect(list_panel_, &SignalListPanel::renameRequested,
            this, &MainWindow::onRenameRequested);
    connect(list_panel_, &SignalListPanel::addRequested,
            this, &MainWindow::onAddRequested);
    connect(list_panel_, &SignalListPanel::removeRequested,
            this, &MainWindow::onRemoveRequested);
    connect(plot_, &SignalPlotWidget::editStarted,
            this, &MainWindow::onPlotEditStarted);
    connect(plot_, &SignalPlotWidget::signalChanged,
            this, &MainWindow::onPlotChanged);
    connect(plot_, &SignalPlotWidget::cursorMoved,
            this, &MainWindow::onCursorMoved);
    connect(table_panel_, &SignalTablePanel::editStarted,
            this, &MainWindow::onTableEditStarted);
    connect(table_panel_, &SignalTablePanel::contentChanged,
            this, &MainWindow::onTableChanged);
    connect(table_panel_, &SignalTablePanel::interpolationChanged,
            this, &MainWindow::onSignalInterpolationChanged);

    update_undo_action();
    refresh_status();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (!event->mimeData()->hasUrls()) {
        return;
    }
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.toLocalFile().endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (path.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)) {
            paths.push_back(path);
        }
    }
    if (!paths.isEmpty()) {
        open_paths(paths);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        onRemoveSignal();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onOpen() {
    const QStringList paths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("Open CSV files"),
        {},
        QStringLiteral("CSV files (*.csv)"));
    open_paths(paths);
}

void MainWindow::onSave() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        show_error(QStringLiteral("Nothing to save"),
                   QStringLiteral("Load a CSV file first."));
        return;
    }

    sync_active_document_from_service();
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    const QString initial_path = document.path.isEmpty() ? QStringLiteral("signals.csv") : document.path;

    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save current CSV"),
        initial_path,
        QStringLiteral("CSV files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }

    const auto result = service_.save_to(std::filesystem::path(path.toStdString()));
    if (!result.is_ok()) {
        show_error(QStringLiteral("Export failed"), QString::fromStdString(result.message));
        return;
    }

    document.path = path;
    document.display_name = QFileInfo(path).fileName();
    document.dirty = false;
    clear_undo_history();
    refresh_file_panel();
    refresh_status(QStringLiteral("Saved %1").arg(path));
}

void MainWindow::onUndo() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }

    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (document.undo_stack.empty()) {
        return;
    }

    const UndoState state = std::move(document.undo_stack.back());
    document.undo_stack.pop_back();
    service_.set_library(state.library);
    sync_active_document_from_service();
    document.dirty = !document.undo_stack.empty();

    list_panel_->set_library(&service_.library());
    if (!service_.library().empty()) {
        const int bounded_index = std::clamp(state.selected_signal_index, 0,
                                             static_cast<int>(service_.library().size()) - 1);
        list_panel_->select(bounded_index);
    }
    rebind_plot();
    update_undo_action();
    refresh_file_panel();
    refresh_status(QStringLiteral("Undo applied"));
}

void MainWindow::onNewSignal() {
    onAddRequested();
}

void MainWindow::onRemoveSignal() {
    onRemoveRequested(list_panel_->current_index());
}

void MainWindow::onRenameSignal() {
    if (active_document_index_ < 0 || service_.library().empty()) {
        return;
    }
    list_panel_->begin_rename_current();
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this,
        QStringLiteral("About Signal Editor"),
        QStringLiteral("<h3>Signal Editor</h3>"
                       "<p>Multi-file waveform editor built with C++23 and Qt 6.</p>"
                       "<p>Current implementation supports CSV workspaces, enumerated-state signals, file switching, renaming, undo, waypoint drag/edit, point insertion and Gaussian brushing for numeric curves.</p>"));
}

void MainWindow::onFileSelectionChanged(int index) {
    if (switching_document_) {
        return;
    }
    activate_document(index);
}

void MainWindow::onSignalSelectionChanged(int /*index*/) {
    rebind_plot();
}

void MainWindow::onFileRemoveRequested(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }

    const QString file_name = documents_[static_cast<std::size_t>(index)].display_name;
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Remove file from workspace"),
        QStringLiteral("Remove %1 from the current workspace?\nThe file on disk will not be deleted.")
            .arg(file_name));
    if (answer != QMessageBox::Yes) {
        return;
    }

    documents_.erase(documents_.begin() + index);
    if (documents_.empty()) {
        active_document_index_ = -1;
        service_.clear();
        list_panel_->set_library(nullptr);
        plot_->set_signal(nullptr);
        table_panel_->set_signal(nullptr);
        refresh_file_panel();
        update_undo_action();
        refresh_status(QStringLiteral("Removed %1 from workspace").arg(file_name));
        return;
    }

    int next_index = index;
    if (index <= active_document_index_) {
        next_index = std::max(0, index - (index == active_document_index_ ? 1 : 0));
    }
    next_index = std::clamp(next_index, 0, static_cast<int>(documents_.size()) - 1);
    refresh_file_panel();
    activate_document(next_index);
    refresh_status(QStringLiteral("Removed %1 from workspace").arg(file_name));
}

void MainWindow::onFileDetailsRequested(int index) {
    show_file_details(index);
}

void MainWindow::onRenameRequested(int index, const QString& new_name) {
    if (active_document_index_ < 0 || index < 0) {
        return;
    }

    push_undo_state();
    const auto result = service_.rename_signal(static_cast<std::size_t>(index), new_name.toStdString());
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(QStringLiteral("Rename failed"), QString::fromStdString(result.message));
        list_panel_->refresh();
        return;
    }

    mark_active_document_dirty();
    list_panel_->refresh();
    rebind_plot();
}

void MainWindow::onAddRequested() {
    if (active_document_index_ < 0) {
        show_error(QStringLiteral("No active file"),
                   QStringLiteral("Load a CSV file before creating a signal."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("New signal"));
    dlg.resize(560, 560);
    auto* layout = new QVBoxLayout(&dlg);
    auto* common_form = new QFormLayout();

    auto* name_edit = new QLineEdit(QStringLiteral("new_signal"), &dlg);
    auto* waveform_box = new QComboBox(&dlg);
    auto* interpolation_box = new QComboBox(&dlg);
    auto* t_start = new QDoubleSpinBox(&dlg);
    auto* t_end = new QDoubleSpinBox(&dlg);
    auto* n_samples = new QSpinBox(&dlg);

    waveform_box->addItem(QStringLiteral("Constant"));
    waveform_box->addItem(QStringLiteral("Sine"));
    waveform_box->addItem(QStringLiteral("Cosine"));
    waveform_box->addItem(QStringLiteral("Pulse"));
    waveform_box->addItem(QStringLiteral("Sawtooth"));
    waveform_box->addItem(QStringLiteral("Triangle"));
    waveform_box->addItem(QStringLiteral("Ramp"));
    waveform_box->addItem(QStringLiteral("Enumerated"));
    interpolation_box->addItem(QStringLiteral("Linear"));
    interpolation_box->addItem(QStringLiteral("Step"));

    t_start->setRange(-1e9, 1e9);
    t_start->setValue(0.0);
    t_start->setDecimals(4);
    t_end->setRange(-1e9, 1e9);
    t_end->setValue(1.0);
    t_end->setDecimals(4);
    n_samples->setRange(2, 1000000);
    n_samples->setValue(101);

    common_form->addRow(QStringLiteral("Name"), name_edit);
    common_form->addRow(QStringLiteral("Waveform"), waveform_box);
    common_form->addRow(QStringLiteral("Interpolation"), interpolation_box);
    common_form->addRow(QStringLiteral("t start"), t_start);
    common_form->addRow(QStringLiteral("t end"), t_end);
    common_form->addRow(QStringLiteral("# samples"), n_samples);
    layout->addLayout(common_form);

    auto configure_spin = [](QDoubleSpinBox* box, double value) {
        box->setRange(-1e9, 1e9);
        box->setDecimals(6);
        box->setValue(value);
    };

    auto* params_stack = new QStackedWidget(&dlg);

    auto* constant_page = new QWidget(params_stack);
    auto* constant_form = new QFormLayout(constant_page);
    auto* constant_level = new QDoubleSpinBox(constant_page);
    configure_spin(constant_level, 0.0);
    constant_form->addRow(QStringLiteral("Level"), constant_level);
    params_stack->addWidget(constant_page);

    auto* sine_page = new QWidget(params_stack);
    auto* sine_form = new QFormLayout(sine_page);
    auto* sine_amplitude = new QDoubleSpinBox(sine_page);
    auto* sine_offset = new QDoubleSpinBox(sine_page);
    auto* sine_frequency = new QDoubleSpinBox(sine_page);
    auto* sine_phase = new QDoubleSpinBox(sine_page);
    configure_spin(sine_amplitude, 1.0);
    configure_spin(sine_offset, 0.0);
    configure_spin(sine_frequency, 1.0);
    configure_spin(sine_phase, 0.0);
    sine_frequency->setRange(1e-6, 1e9);
    sine_phase->setRange(-3600.0, 3600.0);
    sine_form->addRow(QStringLiteral("Amplitude"), sine_amplitude);
    sine_form->addRow(QStringLiteral("Offset"), sine_offset);
    sine_form->addRow(QStringLiteral("Frequency [Hz]"), sine_frequency);
    sine_form->addRow(QStringLiteral("Phase [deg]"), sine_phase);
    params_stack->addWidget(sine_page);

    auto* cosine_page = new QWidget(params_stack);
    auto* cosine_form = new QFormLayout(cosine_page);
    auto* cosine_amplitude = new QDoubleSpinBox(cosine_page);
    auto* cosine_offset = new QDoubleSpinBox(cosine_page);
    auto* cosine_frequency = new QDoubleSpinBox(cosine_page);
    auto* cosine_phase = new QDoubleSpinBox(cosine_page);
    configure_spin(cosine_amplitude, 1.0);
    configure_spin(cosine_offset, 0.0);
    configure_spin(cosine_frequency, 1.0);
    configure_spin(cosine_phase, 0.0);
    cosine_frequency->setRange(1e-6, 1e9);
    cosine_phase->setRange(-3600.0, 3600.0);
    cosine_form->addRow(QStringLiteral("Amplitude"), cosine_amplitude);
    cosine_form->addRow(QStringLiteral("Offset"), cosine_offset);
    cosine_form->addRow(QStringLiteral("Frequency [Hz]"), cosine_frequency);
    cosine_form->addRow(QStringLiteral("Phase [deg]"), cosine_phase);
    params_stack->addWidget(cosine_page);

    auto* pulse_page = new QWidget(params_stack);
    auto* pulse_form = new QFormLayout(pulse_page);
    auto* pulse_low = new QDoubleSpinBox(pulse_page);
    auto* pulse_high = new QDoubleSpinBox(pulse_page);
    auto* pulse_frequency = new QDoubleSpinBox(pulse_page);
    auto* pulse_duty = new QDoubleSpinBox(pulse_page);
    auto* pulse_phase = new QDoubleSpinBox(pulse_page);
    configure_spin(pulse_low, 0.0);
    configure_spin(pulse_high, 1.0);
    configure_spin(pulse_frequency, 1.0);
    configure_spin(pulse_duty, 50.0);
    configure_spin(pulse_phase, 0.0);
    pulse_frequency->setRange(1e-6, 1e9);
    pulse_duty->setRange(0.1, 99.9);
    pulse_phase->setRange(-3600.0, 3600.0);
    pulse_form->addRow(QStringLiteral("Low level"), pulse_low);
    pulse_form->addRow(QStringLiteral("High level"), pulse_high);
    pulse_form->addRow(QStringLiteral("Frequency [Hz]"), pulse_frequency);
    pulse_form->addRow(QStringLiteral("Duty cycle [%]"), pulse_duty);
    pulse_form->addRow(QStringLiteral("Phase [deg]"), pulse_phase);
    params_stack->addWidget(pulse_page);

    auto* saw_page = new QWidget(params_stack);
    auto* saw_form = new QFormLayout(saw_page);
    auto* saw_min = new QDoubleSpinBox(saw_page);
    auto* saw_max = new QDoubleSpinBox(saw_page);
    auto* saw_frequency = new QDoubleSpinBox(saw_page);
    auto* saw_phase = new QDoubleSpinBox(saw_page);
    configure_spin(saw_min, -1.0);
    configure_spin(saw_max, 1.0);
    configure_spin(saw_frequency, 1.0);
    configure_spin(saw_phase, 0.0);
    saw_frequency->setRange(1e-6, 1e9);
    saw_phase->setRange(-3600.0, 3600.0);
    saw_form->addRow(QStringLiteral("Min value"), saw_min);
    saw_form->addRow(QStringLiteral("Max value"), saw_max);
    saw_form->addRow(QStringLiteral("Frequency [Hz]"), saw_frequency);
    saw_form->addRow(QStringLiteral("Phase [deg]"), saw_phase);
    params_stack->addWidget(saw_page);

    auto* triangle_page = new QWidget(params_stack);
    auto* triangle_form = new QFormLayout(triangle_page);
    auto* triangle_min = new QDoubleSpinBox(triangle_page);
    auto* triangle_max = new QDoubleSpinBox(triangle_page);
    auto* triangle_frequency = new QDoubleSpinBox(triangle_page);
    auto* triangle_phase = new QDoubleSpinBox(triangle_page);
    configure_spin(triangle_min, -1.0);
    configure_spin(triangle_max, 1.0);
    configure_spin(triangle_frequency, 1.0);
    configure_spin(triangle_phase, 0.0);
    triangle_frequency->setRange(1e-6, 1e9);
    triangle_phase->setRange(-3600.0, 3600.0);
    triangle_form->addRow(QStringLiteral("Min value"), triangle_min);
    triangle_form->addRow(QStringLiteral("Max value"), triangle_max);
    triangle_form->addRow(QStringLiteral("Frequency [Hz]"), triangle_frequency);
    triangle_form->addRow(QStringLiteral("Phase [deg]"), triangle_phase);
    params_stack->addWidget(triangle_page);

    auto* ramp_page = new QWidget(params_stack);
    auto* ramp_form = new QFormLayout(ramp_page);
    auto* ramp_start = new QDoubleSpinBox(ramp_page);
    auto* ramp_end = new QDoubleSpinBox(ramp_page);
    configure_spin(ramp_start, 0.0);
    configure_spin(ramp_end, 1.0);
    ramp_form->addRow(QStringLiteral("Start value"), ramp_start);
    ramp_form->addRow(QStringLiteral("End value"), ramp_end);
    params_stack->addWidget(ramp_page);

    auto* enum_page = new QWidget(params_stack);
    auto* enum_layout = new QVBoxLayout(enum_page);
    enum_layout->setContentsMargins(0, 0, 0, 0);
    enum_layout->setSpacing(10);
    auto* enum_intro = new QLabel(
        QStringLiteral("Define one state per line using LABEL:NUMERIC_VALUE. Example:\nTRUE:1\nFALSE:0\nThe initial state can be left empty to use the first mapping entry."),
        enum_page);
    enum_intro->setWordWrap(true);
    auto* enum_mapping = new QTextEdit(enum_page);
    enum_mapping->setPlaceholderText(QStringLiteral("TRUE:1\nFALSE:0"));
    enum_mapping->setMinimumHeight(140);
    auto* enum_initial_form = new QFormLayout();
    auto* enum_initial_label = new QLineEdit(enum_page);
    enum_initial_label->setPlaceholderText(QStringLiteral("TRUE"));
    enum_initial_form->addRow(QStringLiteral("Initial state"), enum_initial_label);
    enum_layout->addWidget(enum_intro);
    enum_layout->addWidget(enum_mapping);
    enum_layout->addLayout(enum_initial_form);
    enum_layout->addStretch(1);
    params_stack->addWidget(enum_page);

    layout->addWidget(params_stack);
    QObject::connect(waveform_box, qOverload<int>(&QComboBox::currentIndexChanged),
                     &dlg, [params_stack, interpolation_box](int index) {
                         params_stack->setCurrentIndex(index);
                         const bool is_enum = static_cast<WaveformKind>(index) == WaveformKind::Enumerated;
                         if (is_enum) {
                             interpolation_box->setCurrentIndex(static_cast<int>(Signal::InterpolationMode::Step));
                         }
                         interpolation_box->setEnabled(!is_enum);
                     });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const auto waveform = static_cast<WaveformKind>(waveform_box->currentIndex());
    std::array<double, 4> params_a{0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> params_b{0.0, 0.0, 0.0, 0.0};

    switch (waveform) {
    case WaveformKind::Constant:
        params_a[0] = constant_level->value();
        break;
    case WaveformKind::Sine:
        params_a = {sine_amplitude->value(), sine_offset->value(), sine_frequency->value(), sine_phase->value()};
        break;
    case WaveformKind::Cosine:
        params_a = {cosine_amplitude->value(), cosine_offset->value(), cosine_frequency->value(), cosine_phase->value()};
        break;
    case WaveformKind::Pulse:
        params_a = {pulse_low->value(), pulse_high->value(), pulse_frequency->value(), pulse_duty->value()};
        params_b[0] = pulse_phase->value();
        break;
    case WaveformKind::Sawtooth:
        params_a = {saw_min->value(), saw_max->value(), saw_frequency->value(), saw_phase->value()};
        break;
    case WaveformKind::Triangle:
        params_a = {triangle_min->value(), triangle_max->value(), triangle_frequency->value(), triangle_phase->value()};
        break;
    case WaveformKind::Ramp:
        params_a = {ramp_start->value(), ramp_end->value(), 0.0, 0.0};
        break;
    case WaveformKind::Enumerated:
        break;
    }

    try {
        Signal signal = waveform == WaveformKind::Enumerated
            ? generate_enumerated_signal(name_edit->text(),
                                         t_start->value(),
                                         t_end->value(),
                                         static_cast<std::size_t>(n_samples->value()),
                                         parse_enumeration_definition(enum_mapping->toPlainText()),
                                         enum_initial_label->text())
            : generate_waveform_signal(
                waveform,
                name_edit->text(),
                t_start->value(),
                t_end->value(),
                static_cast<std::size_t>(n_samples->value()),
                params_a,
                params_b);

        if (!signal.is_enumerated()) {
            signal.set_interpolation(static_cast<Signal::InterpolationMode>(interpolation_box->currentIndex()));
        }

        push_undo_state();
        const auto result = service_.add_signal(std::move(signal));
        if (!result.is_ok()) {
            discard_last_undo_state();
            show_error(QStringLiteral("Create failed"), QString::fromStdString(result.message));
            return;
        }

        mark_active_document_dirty();
        list_panel_->refresh();
        list_panel_->select(static_cast<int>(service_.library().size()) - 1);
        rebind_plot();
    } catch (const std::exception& ex) {
        show_error(QStringLiteral("Create failed"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::onRemoveRequested(int index) {
    if (active_document_index_ < 0 || index < 0 || index >= static_cast<int>(service_.library().size())) {
        return;
    }

    push_undo_state();
    const auto result = service_.remove_signal(static_cast<std::size_t>(index));
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(QStringLiteral("Remove failed"), QString::fromStdString(result.message));
        return;
    }

    mark_active_document_dirty();
    list_panel_->refresh();
    if (!service_.library().empty()) {
        const int next_index = std::min(index, static_cast<int>(service_.library().size()) - 1);
        list_panel_->select(next_index);
    }
    rebind_plot();
}

void MainWindow::onPlotEditStarted() {
    push_undo_state();
}

void MainWindow::onPlotChanged() {
    if (active_document_index_ < 0) {
        return;
    }
    mark_active_document_dirty();
    if (!plot_->is_drag_active()) {
        list_panel_->refresh();
        plot_->refresh();
        table_panel_->refresh();
    }
    refresh_status();
}

void MainWindow::onTableEditStarted() {
    push_undo_state();
}

void MainWindow::onTableChanged() {
    if (active_document_index_ < 0) {
        return;
    }

    const int signal_index = list_panel_->current_index();
    if (signal_index < 0 || signal_index >= static_cast<int>(service_.library().size())) {
        return;
    }

    const auto& current_signal = service_.library().at(static_cast<std::size_t>(signal_index));
    Signal replacement(current_signal.name(), table_panel_->samples(), current_signal.interpolation());
    if (current_signal.is_enumerated()) {
        replacement.set_enumeration(current_signal.enumeration());
    }
    const auto result = service_.replace_signal(
        static_cast<std::size_t>(signal_index),
        std::move(replacement));
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(QStringLiteral("Table edit failed"), QString::fromStdString(result.message));
        rebind_plot();
        return;
    }

    mark_active_document_dirty();
    list_panel_->refresh();
    rebind_plot();
}

void MainWindow::onSignalInterpolationChanged(int mode) {
    if (active_document_index_ < 0) {
        return;
    }

    const int signal_index = list_panel_->current_index();
    if (signal_index < 0 || signal_index >= static_cast<int>(service_.library().size())) {
        return;
    }

    push_undo_state();
    const auto result = service_.set_signal_interpolation(
        static_cast<std::size_t>(signal_index),
        static_cast<Signal::InterpolationMode>(mode));
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(QStringLiteral("Interpolation change failed"), QString::fromStdString(result.message));
        rebind_plot();
        return;
    }

    mark_active_document_dirty();
    list_panel_->refresh();
    plot_->refresh();
    table_panel_->refresh();
}

void MainWindow::onCursorMoved(double t, double y) {
    const int signal_index = list_panel_->current_index();
    const Signal* signal = (signal_index >= 0 && signal_index < static_cast<int>(service_.library().size()))
        ? &service_.library().at(static_cast<std::size_t>(signal_index))
        : nullptr;
    refresh_status(QStringLiteral("t = %1   y = %2").arg(t, 0, 'f', 4).arg(format_signal_value(signal, y)));
}

void MainWindow::open_paths(const QStringList& paths) {
    for (const QString& path : paths) {
        if (!path.isEmpty()) {
            load_csv(path);
        }
    }
}

void MainWindow::load_csv(const QString& path) {
    const auto result = service_.load_from(std::filesystem::path(path.toStdString()));
    if (!result.is_ok()) {
        show_error(QStringLiteral("Load failed"),
                   QStringLiteral("%1\n\n%2").arg(path, QString::fromStdString(result.message)));
        return;
    }

    sync_active_document_from_service();

    LoadedDocument document;
    document.path = path;
    document.display_name = QFileInfo(path).fileName();
    document.library = service_.library();
    document.undo_stack.clear();
    document.dirty = false;
    documents_.push_back(std::move(document));

    refresh_file_panel();
    activate_document(static_cast<int>(documents_.size()) - 1);
    refresh_status(QStringLiteral("Loaded %1").arg(path));
}

void MainWindow::sync_active_document_from_service() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    documents_[static_cast<std::size_t>(active_document_index_)].library = service_.library();
}

void MainWindow::activate_document(int index, int preferred_signal_index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        active_document_index_ = -1;
        service_.clear();
        list_panel_->set_library(nullptr);
        plot_->set_signal(nullptr);
        table_panel_->set_signal(nullptr);
        update_undo_action();
        refresh_status();
        return;
    }

    sync_active_document_from_service();

    switching_document_ = true;
    active_document_index_ = index;
    service_.set_library(documents_[static_cast<std::size_t>(index)].library);
    list_panel_->set_library(&service_.library());
    if (!service_.library().empty()) {
        const int bounded_index = std::clamp(preferred_signal_index, 0,
                                             static_cast<int>(service_.library().size()) - 1);
        list_panel_->select(bounded_index);
    }
    rebind_plot();
    file_panel_->select(index);
    switching_document_ = false;
    update_undo_action();
    refresh_status();
}

void MainWindow::mark_active_document_dirty(bool dirty) {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    sync_active_document_from_service();
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    document.dirty = dirty;
    update_undo_action();
    refresh_file_panel();
}

void MainWindow::push_undo_state() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }

    sync_active_document_from_service();
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    document.undo_stack.push_back(UndoState{document.library, list_panel_->current_index()});
    update_undo_action();
}

void MainWindow::discard_last_undo_state() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }

    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (!document.undo_stack.empty()) {
        document.undo_stack.pop_back();
    }
    update_undo_action();
}

void MainWindow::clear_undo_history() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }

    documents_[static_cast<std::size_t>(active_document_index_)].undo_stack.clear();
    update_undo_action();
}

void MainWindow::show_file_details(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }

    const auto& document = documents_[static_cast<std::size_t>(index)];
    QFileInfo file_info(document.path);

    std::size_t total_samples = 0;
    double time_min = 0.0;
    double time_max = 0.0;
    bool has_samples = false;
    QStringList signal_summaries;
    signal_summaries.reserve(static_cast<int>(document.library.size()));
    for (const auto& signal : document.library.items()) {
        total_samples += signal.size();
        if (!signal.empty()) {
            if (!has_samples) {
                time_min = signal.t_min();
                time_max = signal.t_max();
                has_samples = true;
            } else {
                time_min = std::min(time_min, signal.t_min());
                time_max = std::max(time_max, signal.t_max());
            }
        }
        signal_summaries.push_back(describe_signal_line(signal));
    }

    QString details;
    details += QStringLiteral("File: %1\n").arg(document.display_name);
    details += QStringLiteral("Path: %1\n").arg(document.path);
    details += QStringLiteral("Folder: %1\n").arg(file_info.absolutePath());
    details += QStringLiteral("Exists on disk: %1\n")
        .arg(file_info.exists() ? QStringLiteral("yes") : QStringLiteral("no"));
    if (file_info.exists()) {
        details += QStringLiteral("Size: %1 bytes\n").arg(file_info.size());
        details += QStringLiteral("Last modified: %1\n")
            .arg(file_info.lastModified().toString(Qt::ISODate));
    }
    details += QStringLiteral("Signals loaded: %1\n")
        .arg(static_cast<qulonglong>(document.library.size()));
    details += QStringLiteral("Total samples: %1\n")
        .arg(static_cast<qulonglong>(total_samples));
    details += QStringLiteral("Workspace modified: %1\n")
        .arg(document.dirty ? QStringLiteral("yes") : QStringLiteral("no"));
    details += QStringLiteral("Undo steps available: %1\n")
        .arg(static_cast<qulonglong>(document.undo_stack.size()));
    if (has_samples) {
        details += QStringLiteral("Global time range: [%1, %2]\n")
            .arg(time_min, 0, 'f', 4)
            .arg(time_max, 0, 'f', 4);
    }
    details += QStringLiteral("\nSignals:\n");
    details += signal_summaries.isEmpty()
        ? QStringLiteral("- none")
        : signal_summaries.join(QStringLiteral("\n"));

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("File details"));
    dialog.resize(760, 520);
    auto* layout = new QVBoxLayout(&dialog);
    auto* text_view = new QTextEdit(&dialog);
    text_view->setReadOnly(true);
    text_view->setPlainText(details);
    layout->addWidget(text_view);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::refresh_file_panel() {
    std::vector<FileListPanel::FileItem> items;
    items.reserve(documents_.size());
    for (const auto& document : documents_) {
        items.push_back(FileListPanel::FileItem{document.display_name, document.path, document.dirty});
    }

    switching_document_ = true;
    file_panel_->set_files(items);
    if (active_document_index_ >= 0) {
        file_panel_->select(active_document_index_);
    }
    switching_document_ = false;
}

void MainWindow::rebind_plot() {
    const int idx = list_panel_->current_index();
    if (idx < 0 || idx >= static_cast<int>(service_.library().size())) {
        plot_->set_signal(nullptr);
        table_panel_->set_signal(nullptr);
    } else {
        auto* signal = &service_.library().at(static_cast<std::size_t>(idx));
        plot_->set_signal(signal);
        table_panel_->set_signal(signal);
    }
    refresh_status();
}

void MainWindow::update_undo_action() {
    if (undo_action_ == nullptr) {
        return;
    }
    const bool enabled = active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size()) &&
        !documents_[static_cast<std::size_t>(active_document_index_)].undo_stack.empty();
    undo_action_->setEnabled(enabled);
}

void MainWindow::refresh_status(const QString& transient_message) {
    if (!transient_message.isEmpty()) {
        statusBar()->showMessage(transient_message, 4000);
    }

    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        if (workspace_title_label_ != nullptr) {
            workspace_title_label_->setText(QStringLiteral("Signal Editor Workspace"));
        }
        if (workspace_meta_label_ != nullptr) {
            workspace_meta_label_->setText(QStringLiteral("No active document"));
        }
        if (workspace_hint_label_ != nullptr) {
            workspace_hint_label_->setText(QStringLiteral("Load CSV files, switch between workspace documents, and sculpt signals through the plot or table."));
        }
        if (transient_message.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("No file loaded"));
        }
        return;
    }

    const auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    const QString dirty_text = document.dirty ? QStringLiteral("Modified") : QStringLiteral("Synced");
    const QString undo_text = QStringLiteral("Undo %1").arg(static_cast<qulonglong>(document.undo_stack.size()));
    const QString summary = QStringLiteral("%1 file(s) loaded | %2 | %3 | %4")
        .arg(static_cast<qulonglong>(documents_.size()))
        .arg(document.display_name)
        .arg(summarize_counts(service_.library()))
        .arg(dirty_text);

    if (workspace_title_label_ != nullptr) {
        workspace_title_label_->setText(document.display_name);
    }
    if (workspace_meta_label_ != nullptr) {
        workspace_meta_label_->setText(QStringLiteral("%1 | %2 | %3")
            .arg(summarize_counts(service_.library()))
            .arg(dirty_text)
            .arg(undo_text));
    }
    const int signal_index = list_panel_->current_index();
    const Signal* active_signal = (signal_index >= 0 && signal_index < static_cast<int>(service_.library().size()))
        ? &service_.library().at(static_cast<std::size_t>(signal_index))
        : nullptr;
    if (workspace_hint_label_ != nullptr) {
        if (active_signal != nullptr && active_signal->is_enumerated()) {
            workspace_hint_label_->setText(QStringLiteral("Enumerated signals snap to user-defined states, expose label-based editing in the table, and render the Y axis with textual states."));
        } else {
            workspace_hint_label_->setText(QStringLiteral("Drag points, use Shift+drag for Gaussian brushing, or fine-tune samples in the table below."));
        }
    }

    if (transient_message.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("%1 | %2")
            .arg(summary)
            .arg(undo_text));
    }
}

void MainWindow::show_error(const QString& title, const QString& message) {
    QMessageBox::warning(this, title, message);
}

}  // namespace myprj::signal_editor::adapters::qt
