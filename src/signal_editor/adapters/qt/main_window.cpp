#include "signal_editor/adapters/qt/main_window.h"

#include "signal_editor/adapters/qt/signal_list_panel.h"
#include "signal_editor/adapters/qt/signal_plot_widget.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/core/usecases/signal_editor_service.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

namespace myprj::signal_editor::adapters::qt {

MainWindow::MainWindow(SignalEditorService& service, QWidget* parent)
    : QMainWindow(parent), service_(service) {
    setWindowTitle(QStringLiteral("Signal Editor"));
    resize(1200, 760);
    setAcceptDrops(true);

    // --- Central widgets ------------------------------------------------
    auto* splitter = new QSplitter(::Qt::Horizontal, this);
    list_panel_ = new SignalListPanel(splitter);
    plot_       = new SignalPlotWidget(splitter);
    splitter->addWidget(list_panel_);
    splitter->addWidget(plot_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 940});
    setCentralWidget(splitter);

    list_panel_->set_library(&service_.library());

    // --- Menu bar -------------------------------------------------------
    auto* menu_file   = menuBar()->addMenu(QStringLiteral("&File"));
    auto* act_open    = menu_file->addAction(QStringLiteral("&Open CSV…"));
    auto* act_save    = menu_file->addAction(QStringLiteral("&Save CSV…"));
    menu_file->addSeparator();
    auto* act_quit    = menu_file->addAction(QStringLiteral("&Quit"));
    act_open->setShortcut(QKeySequence::Open);
    act_save->setShortcut(QKeySequence::Save);
    act_quit->setShortcut(QKeySequence::Quit);

    auto* menu_signal = menuBar()->addMenu(QStringLiteral("&Signal"));
    auto* act_new     = menu_signal->addAction(QStringLiteral("&New from scratch…"));
    auto* act_remove  = menu_signal->addAction(QStringLiteral("&Remove selected"));
    act_new->setShortcut(QKeySequence::New);
    act_remove->setShortcut(QKeySequence::Delete);

    auto* menu_help   = menuBar()->addMenu(QStringLiteral("&Help"));
    auto* act_about   = menu_help->addAction(QStringLiteral("&About"));

    // --- Toolbar --------------------------------------------------------
    auto* tb = addToolBar(QStringLiteral("Main"));
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));
    tb->addAction(act_open);
    tb->addAction(act_save);
    tb->addSeparator();
    tb->addAction(act_new);
    tb->addAction(act_remove);

    statusBar()->showMessage(QStringLiteral("Ready"));

    // --- Connections ----------------------------------------------------
    connect(act_open,   &QAction::triggered, this, &MainWindow::onOpen);
    connect(act_save,   &QAction::triggered, this, &MainWindow::onSave);
    connect(act_new,    &QAction::triggered, this, &MainWindow::onNewSignal);
    connect(act_remove, &QAction::triggered, this, &MainWindow::onRemoveSignal);
    connect(act_about,  &QAction::triggered, this, &MainWindow::onAbout);
    connect(act_quit,   &QAction::triggered, qApp, &QApplication::quit);

    connect(list_panel_, &SignalListPanel::selectionChanged,
            this,        &MainWindow::onSelectionChanged);
    connect(list_panel_, &SignalListPanel::renameRequested,
            this,        &MainWindow::onRenameRequested);
    connect(list_panel_, &SignalListPanel::addRequested,
            this,        &MainWindow::onAddRequested);
    connect(list_panel_, &SignalListPanel::removeRequested,
            this,        &MainWindow::onRemoveRequested);

    connect(plot_, &SignalPlotWidget::signalChanged,
            this,  &MainWindow::onPlotChanged);
    connect(plot_, &SignalPlotWidget::cursorMoved,
            this,  &MainWindow::onCursorMoved);
}

// --- Drag & drop -----------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(QStringLiteral(".csv"), ::Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString p = url.toLocalFile();
        if (p.endsWith(QStringLiteral(".csv"), ::Qt::CaseInsensitive)) {
            load_csv(p);
            return;
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == ::Qt::Key_Delete) {
        onRemoveSignal();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// --- File actions ----------------------------------------------------------
void MainWindow::onOpen() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open CSV"), {}, QStringLiteral("CSV files (*.csv)"));
    if (!path.isEmpty()) {
        load_csv(path);
    }
}

