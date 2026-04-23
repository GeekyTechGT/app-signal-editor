#include "bootstrap.h"

#include "signal_editor/adapters/qt/branding.h"
#include "signal_editor/adapters/qt/constants.hpp"
#include "signal_editor/adapters/qt/main_window.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QTimer>

#include <exception>

#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
#include <QColor>
#include <QSize>

#include <splash_screen_widget.hpp>
#endif

namespace signal_editor::gui {
namespace {
namespace ui = signal_editor::adapters::qt::constants;

/**
 * @brief Normalizes a persisted locale value to the packaged language suffix.
 * @param language Raw locale or language setting.
 * @return Lowercase two-letter language code.
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
 * @brief Reads the persisted UI language for startup-only translation loading.
 * @return Stored language code, or the default first-launch language.
 */
QString read_bootstrap_language() {
    QSettings settings(ui::app_id(), ui::settings_version_scope());
    return settings.value(QString::fromUtf8(ui::kSettingsKeyLanguage), QStringLiteral("it")).toString();
}

/**
 * @brief Builds ordered filesystem locations for a compiled translation file.
 * @param app_dir Directory containing the running executable.
 * @param translation_base Translation filename without the `.qm` extension.
 * @return Deduplicated candidate paths checked before qrc fallback.
 */
QStringList translation_candidates(const QString& app_dir, const QString& translation_base) {
    QStringList candidates;
    const QDir current_dir(QDir::currentPath());
    const auto add_candidate = [&candidates](const QString& path) {
        const QString clean_path = QDir::cleanPath(path);
        if (!clean_path.isEmpty() && !candidates.contains(clean_path)) {
            candidates.push_back(clean_path);
        }
    };

    add_candidate(QDir(app_dir).filePath(QStringLiteral("translations/%1.qm").arg(translation_base)));
    add_candidate(QDir(app_dir).filePath(QStringLiteral("%1.qm").arg(translation_base)));
    add_candidate(QDir(app_dir).filePath(QStringLiteral("../translations/%1.qm").arg(translation_base)));
    add_candidate(current_dir.filePath(QStringLiteral("translations/%1.qm").arg(translation_base)));
    add_candidate(current_dir.filePath(QStringLiteral("%1.qm").arg(translation_base)));
    return candidates;
}

/**
 * @brief Checks whether a translator contains strings required during startup.
 * @param translator Translator candidate to validate.
 * @return True when at least one bootstrap string resolves to a translated value.
 */
bool translator_has_bootstrap_strings(const QTranslator& translator) {
    const QString splash_text = translator.translate("Splash", "Applying theme...");
    if (!splash_text.isEmpty() && splash_text != QStringLiteral("Applying theme...")) {
        return true;
    }

    const QString window_text = translator.translate("MainWindow", "&New from scratch...");
    return !window_text.isEmpty() && window_text != QStringLiteral("&New from scratch...");
}

/**
 * @brief Loads a Qt stylesheet from a qrc or filesystem path.
 * @param resource_path Path to the stylesheet.
 * @return UTF-8 stylesheet contents, or an empty string when unavailable.
 */
QString load_stylesheet(const QString& resource_path) {
    QFile file(resource_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
/**
 * @brief Creates and configures the branded startup splash screen.
 * @return Heap-allocated splash screen owned by Qt after it is shown.
 */
lib_qt_custom_widgets::SplashScreenWidget* create_splash_screen() {
    auto* splash = new lib_qt_custom_widgets::SplashScreenWidget();
    splash->setAppName(ui::display_name());
    splash->setAppVersion(ui::application_version());
    splash->setLogoPath(signal_editor::adapters::qt::splash_logo_path());
    splash->setCompanyName(ui::company_name());
    splash->setBackgroundColor(QColor(0xF7, 0xFA, 0xFC));
    splash->setBorderColor(QColor(0x94, 0xA3, 0xB8, 150));
    splash->setBorderWidth(2);
    splash->setWindowSize(QSize(ui::kSplashWidth, ui::kSplashHeight));
    splash->setMinimumDisplayDuration(ui::kSplashMinDurationMs);
    splash->setProgressMode(lib_qt_custom_widgets::SplashScreenWidget::ProgressMode::Determinate);
    splash->setTotalSteps(ui::kSplashStepCount);
    splash->setStyleSheet(load_stylesheet(ui::splash_stylesheet_resource_path()));
    return splash;
}

/**
 * @brief Queues deterministic startup messages for the splash screen.
 * @param splash Splash screen receiving status and progress updates.
 */
void schedule_splash_progress(lib_qt_custom_widgets::SplashScreenWidget& splash) {
    const QStringList steps = {
        QApplication::translate("Splash", "Applying theme..."),
        QApplication::translate("Splash", "Loading workspace..."),
        QApplication::translate("Splash", "Initializing signals engine..."),
        QApplication::translate("Splash", "Ready"),
    };

    for (int i = 0; i < steps.size(); ++i) {
        QTimer::singleShot((i + 1) * ui::kSplashStepDelayMs, &splash, [&splash, steps, i]() {
            splash.setStatusMessage(steps[i]);
            splash.incrementProgress();
            if (i == steps.size() - 1) {
                splash.finishSplash();
            }
        });
    }
}
#endif

#ifndef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
/**
 * @brief Shows the main window immediately when no splash widget is available.
 * @param service Application service injected into the main window.
 */
void launch_without_splash(signal_editor::SignalEditorService& service) {
    auto* window = new signal_editor::adapters::qt::MainWindow(service);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setWindowIcon(QApplication::windowIcon());
    window->showMaximized();
}
#endif

}  // namespace

bool SafeApplication::notify(QObject* receiver, QEvent* event) {
    try {
        return QApplication::notify(receiver, event);
    } catch (const std::exception& ex) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("Unhandled exception"),
                              QStringLiteral("A runtime error occurred:\n%1")
                                  .arg(QString::fromLocal8Bit(ex.what())));
    } catch (...) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("Unhandled exception"),
                              QStringLiteral("An unknown runtime error occurred."));
    }
    return false;
}

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
    const QString resource_translation = QStringLiteral(":/i18n/%1.qm").arg(translation_base);
    if (translator->load(resource_translation) && translator_has_bootstrap_strings(*translator)) {
        app.installTranslator(translator.get());
        return translator;
    }

    return nullptr;
}

void launch_desktop_shell(signal_editor::SignalEditorService& service) {
#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
    auto* splash = create_splash_screen();
    auto* window = new signal_editor::adapters::qt::MainWindow(service);
    window->setWindowIcon(QApplication::windowIcon());

    QObject::connect(splash,
                     &lib_qt_custom_widgets::SplashScreenWidget::splashFinished,
                     window,
                     [window, splash]() {
                         window->showMaximized();
                         splash->deleteLater();
                     });

    splash->startSplash();
    schedule_splash_progress(*splash);
#else
    launch_without_splash(service);
#endif
}

}  // namespace signal_editor::gui
