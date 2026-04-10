// Signal Editor — GUI entry point.
//
// This main is intentionally tiny: every responsibility lives behind the
// hexagonal boundaries. The only thing that happens here is wiring an
// adapter (CSV repository) to the use case (SignalEditorService) and
// handing both to the Qt main window.

#include "signal_editor/adapters/filesystem/csv_signal_repository.h"
#include "signal_editor/adapters/qt/main_window.h"
#include "signal_editor/adapters/qt/theme.h"
#include "signal_editor/api/signal_editor_api.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Signal Editor"));
    QApplication::setOrganizationName(QStringLiteral("MyPrj"));

    myprj::signal_editor::adapters::qt::apply_dark_fusion_theme(app);

    myprj::signal_editor::adapters::CsvSignalRepository repository;
    myprj::signal_editor::api::Service service(repository);

    myprj::signal_editor::adapters::qt::MainWindow window(service);
    window.show();

    return app.exec();
}