void MainWindow::onSave() {
    if (service_.library().empty()) {
        show_error(QStringLiteral("Nothing to save"),
                   QStringLiteral("The signal library is empty."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export CSV"), QStringLiteral("signals.csv"),
        QStringLiteral("CSV files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    const auto result = service_.save_to(std::filesystem::path(path.toStdString()));
    if (!result.is_ok()) {
        show_error(QStringLiteral("Export failed"), QString::fromStdString(result.message));
        return;
    }
    statusBar()->showMessage(QStringLiteral("Exported %1").arg(path), 4000);
}

void MainWindow::load_csv(const QString& path) {
    const auto result = service_.load_from(std::filesystem::path(path.toStdString()));
    if (!result.is_ok()) {
        show_error(QStringLiteral("Load failed"), QString::fromStdString(result.message));
        return;
    }
    list_panel_->set_library(&service_.library());
    list_panel_->select(0);
    rebind_plot();
    statusBar()->showMessage(
        QStringLiteral("Loaded %1 signal(s) from %2")
            .arg(static_cast<qulonglong>(service_.library().size()))
            .arg(path),
        4000);
}

// --- Signal actions --------------------------------------------------------
void MainWindow::onNewSignal() {
    onAddRequested();
}

void MainWindow::onRemoveSignal() {
    onRemoveRequested(list_panel_->current_index());
}

void MainWindow::onAbout() {
    QMessageBox::about(this, QStringLiteral("About Signal Editor"),
                       QStringLiteral("<h3>Signal Editor</h3>"
                                      "<p>A C++23 / Qt 6 reimagining of the MATLAB Signal Editor.</p>"
                                      "<p>Hexagonal architecture — the GUI layer is fully replaceable.</p>"));
}

void MainWindow::onSelectionChanged(int /*index*/) {
    rebind_plot();
}

void MainWindow::onRenameRequested(int index, const QString& new_name) {
    if (index < 0) {
        return;
    }
    const auto result = service_.rename_signal(static_cast<std::size_t>(index),
                                               new_name.toStdString());
    if (!result.is_ok()) {
        show_error(QStringLiteral("Rename failed"), QString::fromStdString(result.message));
    }
    list_panel_->refresh();
}

void MainWindow::onAddRequested() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("New signal"));
    auto* form = new QFormLayout(&dlg);

    auto* name_edit = new QLineEdit(QStringLiteral("new_signal"), &dlg);
    auto* t_start   = new QDoubleSpinBox(&dlg);
    auto* t_end     = new QDoubleSpinBox(&dlg);
    auto* n_samples = new QSpinBox(&dlg);
    auto* y0        = new QDoubleSpinBox(&dlg);

    t_start->setRange(-1e9, 1e9);
    t_start->setValue(0.0);
    t_start->setDecimals(4);
    t_end->setRange(-1e9, 1e9);
    t_end->setValue(1.0);
    t_end->setDecimals(4);
    n_samples->setRange(2, 1000000);
    n_samples->setValue(101);
    y0->setRange(-1e9, 1e9);
    y0->setValue(0.0);
    y0->setDecimals(4);

    form->addRow(QStringLiteral("Name"),         name_edit);
    form->addRow(QStringLiteral("t start"),      t_start);
    form->addRow(QStringLiteral("t end"),        t_end);
    form->addRow(QStringLiteral("# samples"),    n_samples);
    form->addRow(QStringLiteral("initial y"),    y0);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const auto result = service_.create_signal(name_edit->text().toStdString(),
                                               t_start->value(),
                                               t_end->value(),
                                               static_cast<std::size_t>(n_samples->value()),
                                               y0->value());
    if (!result.is_ok()) {
        show_error(QStringLiteral("Create failed"), QString::fromStdString(result.message));
        return;
    }
    list_panel_->refresh();
    list_panel_->select(static_cast<int>(service_.library().size()) - 1);
    rebind_plot();
}

void MainWindow::onRemoveRequested(int index) {
    if (index < 0 || index >= static_cast<int>(service_.library().size())) {
        return;
    }
    const auto result = service_.remove_signal(static_cast<std::size_t>(index));
    if (!result.is_ok()) {
        show_error(QStringLiteral("Remove failed"), QString::fromStdString(result.message));
        return;
    }
    list_panel_->refresh();
    rebind_plot();
}

void MainWindow::onPlotChanged() {
    list_panel_->refresh();
    refresh_status();
}

void MainWindow::onCursorMoved(double t, double y) {
    statusBar()->showMessage(
        QStringLiteral("t = %1   y = %2").arg(t, 0, 'f', 4).arg(y, 0, 'f', 4));
}

// --- Helpers ----------------------------------------------------------------
void MainWindow::rebind_plot() {
    const int idx = list_panel_->current_index();
    if (idx < 0 || idx >= static_cast<int>(service_.library().size())) {
        plot_->set_signal(nullptr);
    } else {
        plot_->set_signal(&service_.library().at(static_cast<std::size_t>(idx)));
    }
    refresh_status();
}

void MainWindow::refresh_status() {
    const auto n = service_.library().size();
    statusBar()->showMessage(
        QStringLiteral("%1 signal(s) in library").arg(static_cast<qulonglong>(n)));
}

void MainWindow::show_error(const QString& title, const QString& message) {
    QMessageBox::warning(this, title, message);
}

}  // namespace myprj::signal_editor::adapters::qt
