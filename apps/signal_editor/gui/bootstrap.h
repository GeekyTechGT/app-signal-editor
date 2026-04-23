#pragma once

#include <QApplication>
#include <QEvent>
#include <QTranslator>

#include <memory>

namespace signal_editor {
class SignalEditorService;
}  // namespace signal_editor

namespace signal_editor::gui {

/**
 * @brief QApplication wrapper that reports uncaught event-loop exceptions.
 *
 * The class keeps exception handling at the Qt boundary, before control returns
 * to platform event dispatch code where C++ exceptions would be unsafe.
 */
class SafeApplication final : public QApplication {
public:
    using QApplication::QApplication;

    /**
     * @brief Delivers a Qt event and converts uncaught exceptions to a dialog.
     * @param receiver Object receiving the event.
     * @param event Event passed by Qt.
     * @return The value returned by QApplication::notify(), or false after an exception.
     */
    bool notify(QObject* receiver, QEvent* event) override;
};

/**
 * @brief Installs the persisted UI translation before any visible widgets exist.
 *
 * The splash screen and main window both use translated strings during startup,
 * so the bootstrap translator must be installed before either widget is built.
 *
 * @param app Running Qt application instance.
 * @return Ownership handle for the installed translator, or nullptr when no
 *         bootstrap translator is needed.
 */
std::unique_ptr<QTranslator> install_bootstrap_translator(QApplication& app);

/**
 * @brief Starts the GUI shell, using the branded splash screen when available.
 * @param service Application service injected into the main window.
 */
void launch_desktop_shell(signal_editor::SignalEditorService& service);

}  // namespace signal_editor::gui
