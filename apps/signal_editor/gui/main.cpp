/**
 * @file
 * @brief GUI composition root for the Signal Editor desktop application.
 *
 * This translation unit stays intentionally thin. It bootstraps the Qt
 * application, restores the persisted language early enough for the splash
 * screen and main window, applies a safe default theme, and wires the service
 * layer to the desktop shell.
 */

#include "bootstrap.h"

#include "signal_editor/adapters/qt/constants.hpp"
#include "signal_editor/adapters/filesystem/signal_file_repository.h"
#include "signal_editor/adapters/qt/branding.h"
#include "signal_editor/adapters/qt/theme.h"
#include "signal_editor/api/signal_editor_api.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    namespace ui = signal_editor::adapters::qt::constants;

    signal_editor::gui::SafeApplication app(argc, argv);
    QApplication::setApplicationName(ui::app_id());
    QApplication::setOrganizationName(ui::app_id());
    QApplication::setApplicationVersion(ui::application_version());
    QApplication::setWindowIcon(signal_editor::adapters::qt::build_application_icon());
    [[maybe_unused]] auto bootstrap_translator = signal_editor::gui::install_bootstrap_translator(app);

    // Apply a default theme before the splash and main window are created.
    signal_editor::adapters::qt::apply_theme(app, signal_editor::adapters::qt::AppTheme::Dark);

    signal_editor::adapters::SignalFileRepository repository;
    signal_editor::api::Service service(repository);
    if (bootstrap_translator != nullptr) {
        QApplication::removeTranslator(bootstrap_translator.get());
        bootstrap_translator.reset();
    }
    signal_editor::gui::launch_desktop_shell(service);

    return app.exec();
}
