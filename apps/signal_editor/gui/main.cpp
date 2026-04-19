/**
 * @file
 * @brief GUI composition root for the Signal Editor desktop application.
 *
 * This translation unit stays intentionally thin. It bootstraps the Qt
 * application, restores the persisted language early enough for the splash
 * screen and main window, applies a safe default theme, and wires the service
 * layer to the desktop shell.
 */

#include "signal_editor/adapters/qt/constants.hpp"
#include "signal_editor/adapters/filesystem/signal_file_repository.h"
#include "signal_editor/adapters/qt/main_window.h"
#include "signal_editor/adapters/qt/theme.h"
#include "signal_editor/api/signal_editor_api.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSettings>
#include <QTimer>
#include <QTranslator>

#include <memory>

#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
#include <splash_screen_widget.hpp>
#endif

namespace {
namespace ui = signal_editor::adapters::qt::constants;

/**
 * @brief Normalizes a persisted locale value to a two-letter language code.
 * @param language Raw locale or language string from settings.
 * @return Lowercase ISO-like code used to resolve packaged translations.
 */
QString normalize_language_code(QString language) {
    language = language.trimmed();
    if (language.isEmpty()) {
        return QStringLiteral("en");
    }

    language.replace(QLatin1Char('-'), QLatin1Char('_'));
    const qsizetype separator = language.indexOf(QLatin1Char('_'));
    if (separator > 0) {
        language = language.left(separator);
    }
    return language.left(2).toLower();
}

/**
 * @brief Reads the persisted UI language needed during application bootstrap.
 * @return Stored language code, or the Italian default for first launch.
 */
QString read_bootstrap_language() {
    QSettings settings(ui::app_id(), ui::settings_version_scope());
    return settings.value(QString::fromUtf8(ui::kSettingsKeyLanguage),
                          QStringLiteral("it")).toString();
}

/**
 * @brief Builds the list of filesystem candidates for a translation catalog.
 * @param app_dir Directory containing the running executable.
 * @param translation_base Basename prefix plus language suffix.
 * @return Ordered list of `.qm` candidates to probe before falling back to resources.
 */
QStringList translation_candidates(const QString& app_dir,
                                   const QString& translation_base) {
    QStringList candidates;
    const QDir current_dir(QDir::currentPath());
    const auto add_candidate = [&candidates](const QString& path) {
        const QString clean_path = QDir::cleanPath(path);
        if (!clean_path.isEmpty() && !candidates.contains(clean_path)) {
            candidates.push_back(clean_path);
        }
    };

    add_candidate(QDir(app_dir).filePath(
        QStringLiteral("translations/%1.qm").arg(translation_base)));
    add_candidate(QDir(app_dir).filePath(QStringLiteral("%1.qm").arg(translation_base)));
    add_candidate(QDir(app_dir).filePath(
        QStringLiteral("../translations/%1.qm").arg(translation_base)));
    add_candidate(current_dir.filePath(
        QStringLiteral("translations/%1.qm").arg(translation_base)));
    add_candidate(current_dir.filePath(QStringLiteral("%1.qm").arg(translation_base)));
    return candidates;
}

/**
 * @brief Verifies that a loaded translator contains Signal Editor strings.
 * @param translator Translator candidate under validation.
 * @return `true` when the catalog resolves at least one bootstrap string.
 */
bool translator_has_bootstrap_strings(const QTranslator& translator) {
    const QString splash_text = translator.translate("Splash", "Applying theme...");
    if (!splash_text.isEmpty() &&
        splash_text != QStringLiteral("Applying theme...")) {
        return true;
    }

    const QString window_text =
        translator.translate("MainWindow", "&New from scratch...");
    return !window_text.isEmpty() &&
           window_text != QStringLiteral("&New from scratch...");
}

/**
 * @brief Installs the persisted application translator before any UI is built.
 * @param app Running Qt application instance.
 * @return Owning handle for the installed translator, or `nullptr` for English/default startup.
 */
std::unique_ptr<QTranslator> install_bootstrap_translator(QApplication& app) {
    const QString lang = normalize_language_code(read_bootstrap_language());
    if (lang == QStringLiteral("en")) {
        return nullptr;
    }

    const QString translation_base = ui::translation_base_name() + lang;
    const QString app_dir = QCoreApplication::applicationDirPath();

    for (const QString& candidate : translation_candidates(app_dir, translation_base)) {
        auto translator = std::make_unique<QTranslator>();
        if (translator->load(candidate) && translator_has_bootstrap_strings(*translator)) {
            app.installTranslator(translator.get());
            return translator;
        }
    }

    auto translator = std::make_unique<QTranslator>();
    const QString resource_translation =
        QStringLiteral(":/i18n/%1.qm").arg(translation_base);
    if (translator->load(resource_translation) &&
        translator_has_bootstrap_strings(*translator)) {
        app.installTranslator(translator.get());
        return translator;
    }

    return nullptr;
}

#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
using namespace lib_qt_custom_widgets;

/**
 * @brief Shows the branded splash screen before revealing the main window.
 * @param service Application service injected into the main window shell.
 */
void run_with_splash(signal_editor::api::Service& service)
{
    auto* splash = new SplashScreenWidget();   // frameless top-level widget

    splash->setAppName(ui::display_name());
    splash->setAppVersion(ui::application_version());
    splash->setLogoPath(QStringLiteral(":/img/app_logo.svg"));
    splash->setCompanyName(ui::company_name());
    splash->setBackgroundColor(QColor(0x0E, 0x22, 0x33));
    splash->setBorderColor(QColor(0x67, 0xE8, 0xF9, 180));
    splash->setBorderWidth(2);
    splash->setWindowSize(QSize(ui::kSplashWidth, ui::kSplashHeight));
    splash->setMinimumDisplayDuration(ui::kSplashMinDurationMs);
    splash->setProgressMode(SplashScreenWidget::ProgressMode::Determinate);
    splash->setTotalSteps(ui::kSplashStepCount);
    splash->setStyleSheet(QStringLiteral(R"qss(
QLabel#splashAppName {
    color: #F8FAFC;
    font-size: 28px;
    font-weight: 800;
    letter-spacing: 0.4px;
}
QLabel#splashAppVersion {
    color: rgba(207, 250, 254, 0.82);
    font-size: 13px;
    font-weight: 600;
}
QLabel#splashStatusLabel {
    color: #E2E8F0;
    font-size: 13px;
    font-weight: 600;
}
QLabel#splashCompanyName {
    color: rgba(226, 232, 240, 0.88);
    font-size: 12px;
    font-weight: 600;
}
QProgressBar#splashProgressBar {
    background: rgba(148, 163, 184, 0.18);
    border: 1px solid rgba(125, 211, 252, 0.18);
    border-radius: 6px;
}
QProgressBar#splashProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #2DD4BF, stop:0.55 #22D3EE, stop:1 #FDBA74);
    border-radius: 6px;
}
)qss"));

    // Build and show the main window only after the splash is done.
    // MainWindow is heap-allocated so its lifetime is not tied to this scope.
    auto* window = new signal_editor::adapters::qt::MainWindow(service);

    QObject::connect(splash, &SplashScreenWidget::splashFinished,
                     window, [window, splash]() {
        window->showMaximized();
        splash->deleteLater();
    });

    splash->startSplash();

    // The loading text is staged to keep startup status legible even though
    // the service wiring and window construction are synchronous.
    const QStringList steps = {
        QApplication::translate("Splash", "Applying theme..."),
        QApplication::translate("Splash", "Loading workspace..."),
        QApplication::translate("Splash", "Initializing signals engine..."),
        QApplication::translate("Splash", "Ready"),
    };

    for (int i = 0; i < steps.size(); ++i) {
        QTimer::singleShot((i + 1) * ui::kSplashStepDelayMs, splash,
                           [splash, steps, i]() {
            splash->setStatusMessage(steps[i]);
            splash->incrementProgress();
            if (i == steps.size() - 1) {
                splash->finishSplash();
            }
        });
    }
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(
        signal_editor::adapters::qt::constants::app_id());
    QApplication::setOrganizationName(
        signal_editor::adapters::qt::constants::app_id());
    QApplication::setApplicationVersion(
        signal_editor::adapters::qt::constants::application_version());
    QApplication::setWindowIcon(QIcon(
        signal_editor::adapters::qt::constants::logo_resource_path()));
    auto bootstrap_translator = install_bootstrap_translator(app);

    // ── Theme ─────────────────────────────────────────────────────────────────
    // Apply a neutral default theme now so the splash inherits it.
    // MainWindow::load_persisted_settings() will re-apply the user's saved
    // theme (and install the translator) once the window is constructed.
    signal_editor::adapters::qt::apply_theme(
        app, signal_editor::adapters::qt::AppTheme::Dark);

    // ── Service wiring ────────────────────────────────────────────────────────
    signal_editor::adapters::SignalFileRepository repository;
    signal_editor::api::Service service(repository);

    // ── Splash + MainWindow ───────────────────────────────────────────────────
#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
    run_with_splash(service);
#else
    signal_editor::adapters::qt::MainWindow window(service);
    window.showMaximized();
#endif

    return app.exec();
}
