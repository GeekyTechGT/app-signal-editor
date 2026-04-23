#include "signal_editor/adapters/qt/branding.h"
#include "signal_editor/adapters/qt/constants.hpp"
#include "signal_editor/adapters/qt/main_window.h"

#include "signal_editor/adapters/qt/file_list_panel.h"
#include "signal_editor/adapters/qt/icon_theme.h"
#include "signal_editor/adapters/qt/signal_list_panel.h"
#include "signal_editor/adapters/qt/signal_plot_widget.h"
#include "signal_editor/adapters/qt/signal_table_panel.h"
#include "signal_editor/adapters/filesystem/csv_signal_repository.h"
#include "signal_editor/adapters/filesystem/delimited_signal_repository.h"
#include "signal_editor/adapters/filesystem/json_signal_repository.h"
#include "signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h"
#include "signal_editor/adapters/filesystem/xlsx_signal_repository.h"
#include "signal_editor/core/usecases/signal_editor_service.h"

#include <QAction>
#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFont>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QComboBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMetaObject>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QTextEdit>
#include <QTranslator>
#include <QUrl>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QStringList>

#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
#include <settings_panel_dialog.hpp>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace signal_editor::adapters::qt {
namespace ui = signal_editor::adapters::qt::constants;

namespace {
#define tr qt_main_window_tr

QString qt_main_window_tr(const char* source_text,
                          const char* disambiguation = nullptr,
                          int n = -1) {
    return QCoreApplication::translate("MainWindow", source_text,
                                       disambiguation, n);
}

constexpr double kPi = 3.14159265358979323846;
constexpr double kSamplingEpsilon = 1e-9;
constexpr std::size_t kMaxGeneratedSamples = 50'000'000;
constexpr std::size_t kGenerationProgressStride = 100'000;

/**
 * @brief Parses a floating-point value accepting both comma and dot decimals.
 * @param text User-entered text to parse.
 * @param ok Set to `true` when the conversion succeeds.
 * @return Parsed numeric value or `0.0` on failure.
 */
double parse_flexible_double(const QString& text, bool* ok = nullptr) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0.0;
    }

    QString normalized = trimmed;
    const int last_dot = normalized.lastIndexOf('.');
    const int last_comma = normalized.lastIndexOf(',');
    const int decimal_index = std::max(last_dot, last_comma);
    for (int i = 0; i < normalized.size(); ++i) {
        if (normalized[i] == '.' || normalized[i] == ',') {
            normalized[i] = (i == decimal_index) ? QChar('.') : QChar();
        }
    }
    normalized.remove(QChar());

    bool local_ok = false;
    const double value = normalized.toDouble(&local_ok);
    if (ok != nullptr) {
        *ok = local_ok;
    }
    return value;
}

/**
 * @brief Formats a floating-point value for line-edit presentation.
 * @param locale Locale used for output formatting.
 * @param value Numeric value to display.
 * @param decimals Fractional digits to expose.
 * @return Locale-aware string without grouping separators.
 */
QString format_line_edit_double(const QLocale& locale, double value, int decimals) {
    QLocale copy(locale);
    copy.setNumberOptions(copy.numberOptions() | QLocale::OmitGroupSeparator);
    return copy.toString(value, 'f', decimals);
}

QRegularExpression flexible_decimal_pattern(bool allow_empty = false) {
    return QRegularExpression(
        allow_empty
            ? QStringLiteral(R"(^[+-]?(?:\d+([.,]\d*)?|[.,]\d+)?$)")
            : QStringLiteral(R"(^[+-]?(?:\d+([.,]\d*)?|[.,]\d+)$)"));
}

QString escape_for_qss(QString value) {
    value.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    value.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return value;
}

QString build_accessibility_qss(const lib_qt_custom_widgets::AppSettings& settings) {
    if (!settings.highContrastMode) {
        return {};
    }
    return QStringLiteral(R"qss(
QGroupBox, QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox,
QTableWidget, QListWidget, QPushButton, QToolButton {
    border-width: 2px;
}
QLabel {
    color: palette(windowText);
}
)qss");
}

QString build_font_qss(const lib_qt_custom_widgets::AppSettings& settings) {
    return QStringLiteral(R"qss(
QWidget {
    font-family: "%1";
    font-size: %2pt;
}
)qss")
        .arg(escape_for_qss(settings.fontFamily))
        .arg(settings.fontSize);
}

void set_ui_effects_enabled(bool enabled) {
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, enabled);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, enabled);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, enabled);
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, enabled);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, enabled);
}

void remove_default_settings_panel_cache() {
    QSettings settings;
    settings.remove(QStringLiteral("AppSettings"));
    settings.sync();
}

void animate_tab_reveal(QTabWidget* tabs, int index) {
    if (tabs == nullptr || index < 0 || index >= tabs->count()) {
        return;
    }
    QWidget* page = tabs->widget(index);
    if (page == nullptr) {
        return;
    }
    page->raise();
    page->update();
    tabs->tabBar()->update();
}

QString summarize_counts(const SignalLibrary& library) {
    return QCoreApplication::translate("MainWindow", "%n signal(s)", nullptr,
                                       static_cast<int>(library.size()));
}

QString import_file_filter() {
    return QCoreApplication::translate("MainWindow",
        "Supported signal files (*.csv *.tsv *.txt *.json *.xml *.xlsx);;"
        "CSV files (*.csv);;"
        "Tab-delimited files (*.tsv *.txt);;"
        "JSON files (*.json);;"
        "SpreadsheetML XML files (*.xml);;"
        "Excel workbook files (*.xlsx)");
}

QIcon application_window_icon() {
    return QApplication::windowIcon();
}

void apply_application_icon(QWidget* widget) {
    if (widget == nullptr) {
        return;
    }
    const QIcon icon = application_window_icon();
    if (!icon.isNull()) {
        widget->setWindowIcon(icon);
    }
}

QPixmap application_logo_pixmap(const QSize& size = QSize(96, 96)) {
    QPixmap pixmap = render_application_logo(size);
    if (!pixmap.isNull()) {
        return pixmap;
    }
    const QIcon icon = application_window_icon();
    if (!icon.isNull()) {
        return icon.pixmap(size);
    }
    return {};
}

QString export_file_filter() {
    return QCoreApplication::translate("MainWindow",
        "Supported export files (*.csv *.tsv *.txt *.json *.xml *.xlsx);;"
        "CSV files (*.csv);;"
        "Tab-delimited files (*.tsv *.txt);;"
        "JSON files (*.json);;"
        "SpreadsheetML XML files (*.xml);;"
        "Excel workbook files (*.xlsx)");
}

bool is_supported_import_path(const QString& path) {
    const QString lowered = QFileInfo(path).suffix().toLower();
    return lowered == QStringLiteral("csv") ||
           lowered == QStringLiteral("tsv") ||
           lowered == QStringLiteral("txt") ||
           lowered == QStringLiteral("json") ||
           lowered == QStringLiteral("xml") ||
           lowered == QStringLiteral("xlsx");
}

DocumentFormat document_format_for_path(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().trimmed().toLower();
    if (suffix == QStringLiteral("csv")) {
        return DocumentFormat::Csv;
    }
    if (suffix == QStringLiteral("tsv") || suffix == QStringLiteral("txt")) {
        return DocumentFormat::Delimited;
    }
    if (suffix == QStringLiteral("json")) {
        return DocumentFormat::Json;
    }
    if (suffix == QStringLiteral("xml")) {
        return DocumentFormat::SpreadsheetXml;
    }
    if (suffix == QStringLiteral("xlsx")) {
        return DocumentFormat::Xlsx;
    }
    return DocumentFormat::Unknown;
}

QString normalize_language_code(QString language) {
    language = language.trimmed();
    if (language.isEmpty()) {
        return QStringLiteral("en");
    }

    language.replace(QLatin1Char('-'), QLatin1Char('_'));
    const int separator = language.indexOf(QLatin1Char('_'));
    if (separator > 0) {
        language = language.left(separator);
    }
    return language.left(2).toLower();
}

bool translator_has_app_ui_strings(const QTranslator& translator) {
    const QString main_window_text =
        translator.translate("MainWindow", "&New from scratch...");
    if (!main_window_text.isEmpty() &&
        main_window_text != QStringLiteral("&New from scratch...")) {
        return true;
    }

    const QString signal_list_text =
        translator.translate("SignalListPanel", "+ New");
    return !signal_list_text.isEmpty() &&
           signal_list_text != QStringLiteral("+ New");
}

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
    case Signal::InterpolationMode::Linear: return "Linear";
    case Signal::InterpolationMode::Step:   return "Step";
    }
    return "Linear";
}

double wrap_unit_phase(double x) {
    const double wrapped = std::fmod(x, 1.0);
    return wrapped < 0.0 ? wrapped + 1.0 : wrapped;
}

/**
 * @brief Computes the number of samples implied by a sampling interval.
 * @param t_start Inclusive first timestamp.
 * @param t_end Inclusive upper bound of the generated time range.
 * @param sampling_time Positive sampling interval entered by the user.
 * @return Number of samples that will be generated for the dialog preview.
 */
[[nodiscard]] std::size_t compute_sample_count(double t_start,
                                               double t_end,
                                               double sampling_time) {
    if (!(sampling_time > 0.0)) {
        throw std::invalid_argument("sampling time must be > 0");
    }
    if (!(t_end > t_start)) {
        throw std::invalid_argument("t end must be greater than t start");
    }
    const auto sample_count =
        static_cast<std::size_t>(std::ceil((t_end - t_start) / sampling_time)) + 1U;
    if (sample_count > kMaxGeneratedSamples) {
        throw std::invalid_argument("sampling time generates too many samples");
    }
    return std::max<std::size_t>(2U, sample_count);
}

using GenerationProgress = std::function<bool(std::size_t completed, std::size_t total)>;

template <typename ValueAt>
Signal build_generated_signal(const QString& name,
                              double t_start,
                              double t_end,
                              double sampling_time,
                              Signal::InterpolationMode interpolation,
                              ValueAt value_at,
                              const GenerationProgress& progress = {}) {
    const std::size_t sample_count = compute_sample_count(t_start, t_end, sampling_time);
    std::vector<SamplePoint> samples;
    samples.reserve(sample_count);

    for (std::size_t index = 0; index < sample_count; ++index) {
        const double t = index + 1U == sample_count
            ? t_end
            : std::min(t_start + static_cast<double>(index) * sampling_time, t_end);
        samples.push_back({t, value_at(t, index, sample_count)});

        if (progress && (index % kGenerationProgressStride == 0U || index + 1U == sample_count)) {
            if (!progress(index + 1U, sample_count)) {
                throw std::runtime_error("signal generation cancelled");
            }
        }
    }

    return Signal::from_ordered_samples(name.toStdString(), std::move(samples), interpolation);
}

Signal generate_constant_signal(const QString& name, double t_start, double t_end,
                                double sampling_time, double level,
                                const GenerationProgress& progress = {}) {
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Linear,
                                  [level](double, std::size_t, std::size_t) { return level; },
                                  progress);
}

Signal generate_periodic_signal(const QString& name, double t_start, double t_end,
                                double sampling_time, double amplitude, double offset,
                                double frequency_hz, double phase_deg, bool use_cosine,
                                const GenerationProgress& progress = {}) {
    if (!(frequency_hz > 0.0)) {
        throw std::invalid_argument("frequency must be > 0");
    }
    const double phase_rad = phase_deg * kPi / 180.0;
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Linear,
                                  [=](double t, std::size_t, std::size_t) {
                                      const double angle = 2.0 * kPi * frequency_hz * (t - t_start) + phase_rad;
                                      const double carrier = use_cosine ? std::cos(angle) : std::sin(angle);
                                      return offset + amplitude * carrier;
                                  },
                                  progress);
}

Signal generate_pulse_signal(const QString& name, double t_start, double t_end,
                             double sampling_time, double low_level, double high_level,
                             double frequency_hz, double duty_cycle_pct, double phase_deg,
                             const GenerationProgress& progress = {}) {
    if (!(frequency_hz > 0.0)) { throw std::invalid_argument("frequency must be > 0"); }
    if (!(duty_cycle_pct > 0.0 && duty_cycle_pct < 100.0)) {
        throw std::invalid_argument("duty cycle must be between 0 and 100");
    }
    const double duty         = duty_cycle_pct / 100.0;
    const double phase_cycles = phase_deg / 360.0;
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Step,
                                  [=](double t, std::size_t, std::size_t) {
                                      const double unit_phase =
                                          wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
                                      return unit_phase < duty ? high_level : low_level;
                                  },
                                  progress);
}

Signal generate_sawtooth_signal(const QString& name, double t_start, double t_end,
                                double sampling_time, double min_value, double max_value,
                                double frequency_hz, double phase_deg,
                                const GenerationProgress& progress = {}) {
    if (!(frequency_hz > 0.0)) { throw std::invalid_argument("frequency must be > 0"); }
    const double span         = max_value - min_value;
    const double phase_cycles = phase_deg / 360.0;
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Linear,
                                  [=](double t, std::size_t, std::size_t) {
                                      const double unit_phase =
                                          wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
                                      return min_value + span * unit_phase;
                                  },
                                  progress);
}

Signal generate_triangle_signal(const QString& name, double t_start, double t_end,
                                double sampling_time, double min_value, double max_value,
                                double frequency_hz, double phase_deg,
                                const GenerationProgress& progress = {}) {
    if (!(frequency_hz > 0.0)) { throw std::invalid_argument("frequency must be > 0"); }
    const double phase_cycles = phase_deg / 360.0;
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Linear,
                                  [=](double t, std::size_t, std::size_t) {
                                      const double unit_phase =
                                          wrap_unit_phase(frequency_hz * (t - t_start) + phase_cycles);
                                      const double shape =
                                          unit_phase < 0.5 ? unit_phase * 2.0 : 2.0 - unit_phase * 2.0;
                                      return min_value + (max_value - min_value) * shape;
                                  },
                                  progress);
}

Signal generate_ramp_signal(const QString& name, double t_start, double t_end,
                            double sampling_time, double start_value, double end_value,
                            const GenerationProgress& progress = {}) {
    return build_generated_signal(name, t_start, t_end, sampling_time,
                                  Signal::InterpolationMode::Linear,
                                  [=](double, std::size_t index, std::size_t sample_count) {
                                      const double alpha =
                                          static_cast<double>(index) / static_cast<double>(sample_count - 1U);
                                      return start_value + (end_value - start_value) * alpha;
                                  },
                                  progress);
}

std::vector<Signal::EnumerationEntry> parse_enumeration_definition(const QString& text) {
    std::vector<Signal::EnumerationEntry> entries;
    const QStringList lines = text.split(QStringLiteral("\n"));
    for (const QString& raw_line : lines) {
        const QString line = raw_line.trimmed();
        if (line.isEmpty()) { continue; }
        const int separator = line.lastIndexOf(':');
        if (separator <= 0 || separator >= line.size() - 1) {
            throw std::invalid_argument(
                "Each enumerated state must use the format LABEL:NUMERIC_VALUE");
        }
        bool ok = false;
        const QString label = line.left(separator).trimmed();
        const double value  = parse_flexible_double(line.mid(separator + 1).trimmed(), &ok);
        if (label.isEmpty() || !ok) {
            throw std::invalid_argument(
                "Invalid enumerated state definition: " + line.toStdString());
        }
        entries.push_back(Signal::EnumerationEntry{label.toStdString(), value});
    }
    if (entries.empty()) {
        throw std::invalid_argument("Define at least one enumerated state");
    }
    return entries;
}

QString format_enumeration_definition(const std::vector<Signal::EnumerationEntry>& enumeration) {
    QStringList lines;
    lines.reserve(static_cast<qsizetype>(enumeration.size()));
    for (const auto& entry : enumeration) {
        lines.push_back(QStringLiteral("%1:%2")
                            .arg(QString::fromStdString(entry.label))
                            .arg(QString::number(entry.value, 'g', 15)));
    }
    return lines.join(QStringLiteral("\n"));
}

Signal generate_enumerated_signal(const QString& name, double t_start, double t_end,
                                  double sampling_time,
                                  const std::vector<Signal::EnumerationEntry>& enumeration,
                                  const QString& initial_label,
                                  const GenerationProgress& progress = {}) {
    QString resolved_label = initial_label.trimmed();
    if (resolved_label.isEmpty()) {
        resolved_label = QString::fromStdString(enumeration.front().label);
    }
    const double initial_value = [&]() {
        Signal preview = Signal::from_ordered_samples(
            name.toStdString(),
            std::vector<SamplePoint>{{t_start, 0.0}, {t_end, 0.0}},
            Signal::InterpolationMode::Step);
        preview.set_enumeration(enumeration);
        return preview.value_for_label(resolved_label.toStdString());
    }();
    Signal signal = build_generated_signal(name, t_start, t_end, sampling_time,
                                           Signal::InterpolationMode::Step,
                                           [initial_value](double, std::size_t, std::size_t) {
                                               return initial_value;
                                           },
                                           progress);
    signal.set_enumeration(enumeration);
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

Signal generate_waveform_signal(WaveformKind kind, const QString& name,
                                double t_start, double t_end, double sampling_time,
                                const std::array<double, 4>& params_a,
                                const std::array<double, 4>& params_b,
                                const GenerationProgress& progress = {}) {
    switch (kind) {
    case WaveformKind::Constant:
        return generate_constant_signal(name, t_start, t_end, sampling_time, params_a[0], progress);
    case WaveformKind::Sine:
        return generate_periodic_signal(name, t_start, t_end, sampling_time,
                                        params_a[0], params_a[1], params_a[2], params_a[3], false,
                                        progress);
    case WaveformKind::Cosine:
        return generate_periodic_signal(name, t_start, t_end, sampling_time,
                                        params_a[0], params_a[1], params_a[2], params_a[3], true,
                                        progress);
    case WaveformKind::Pulse:
        return generate_pulse_signal(name, t_start, t_end, sampling_time,
                                     params_a[0], params_a[1], params_a[2], params_a[3], params_b[0],
                                     progress);
    case WaveformKind::Sawtooth:
        return generate_sawtooth_signal(name, t_start, t_end, sampling_time,
                                        params_a[0], params_a[1], params_a[2], params_a[3],
                                        progress);
    case WaveformKind::Triangle:
        return generate_triangle_signal(name, t_start, t_end, sampling_time,
                                        params_a[0], params_a[1], params_a[2], params_a[3],
                                        progress);
    case WaveformKind::Ramp:
        return generate_ramp_signal(name, t_start, t_end, sampling_time,
                                    params_a[0], params_a[1], progress);
    case WaveformKind::Enumerated:
        break;
    }
    throw std::invalid_argument("unsupported waveform");
}
}  // namespace

// ============================================================================
// Construction
// ============================================================================

MainWindow::MainWindow(SignalEditorService& service, QWidget* parent)
    : QMainWindow(parent), service_(service) {
    setWindowTitle(tr("Signal Editor"));
    apply_application_icon(this);
    resize(1400, 820);
    setAcceptDrops(true);
    app_settings_.theme = QStringLiteral("light");
    app_settings_.language = QStringLiteral("it");
    visual_settings_ = app_settings_;

#ifdef LIB_QT_UTILS_AVAILABLE
    app_state_ = std::make_unique<QtUtils::AppState>(
        ui::app_id(),
        ui::settings_version_scope());

    if (app_state_) {
        app_settings_.language = app_state_->value(
            QString::fromUtf8(ui::kSettingsKeyLanguage),
            app_settings_.language).toString();
    }
#endif
    visual_settings_ = app_settings_;
    apply_language(app_settings_.language);

    // ── Central layout ────────────────────────────────────────────────────────
    auto* central        = new QWidget(this);
    auto* central_layout = new QVBoxLayout(central);
    central_layout->setContentsMargins(16, 14, 16, 14);
    central_layout->setSpacing(10);

    // ── Side panel ────────────────────────────────────────────────────────────
    auto* outer_splitter = new QSplitter(Qt::Horizontal, central);
    auto* side_panel     = new QWidget(outer_splitter);
    auto* side_layout    = new QVBoxLayout(side_panel);
    side_layout->setContentsMargins(0, 0, 0, 0);
    side_layout->setSpacing(0);

    auto* side_splitter = new QSplitter(Qt::Vertical, side_panel);
    side_splitter->setChildrenCollapsible(false);
    side_splitter->setHandleWidth(10);

    file_panel_ = new FileListPanel(side_splitter);
    list_panel_ = new SignalListPanel(side_splitter);
    side_splitter->addWidget(file_panel_);
    side_splitter->addWidget(list_panel_);
    side_splitter->setStretchFactor(0, 1);
    side_splitter->setStretchFactor(1, 2);
    side_splitter->setSizes({320, 520});

    side_layout->addWidget(side_splitter, 1);

    // ── Center panel ──────────────────────────────────────────────────────────
    auto* center_panel  = new QWidget(outer_splitter);
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(10);

    auto* center_toolbar = new QHBoxLayout();
    center_toolbar->setContentsMargins(0, 0, 0, 0);
    center_toolbar->setSpacing(10);
    interp_label_ = new QLabel(tr("Interpolation"), center_panel);
    interp_label_->setObjectName(QStringLiteral("PanelDetail"));
    interpolation_box_ = new QComboBox(center_panel);
    interpolation_box_->addItem(tr("Linear"));
    interpolation_box_->addItem(tr("Step"));
    center_toolbar->addWidget(interp_label_);
    center_toolbar->addWidget(interpolation_box_, 0);
    center_toolbar->addStretch(1);
    center_layout->addLayout(center_toolbar);

    workspace_tabs_ = new QTabWidget(center_panel);
    workspace_tabs_->setObjectName(QStringLiteral("WorkspaceTabs"));
    workspace_tabs_->setDocumentMode(true);
    workspace_tabs_->setTabPosition(QTabWidget::North);
    workspace_tabs_->tabBar()->setExpanding(true);
    workspace_tabs_->setUsesScrollButtons(false);

    auto* plot_page  = new QWidget(workspace_tabs_);
    plot_page->setObjectName(QStringLiteral("WorkspaceTabPage"));
    plot_page->setAttribute(Qt::WA_StyledBackground, true);
    plot_page->setAutoFillBackground(true);
    auto* plot_layout = new QVBoxLayout(plot_page);
    plot_layout->setContentsMargins(12, 12, 12, 12);
    plot_layout->setSpacing(10);

    auto* plot_content = new QWidget(plot_page);
    auto* plot_content_layout = new QVBoxLayout(plot_content);
    plot_content_layout->setContentsMargins(0, 0, 0, 0);
    plot_content_layout->setSpacing(10);

    auto* plot_tools = new QWidget(plot_content);
    plot_tools->setObjectName(QStringLiteral("PlotTools"));
    plot_tools->setAttribute(Qt::WA_StyledBackground, true);
    plot_tools->setAutoFillBackground(false);
    plot_tools->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* plot_toolbar = new QHBoxLayout(plot_tools);
    plot_toolbar->setContentsMargins(0, 0, 0, 0);
    plot_toolbar->setSpacing(10);
    plot_zoom_in_button_ = new QPushButton(plot_tools);
    plot_zoom_out_button_ = new QPushButton(plot_tools);
    plot_pan_mode_button_ = new QPushButton(plot_tools);
    plot_rect_mode_button_ = new QPushButton(plot_tools);
    plot_reset_view_button_ = new QPushButton(plot_tools);
    plot_export_button_ = new QPushButton(plot_tools);
    plot_t_start_edit_ = new QLineEdit(plot_tools);
    plot_t_end_edit_ = new QLineEdit(plot_tools);
    plot_zoom_in_button_->setObjectName(QStringLiteral("SubtleButton"));
    plot_zoom_out_button_->setObjectName(QStringLiteral("SubtleButton"));
    plot_pan_mode_button_->setObjectName(QStringLiteral("SubtleButton"));
    plot_rect_mode_button_->setObjectName(QStringLiteral("SubtleButton"));
    plot_reset_view_button_->setObjectName(QStringLiteral("AccentButton"));
    plot_export_button_->setObjectName(QStringLiteral("SubtleButton"));
    plot_zoom_in_button_->setCursor(Qt::PointingHandCursor);
    plot_zoom_out_button_->setCursor(Qt::PointingHandCursor);
    plot_pan_mode_button_->setCursor(Qt::PointingHandCursor);
    plot_rect_mode_button_->setCursor(Qt::PointingHandCursor);
    plot_reset_view_button_->setCursor(Qt::PointingHandCursor);
    plot_export_button_->setCursor(Qt::PointingHandCursor);
    plot_zoom_in_button_->setToolTip(tr("Zoom in"));
    plot_zoom_out_button_->setToolTip(tr("Zoom out"));
    plot_pan_mode_button_->setToolTip(tr("Pan mode"));
    plot_rect_mode_button_->setToolTip(tr("Rect mode"));
    plot_reset_view_button_->setToolTip(tr("Fit view"));
    plot_export_button_->setToolTip(tr("Export image"));
    plot_pan_mode_button_->setCheckable(true);
    plot_rect_mode_button_->setCheckable(true);
    plot_t_start_edit_->setValidator(
        new QRegularExpressionValidator(flexible_decimal_pattern(false), plot_t_start_edit_));
    plot_t_end_edit_->setValidator(
        new QRegularExpressionValidator(flexible_decimal_pattern(false), plot_t_end_edit_));
    plot_t_start_edit_->setPlaceholderText(tr("t start"));
    plot_t_end_edit_->setPlaceholderText(tr("t end"));
    plot_t_start_edit_->setAlignment(Qt::AlignRight);
    plot_t_end_edit_->setAlignment(Qt::AlignRight);
    plot_t_start_edit_->setObjectName(QStringLiteral("PlotRangeEdit"));
    plot_t_end_edit_->setObjectName(QStringLiteral("PlotRangeEdit"));
    plot_toolbar->addWidget(plot_zoom_in_button_);
    plot_toolbar->addWidget(plot_zoom_out_button_);
    plot_toolbar->addWidget(plot_pan_mode_button_);
    plot_toolbar->addWidget(plot_rect_mode_button_);
    plot_toolbar->addWidget(plot_reset_view_button_);
    plot_toolbar->addWidget(plot_export_button_);
    plot_toolbar->addSpacing(10);
    plot_t_start_label_ = new QLabel(tr("Visible t start"), plot_tools);
    plot_t_end_label_ = new QLabel(tr("Visible t end"), plot_tools);
    plot_t_start_label_->setObjectName(QStringLiteral("PanelDetail"));
    plot_t_end_label_->setObjectName(QStringLiteral("PanelDetail"));
    plot_toolbar->addWidget(plot_t_start_label_);
    plot_toolbar->addWidget(plot_t_start_edit_);
    plot_toolbar->addWidget(plot_t_end_label_);
    plot_toolbar->addWidget(plot_t_end_edit_);
    plot_toolbar->addStretch(1);
    plot_ = new SignalPlotWidget(plot_content);
    plot_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    plot_content_layout->addWidget(plot_tools, 0);
    plot_content_layout->addWidget(plot_, 1);
    plot_layout->addWidget(plot_content, 1);

    auto* table_page   = new QWidget(workspace_tabs_);
    table_page->setObjectName(QStringLiteral("WorkspaceTabPage"));
    table_page->setAttribute(Qt::WA_StyledBackground, true);
    table_page->setAutoFillBackground(true);
    auto* table_layout = new QVBoxLayout(table_page);
    table_layout->setContentsMargins(12, 12, 12, 12);
    table_layout->setSpacing(10);
    table_panel_ = new SignalTablePanel(table_page);
    table_layout->addWidget(table_panel_);

    workspace_tabs_->addTab(plot_page,  tr("Plot"));
    workspace_tabs_->addTab(table_page, tr("Table"));
    center_layout->addWidget(workspace_tabs_, 1);
    animate_tab_reveal(workspace_tabs_, 0);

    outer_splitter->addWidget(side_panel);
    outer_splitter->addWidget(center_panel);
    outer_splitter->setChildrenCollapsible(false);
    outer_splitter->setHandleWidth(10);
    outer_splitter->setStretchFactor(0, 0);
    outer_splitter->setStretchFactor(1, 1);
    outer_splitter->setSizes({300, 1120});
    central_layout->addWidget(outer_splitter, 1);
    setCentralWidget(central);

    list_panel_->set_library(nullptr);
    list_panel_->set_visible_signal_indices({});
    list_panel_->set_active_signal_index(-1);
    table_panel_->set_library(nullptr, -1, {});
    update_interpolation_box();

    // ── Menu bar ──────────────────────────────────────────────────────────────
    menu_file_   = menuBar()->addMenu(tr("&File"));
    act_open_    = menu_file_->addAction(tr("&Open signal files..."));
    act_save_    = menu_file_->addAction(tr("&Save current file..."));
    act_export_plot_ = menu_file_->addAction(tr("Export &plot image..."));
    undo_action_ = menu_file_->addAction(tr("&Undo"));
    menu_file_->addSeparator();
    act_quit_    = menu_file_->addAction(tr("&Quit"));
    act_open_->setShortcut(QKeySequence::Open);
    act_save_->setShortcut(QKeySequence::Save);
    act_export_plot_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    undo_action_->setShortcut(QKeySequence::Undo);
    act_quit_->setShortcut(QKeySequence::Quit);

    menu_signal_   = menuBar()->addMenu(tr("&Signal"));
    act_new_       = menu_signal_->addAction(tr("&New from scratch..."));
    rename_action_ = menu_signal_->addAction(tr("Re&name selected"));
    act_remove_    = menu_signal_->addAction(tr("&Remove selected"));
    act_new_->setShortcut(QKeySequence::New);
    rename_action_->setShortcut(Qt::Key_F2);
    act_remove_->setShortcut(QKeySequence::Delete);

    menu_settings_   = menuBar()->addMenu(tr("&Settings"));
    settings_action_ = menu_settings_->addAction(tr("&Preferences..."));
    settings_action_->setShortcut(QKeySequence::Preferences);

    menu_help_ = menuBar()->addMenu(tr("&Help"));
    act_manual_ = menu_help_->addAction(tr("&User manual..."));
    act_about_ = menu_help_->addAction(tr("&About"));

    // ── Toolbar ───────────────────────────────────────────────────────────────
    main_toolbar_ = addToolBar(tr("Main"));
    main_toolbar_->setMovable(false);
    main_toolbar_->setIconSize(QSize(20, 20));
    main_toolbar_->addAction(act_open_);
    main_toolbar_->addAction(act_save_);
    main_toolbar_->addAction(act_export_plot_);
    main_toolbar_->addAction(undo_action_);
    main_toolbar_->addSeparator();
    main_toolbar_->addAction(act_new_);
    main_toolbar_->addAction(rename_action_);
    main_toolbar_->addAction(act_remove_);

    // ── Status bar — settings button (permanent area, never covered) ─────────
    settings_btn_ = new QToolButton(this);
    settings_btn_->setToolTip(tr("Open application settings"));
    settings_btn_->setStatusTip(tr("Open preferences and visual settings."));
    settings_btn_->setAutoRaise(true);
    settings_btn_->setCursor(Qt::PointingHandCursor);
    statusBar()->addPermanentWidget(settings_btn_, 0);

    register_svg_icon(act_open_, QStringLiteral(":/img/open.svg"));
    register_svg_icon(act_save_, QStringLiteral(":/img/save.svg"));
    register_svg_icon(act_export_plot_, QStringLiteral(":/img/screen-share.svg"));
    register_svg_icon(undo_action_, QStringLiteral(":/img/undo.svg"));
    register_svg_icon(act_new_, QStringLiteral(":/img/new.svg"));
    register_svg_icon(rename_action_, QStringLiteral(":/img/rename.svg"));
    register_svg_icon(act_remove_, QStringLiteral(":/img/remove.svg"));
    register_svg_icon(plot_zoom_in_button_, QStringLiteral(":/img/zoom-in.svg"));
    register_svg_icon(plot_zoom_out_button_, QStringLiteral(":/img/zoom-out.svg"));
    register_svg_icon(plot_pan_mode_button_, QStringLiteral(":/img/pan.svg"));
    register_svg_icon(plot_rect_mode_button_, QStringLiteral(":/img/zoom-selection.svg"));
    register_svg_icon(plot_reset_view_button_, QStringLiteral(":/img/zoom-fit.svg"));
    register_svg_icon(plot_export_button_, QStringLiteral(":/img/screen-share.svg"));
    register_svg_icon(settings_btn_, QStringLiteral(":/img/settings.svg"));
    refresh_registered_svg_icons();

    // ── Connections ───────────────────────────────────────────────────────────
    connect(act_open_,     &QAction::triggered, this, &MainWindow::onOpen);
    connect(act_save_,     &QAction::triggered, this, &MainWindow::onSave);
    connect(act_export_plot_, &QAction::triggered, this, &MainWindow::onExportPlotImage);
    connect(undo_action_,  &QAction::triggered, this, &MainWindow::onUndo);
    connect(act_new_,      &QAction::triggered, this, &MainWindow::onNewSignal);
    connect(rename_action_,&QAction::triggered, this, &MainWindow::onRenameSignal);
    connect(act_remove_,   &QAction::triggered, this, &MainWindow::onRemoveSignal);
    connect(act_manual_,   &QAction::triggered, this, &MainWindow::onOpenManualPlaceholder);
    connect(act_about_,    &QAction::triggered, this, &MainWindow::onAbout);
    connect(act_quit_,     &QAction::triggered, qApp, &QApplication::quit);
    connect(settings_action_, &QAction::triggered, this, &MainWindow::onOpenSettings);
    connect(settings_btn_, &QToolButton::clicked, this, &MainWindow::onOpenSettings);

    connect(file_panel_, &FileListPanel::selectionChanged,
            this, &MainWindow::onFileSelectionChanged);
    connect(file_panel_, &FileListPanel::openRequested,
            this, &MainWindow::onFileOpenRequested);
    connect(file_panel_, &FileListPanel::renameRequested,
            this, &MainWindow::onFileRenameRequested);
    connect(file_panel_, &FileListPanel::removeRequested,
            this, &MainWindow::onFileRemoveRequested);
    connect(file_panel_, &FileListPanel::reloadRequested,
            this, &MainWindow::onFileReloadRequested);
    connect(file_panel_, &FileListPanel::detailsRequested,
            this, &MainWindow::onFileDetailsRequested);
    connect(list_panel_, &SignalListPanel::selectionChanged,
            this, &MainWindow::onSignalSelectionChanged);
    connect(list_panel_, &SignalListPanel::visibilityChanged,
            this, &MainWindow::onSignalVisibilityChanged);
    connect(list_panel_, &SignalListPanel::optionsRequested,
            this, &MainWindow::onSignalOptionsRequested);
    connect(list_panel_, &SignalListPanel::renameRequested,
            this, &MainWindow::onRenameRequested);
    connect(list_panel_, &SignalListPanel::addRequested,
            this, &MainWindow::onAddRequested);
    connect(list_panel_, &SignalListPanel::removeRequested,
            this, &MainWindow::onRemoveRequested);
    connect(plot_, &SignalPlotWidget::editStarted,
            this, &MainWindow::onPlotEditStarted);
    connect(plot_, &SignalPlotWidget::sampleMoveRequested,
            this, &MainWindow::onPlotSampleMoveRequested);
    connect(plot_, &SignalPlotWidget::sampleInsertRequested,
            this, &MainWindow::onPlotSampleInsertRequested);
    connect(plot_, &SignalPlotWidget::sampleRemoveRequested,
            this, &MainWindow::onPlotSampleRemoveRequested);
    connect(plot_, &SignalPlotWidget::segmentMoveRequested,
            this, &MainWindow::onPlotSegmentMoveRequested);
    connect(plot_, &SignalPlotWidget::gaussianBrushRequested,
            this, &MainWindow::onPlotGaussianBrushRequested);
    connect(plot_, &SignalPlotWidget::offsetAllRequested,
            this, &MainWindow::onOffsetAllRequested);
    connect(plot_, &SignalPlotWidget::sampleOffsetRequested,
            this, &MainWindow::onPlotSampleOffsetRequested);
    connect(plot_, &SignalPlotWidget::signalChanged,
            this, &MainWindow::onPlotChanged);
    connect(plot_, &SignalPlotWidget::cursorMoved,
            this, &MainWindow::onCursorMoved);
    connect(plot_, &SignalPlotWidget::timeViewChanged,
            this, &MainWindow::onPlotTimeViewChanged);
    connect(table_panel_, &SignalTablePanel::editStarted,
            this, &MainWindow::onTableEditStarted);
    connect(table_panel_, &SignalTablePanel::sampleEdited,
            this, &MainWindow::onTableSampleEdited);
    connect(table_panel_, &SignalTablePanel::sampleInserted,
            this, &MainWindow::onTableSampleInserted);
    connect(table_panel_, &SignalTablePanel::sampleRemoved,
            this, &MainWindow::onTableSampleRemoved);
    connect(table_panel_, &SignalTablePanel::offsetAllRequested,
            this, &MainWindow::onOffsetAllRequested);
    connect(table_panel_, &SignalTablePanel::sampleOffsetRequested,
            this, &MainWindow::onTableSampleOffsetRequested);
    connect(table_panel_, &SignalTablePanel::contentChanged,
            this, &MainWindow::onTableChanged);
    connect(interpolation_box_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSignalInterpolationChanged);
    connect(plot_zoom_in_button_, &QPushButton::clicked,
            this, &MainWindow::onPlotZoomInRequested);
    connect(plot_zoom_out_button_, &QPushButton::clicked,
            this, &MainWindow::onPlotZoomOutRequested);
    connect(plot_pan_mode_button_, &QPushButton::toggled,
            this, &MainWindow::onPlotPanModeToggled);
    connect(plot_rect_mode_button_, &QPushButton::toggled,
            this, &MainWindow::onPlotRectModeToggled);
    connect(plot_reset_view_button_, &QPushButton::clicked,
            plot_, &SignalPlotWidget::reset_time_view);
    connect(plot_export_button_, &QPushButton::clicked,
            this, &MainWindow::onExportPlotImage);
    connect(plot_t_start_edit_, &QLineEdit::editingFinished,
            this, &MainWindow::onPlotRangeEditingFinished);
    connect(plot_t_end_edit_, &QLineEdit::editingFinished,
            this, &MainWindow::onPlotRangeEditingFinished);
    connect(workspace_tabs_, &QTabWidget::currentChanged,
            this, [this](int index) { animate_tab_reveal(workspace_tabs_, index); });

    load_persisted_settings();
    sync_plot_view_controls();
    update_undo_action();
    refresh_status();
}

// ============================================================================
// Qt event overrides
// ============================================================================

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (!event->mimeData()->hasUrls()) { return; }
    for (const QUrl& url : event->mimeData()->urls()) {
        if (is_supported_import_path(url.toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (is_supported_import_path(path)) {
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

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslate_ui();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
#ifdef LIB_QT_UTILS_AVAILABLE
    if (app_state_) {
        app_state_->saveWindowGeometry(QStringLiteral("main_window"), saveGeometry());
        app_state_->saveWindowState(QStringLiteral("main_window"), saveState());
        app_state_->sync();
    }
#endif
    QMainWindow::closeEvent(event);
}

// ============================================================================
// File slots
// ============================================================================

void MainWindow::onOpen() {
#ifdef LIB_QT_UTILS_AVAILABLE
    const QString initial_dir = app_state_ ? app_state_->lastOpenedFolder() : QString{};
#else
    const QString initial_dir;
#endif
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Open signal files"), initial_dir, import_file_filter());
    if (!paths.isEmpty()) {
#ifdef LIB_QT_UTILS_AVAILABLE
        if (app_state_) {
            app_state_->setLastOpenedFolder(QFileInfo(paths.front()).absolutePath());
            app_state_->sync();
        }
#endif
    }
    open_paths(paths);
}

void MainWindow::onSave() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        show_error(tr("Nothing to save"), tr("Load a supported signal file first."));
        return;
    }
    sync_active_document_from_service();
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    const QString initial_path = document.path.isEmpty()
        ? QStringLiteral("%1.csv").arg(
              document.display_name.trimmed().isEmpty()
                  ? QStringLiteral("signals")
                  : document.display_name.trimmed())
        : document.path;

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save current signal file"), initial_path, export_file_filter());
    if (path.isEmpty()) { return; }

#ifdef LIB_QT_UTILS_AVAILABLE
    if (app_state_) {
        app_state_->setLastOpenedFolder(QFileInfo(path).absolutePath());
        app_state_->sync();
    }
#endif

    const auto result = save_document_state(document, path);
    if (!result.is_ok()) {
        show_error(tr("Export failed"), QString::fromStdString(result.message));
        return;
    }

    document.path         = path;
    document.display_name = QFileInfo(path).fileName();
    document.format       = document_format_for_path(path);
    document.dirty        = false;
    clear_undo_history();
    refresh_file_panel();
    refresh_status(tr("Saved %1").arg(path));
}

void MainWindow::onExportPlotImage() {
    if (plot_ == nullptr) {
        show_error(tr("Export failed"), tr("Plot widget is not available."));
        return;
    }

    QString suggested_name = QStringLiteral("plot_snapshot.png");
    if (active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size())) {
        const QString base_name =
            QFileInfo(documents_[static_cast<std::size_t>(active_document_index_)].display_name).completeBaseName();
        if (!base_name.trimmed().isEmpty()) {
            suggested_name = QStringLiteral("%1_plot.png").arg(base_name.trimmed());
        }
    }

    const QString selected_path = QFileDialog::getSaveFileName(
        this,
        tr("Export plot image"),
        suggested_name,
        tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg);;BMP image (*.bmp)"));
    if (selected_path.isEmpty()) {
        return;
    }

    QString path = selected_path;
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".png");
    }

    const QPixmap snapshot = plot_->grab();
    if (snapshot.isNull()) {
        show_error(tr("Export failed"), tr("Unable to capture the current plot image."));
        return;
    }
    if (!snapshot.save(path)) {
        show_error(tr("Export failed"),
                   tr("Unable to save the plot image to:\n%1").arg(path));
        return;
    }

    refresh_status(tr("Plot image exported to %1").arg(path));
}

void MainWindow::onUndo() {
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (document.undo_stack.empty()) { return; }

    const UndoState state = std::move(document.undo_stack.back());
    document.undo_stack.pop_back();
    document.sheets.clear();
    document.sheets.reserve(state.sheets.size());
    for (const auto& sheet : state.sheets) {
        document.sheets.push_back(LoadedSheetState{
            sheet.name,
            sheet.library,
            sheet.visible_signal_indices,
            sheet.active_signal_index,
        });
    }
    document.active_sheet_index = std::clamp(
        state.active_sheet_index,
        0,
        document.sheets.empty() ? 0 : static_cast<int>(document.sheets.size()) - 1);
    service_.set_library(document.sheets.empty()
                             ? SignalLibrary{}
                             : document.sheets[static_cast<std::size_t>(document.active_sheet_index)].library);
    document.dirty = !document.undo_stack.empty();
    activate_document(active_document_index_, active_signal_index());
    update_undo_action();
    refresh_file_panel();
    refresh_status(tr("Undo applied"));
}

// ============================================================================
// Signal slots
// ============================================================================

void MainWindow::onNewSignal()    { onAddRequested(); }
void MainWindow::onRemoveSignal() { onRemoveRequested(active_signal_index()); }
void MainWindow::onRenameSignal() {
    if (active_document_index_ < 0 || service_.library().empty()) { return; }
    list_panel_->begin_rename_current();
}

void MainWindow::onAbout() {
    QMessageBox box(this);
    apply_application_icon(&box);
    box.setWindowTitle(tr("About Signal Editor"));
    box.setIconPixmap(application_logo_pixmap(QSize(96, 96)));
    box.setTextFormat(Qt::RichText);
    box.setText(
        tr("<h3>Signal Editor</h3>"
           "<p>Desktop waveform editor for engineering workflows.</p>"
           "<p><b>Version:</b> %1</p>"
           "<p>Signal Editor provides multi-file and multi-sheet signal editing with "
           "synchronized plot and table workflows, numeric and enumerated signal support, "
           "workbook-aware XLSX handling, undo, reload-from-disk, and plot image export.</p>")
            .arg(ui::application_version()));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void MainWindow::onOpenManualPlaceholder() {
    QMessageBox box(this);
    apply_application_icon(&box);
    box.setWindowTitle(tr("User manual"));
    box.setIconPixmap(application_logo_pixmap(QSize(72, 72)));
    box.setTextFormat(Qt::RichText);
    box.setText(
        tr("<h3>User manual placeholder</h3>"
           "<p>The integrated user manual entry point is reserved here.</p>"
           "<p>You can later connect this action to the PDF generated from the "
           "Markdown manual under <code>docs/user/user_manual.md</code>.</p>"));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

// ============================================================================
// Settings slot
// ============================================================================

void MainWindow::onOpenSettings() {
#ifdef LIB_QT_CUSTOM_WIDGETS_AVAILABLE
    lib_qt_custom_widgets::SettingsPanelDialog dlg(this);
    apply_application_icon(&dlg);
    dlg.setCurrentSettings(app_settings_);
    auto preview_settings = visual_settings_;

    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::themeChanged,
            this, [this, &preview_settings](const QString& theme) {
        preview_settings.theme = theme;
        apply_visual_settings(preview_settings, false);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::primaryColorChanged,
            this, [this, &preview_settings](const QColor& color) {
        preview_settings.primaryColor = color;
        apply_visual_settings(preview_settings, false);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::highContrastModeChanged,
            this, [this, &preview_settings](bool enabled) {
        preview_settings.highContrastMode = enabled;
        apply_visual_settings(preview_settings, false);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::fontFamilyChanged,
            this, [this, &preview_settings](const QString& family) {
        preview_settings.fontFamily = family;
        apply_visual_settings(preview_settings, false);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::fontSizeChanged,
            this, [this, &preview_settings](int size) {
        preview_settings.fontSize = size;
        apply_visual_settings(preview_settings, false);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::widgetDensityChanged,
            this, [this, &preview_settings](const QString& density) {
        preview_settings.widgetDensity = density;
        apply_density(density);
        visual_settings_.widgetDensity = density;
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::languageChanged,
            this, [this, &preview_settings](const QString& language) {
        preview_settings.language = language;
        apply_language(language);
        visual_settings_.language = language;
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::animationDurationChanged,
            this, [this, &preview_settings](int ms) {
        preview_settings.animationDurationMs = ms;
        apply_behavior_settings(preview_settings);
    });
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::settingsApplied,
            this, &MainWindow::onSettingsApplied);
    connect(&dlg, &lib_qt_custom_widgets::SettingsPanelDialog::settingsSaved,
            this, &MainWindow::onSettingsSaved);
    const int result = dlg.exec();
    if (result != QDialog::Accepted && visual_settings_ != app_settings_) {
        apply_settings(app_settings_, false);
        refresh_status(tr("Settings preview discarded"));
    }
#else
    QMessageBox::information(this,
        tr("Settings"),
        tr("Settings panel is not available in this build.\n"
           "Build with lib-qt-custom-widgets to enable it."));
#endif
}

void MainWindow::onSettingsApplied(const lib_qt_custom_widgets::AppSettings& settings) {
    apply_settings(settings, false);
    refresh_status(tr("Settings applied as preview"));
}

void MainWindow::onSettingsSaved(const lib_qt_custom_widgets::AppSettings& settings) {
    apply_settings(settings, true);
    refresh_status(tr("Settings saved"));
}

void MainWindow::apply_settings(const lib_qt_custom_widgets::AppSettings& settings, bool persist) {
    const lib_qt_custom_widgets::AppSettings baseline = persist ? app_settings_ : visual_settings_;
    if (settings == baseline) {
        return;
    }

    apply_visual_settings(settings, true);
    apply_behavior_settings(settings);
    apply_language(settings.language);

    if (persist) {
        app_settings_ = settings;
        persist_settings();
        remove_default_settings_panel_cache();
    }
}

// ============================================================================
// File panel slots
// ============================================================================

void MainWindow::onFileSelectionChanged(int index) {
    if (switching_document_) { return; }
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        refresh_status();
        return;
    }
    refresh_status(tr("Selected %1").arg(
        documents_[static_cast<std::size_t>(index)].display_name));
}

void MainWindow::onFileOpenRequested(int index) {
    if (switching_document_) {
        return;
    }
    activate_document(index);
}

void MainWindow::onSignalSelectionChanged(int index) {
    if (switching_document_) {
        return;
    }
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        rebind_plot();
        return;
    }

    auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        rebind_plot();
        return;
    }
    if (index >= 0) {
        sheet->active_signal_index = index;
        auto& plotted = sheet->visible_signal_indices;
        if (std::find(plotted.begin(), plotted.end(), index) == plotted.end()) {
            plotted.push_back(index);
            normalize_visible_signal_indices(*sheet);
            switching_document_ = true;
            list_panel_->set_visible_signal_indices(plotted);
            list_panel_->set_active_signal_index(index);
            list_panel_->select(index);
            switching_document_ = false;
        }
    }

    const bool preserve_time_view = plot_ != nullptr;
    const std::pair<double, double> previous_time_view =
        preserve_time_view ? plot_->time_view() : std::pair<double, double>{0.0, 0.0};
    rebind_plot();
    if (preserve_time_view && index >= 0 && plot_ != nullptr) {
        (void)plot_->set_time_view(previous_time_view.first, previous_time_view.second);
        sync_plot_view_controls();
    }
}

void MainWindow::onSignalVisibilityChanged(int index, bool visible) {
    if (switching_document_ ||
        active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size()) ||
        index < 0) {
        return;
    }
    const bool preserve_time_view = plot_ != nullptr;
    const std::pair<double, double> previous_time_view =
        preserve_time_view ? plot_->time_view() : std::pair<double, double>{0.0, 0.0};

    auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        rebind_plot();
        return;
    }
    auto& plotted = sheet->visible_signal_indices;
    const auto existing = std::find(plotted.begin(), plotted.end(), index);
    if (visible) {
        if (existing == plotted.end()) {
            plotted.push_back(index);
        }
    } else if (existing != plotted.end()) {
        plotted.erase(existing);
    }
    normalize_visible_signal_indices(*sheet);

    if (list_panel_ == nullptr) {
        rebind_plot();
        if (preserve_time_view && active_signal_index() >= 0 && plot_ != nullptr) {
            (void)plot_->set_time_view(previous_time_view.first, previous_time_view.second);
            sync_plot_view_controls();
        }
        return;
    }

    const int current_index = active_signal_index();
    int next_active_index = current_index;
    if (!visible) {
        if (current_index == index) {
            next_active_index = plotted.empty() ? -1 : plotted.front();
        }
    } else if (current_index < 0 ||
               std::find(plotted.begin(), plotted.end(), current_index) == plotted.end()) {
        next_active_index = index;
    }
    sheet->active_signal_index = next_active_index;

    switching_document_ = true;
    list_panel_->set_visible_signal_indices(plotted);
    list_panel_->set_active_signal_index(next_active_index);
    list_panel_->select(next_active_index);
    switching_document_ = false;

    rebind_plot();
    if (preserve_time_view && active_signal_index() >= 0 && plot_ != nullptr) {
        (void)plot_->set_time_view(previous_time_view.first, previous_time_view.second);
        sync_plot_view_controls();
    }
}

void MainWindow::onSignalOptionsRequested(int index) { show_signal_options(index); }

void MainWindow::onFileRemoveRequested(const QList<int>& indices) {
    QList<int> rows = indices;
    rows.erase(std::remove_if(rows.begin(), rows.end(), [&](int index) {
        return index < 0 || index >= static_cast<int>(documents_.size());
    }), rows.end());
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    if (rows.isEmpty()) {
        return;
    }

    QString confirmation_text;
    if (rows.size() == 1) {
        const QString file_name = documents_[static_cast<std::size_t>(rows.front())].display_name;
        confirmation_text =
            tr("Remove %1 from the current workspace?\nThe file on disk will not be deleted.")
                .arg(file_name);
    } else {
        confirmation_text =
            tr("Remove %1 selected files from the current workspace?\nThe files on disk will not be deleted.")
                .arg(rows.size());
    }

    const auto answer = QMessageBox::question(
        this,
        tr("Remove file from workspace"),
        confirmation_text);
    if (answer != QMessageBox::Yes) {
        return;
    }

    const int previous_active_index = active_document_index_;
    const int previous_signal_index = active_signal_index();
    if (active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size())) {
        sync_active_document_from_service();
    }

    const int first_removed_index = rows.front();
    const bool removing_active = previous_active_index >= 0 && rows.contains(previous_active_index);
    int removed_before_active = 0;
    for (int row : rows) {
        if (previous_active_index >= 0 && row < previous_active_index) {
            ++removed_before_active;
        }
    }

    for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
        documents_.erase(documents_.begin() + *it);
    }

    if (documents_.empty()) {
        active_document_index_ = -1;
        service_.clear();
        list_panel_->set_library(nullptr);
        list_panel_->set_visible_signal_indices({});
        list_panel_->set_active_signal_index(-1);
        plot_->set_library(nullptr, -1, {});
        table_panel_->set_library(nullptr, -1, {});
        refresh_file_panel();
        update_undo_action();
        refresh_status(rows.size() == 1
                           ? tr("Removed 1 file from workspace")
                           : tr("Removed %1 files from workspace").arg(rows.size()));
        return;
    }

    int next_index = removing_active
        ? std::min(first_removed_index, static_cast<int>(documents_.size()) - 1)
        : previous_active_index - removed_before_active;
    next_index = std::clamp(next_index, 0, static_cast<int>(documents_.size()) - 1);
    refresh_file_panel();
    activate_document(next_index, removing_active ? 0 : previous_signal_index);
    refresh_status(rows.size() == 1
                       ? tr("Removed 1 file from workspace")
                       : tr("Removed %1 files from workspace").arg(rows.size()));
}

void MainWindow::onFileReloadRequested(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }

    auto& document = documents_[static_cast<std::size_t>(index)];
    if (document.path.trimmed().isEmpty()) {
        show_error(tr("Reload failed"),
                   tr("This workspace item has no source file on disk yet."));
        return;
    }

    if (document.dirty) {
        const auto answer = QMessageBox::question(
            this,
            tr("Reload file from disk"),
            tr("Reload %1 from disk and discard the workspace changes for this file?")
                .arg(document.display_name));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    const bool had_active_document =
        active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size());
    const int previous_active_document_index = active_document_index_;
    const int previous_signal_index = active_signal_index();
    const int previous_sheet_index = active_sheet_index();
    if (had_active_document) {
        sync_active_document_from_service();
    }
    const auto previous_documents = documents_;

    try {
        switching_document_ = true;
        list_panel_->set_library(nullptr);
        list_panel_->set_visible_signal_indices({});
        list_panel_->set_active_signal_index(-1);
        plot_->set_library(nullptr, -1, {});
        table_panel_->set_library(nullptr, -1, {});
        switching_document_ = false;

        LoadedDocument reloaded = load_document_state(document.path);
        reloaded.display_name = document.display_name;
        reloaded.undo_stack.clear();
        reloaded.dirty = false;

        int next_sheet_index = 0;
        if (!document.sheets.empty()) {
            const QString previous_sheet_name =
                document.sheets[static_cast<std::size_t>(
                    std::clamp(document.active_sheet_index, 0,
                               static_cast<int>(document.sheets.size()) - 1))].name;
            for (int candidate = 0; candidate < static_cast<int>(reloaded.sheets.size()); ++candidate) {
                if (reloaded.sheets[static_cast<std::size_t>(candidate)].name == previous_sheet_name) {
                    next_sheet_index = candidate;
                    break;
                }
            }
        }
        reloaded.active_sheet_index = next_sheet_index;
        documents_[static_cast<std::size_t>(index)] = std::move(reloaded);
        refresh_file_panel();

        if (had_active_document) {
            if (index == previous_active_document_index) {
                activate_document(index, previous_signal_index, false);
            } else {
                activate_document(previous_active_document_index, previous_signal_index);
                if (previous_sheet_index >= 0 &&
                    previous_active_document_index >= 0 &&
                    previous_active_document_index < static_cast<int>(documents_.size())) {
                    documents_[static_cast<std::size_t>(previous_active_document_index)].active_sheet_index =
                        std::min(previous_sheet_index,
                                 static_cast<int>(documents_[static_cast<std::size_t>(previous_active_document_index)]
                                                      .sheets.size()) - 1);
                    activate_document(previous_active_document_index, previous_signal_index);
                }
            }
        } else {
            service_.clear();
        }
        update_undo_action();
        refresh_status(tr("Reloaded %1 from disk").arg(document.display_name));
    } catch (const std::exception& ex) {
        documents_ = previous_documents;
        if (had_active_document) {
            activate_document(previous_active_document_index, previous_signal_index);
        } else {
            service_.clear();
            rebind_plot();
        }
        show_error(tr("Reload failed"),
                   QStringLiteral("%1\n\n%2")
                       .arg(document.path, QString::fromLocal8Bit(ex.what())));
    }
}

void MainWindow::onFileDetailsRequested(int index) { show_file_details(index); }

void MainWindow::onFileRenameRequested(int index, const QString& new_name) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }
    const QString trimmed_name = new_name.trimmed();
    if (trimmed_name.isEmpty()) {
        refresh_file_panel();
        return;
    }

    auto& document = documents_[static_cast<std::size_t>(index)];
    document.display_name = trimmed_name;
    if (index == active_document_index_) {
        refresh_status(tr("Workspace item renamed to %1").arg(trimmed_name));
    }
    refresh_file_panel();
}

void MainWindow::show_signal_options(int index) {
    if (active_document_index_ < 0 || index < 0 ||
        index >= static_cast<int>(service_.library().size())) {
        return;
    }

    const Signal& signal = service_.library().at(static_cast<std::size_t>(index));

    QDialog dialog(this);
    apply_application_icon(&dialog);
    dialog.setWindowTitle(tr("Signal options"));
    dialog.resize(520, 420);
    auto* layout = new QVBoxLayout(&dialog);

    auto* summary = new QLabel(
        tr("Signal: %1\nSamples: %2\nInterpolation: %3")
            .arg(QString::fromStdString(signal.name()))
            .arg(static_cast<qulonglong>(signal.size()))
            .arg(signal.interpolation() == Signal::InterpolationMode::Step ? tr("step")
                                                                           : tr("linear")),
        &dialog);
    summary->setWordWrap(true);
    layout->addWidget(summary);

    auto* info = new QLabel(&dialog);
    info->setWordWrap(true);
    layout->addWidget(info);

    auto* details_view = new QTextEdit(&dialog);
    details_view->setMinimumHeight(180);
    details_view->setReadOnly(true);
    layout->addWidget(details_view, 1);

    if (signal.is_enumerated()) {
        info->setText(
            tr("Edit one enumerated state per line using LABEL:NUMERIC_VALUE. "
               "When you apply the new mapping, plot and table are refreshed immediately."));
        details_view->setReadOnly(false);
        details_view->setPlainText(format_enumeration_definition(signal.enumeration()));
    } else {
        double min_value = 0.0;
        double max_value = 0.0;
        double mean_value = 0.0;
        double first_value = 0.0;
        double last_value = 0.0;
        double min_step = 0.0;
        double max_step = 0.0;
        bool has_samples = false;
        bool has_step = false;

        for (std::size_t sample_index = 0; sample_index < signal.size(); ++sample_index) {
            const auto& sample = signal.samples()[sample_index];
            if (!has_samples) {
                min_value = sample.y;
                max_value = sample.y;
                first_value = sample.y;
                last_value = sample.y;
                has_samples = true;
            } else {
                min_value = std::min(min_value, sample.y);
                max_value = std::max(max_value, sample.y);
                last_value = sample.y;
            }
            mean_value += sample.y;

            if (sample_index > 0U) {
                const double dt =
                    sample.t - signal.samples()[sample_index - 1U].t;
                if (!has_step) {
                    min_step = dt;
                    max_step = dt;
                    has_step = true;
                } else {
                    min_step = std::min(min_step, dt);
                    max_step = std::max(max_step, dt);
                }
            }
        }
        if (has_samples) {
            mean_value /= static_cast<double>(signal.size());
        }

        info->setText(
            tr("Numeric signal information. This window shows statistics and metadata useful "
               "for validation and inspection."));
        details_view->setPlainText(
            tr("Signal name: %1\n"
               "Samples: %2\n"
               "Interpolation: %3\n"
               "Enumerated: no\n"
               "Time range: %4 s .. %5 s\n"
               "Duration: %6 s\n"
               "Minimum value: %7\n"
               "Maximum value: %8\n"
               "Mean value: %9\n"
               "First sample value: %10\n"
               "Last sample value: %11\n"
               "Sampling step min: %12 s\n"
               "Sampling step max: %13 s")
                .arg(QString::fromStdString(signal.name()))
                .arg(static_cast<qulonglong>(signal.size()))
                .arg(signal.interpolation() == Signal::InterpolationMode::Step ? tr("step")
                                                                               : tr("linear"))
                .arg(signal.empty() ? 0.0 : signal.t_min(), 0, 'f', 6)
                .arg(signal.empty() ? 0.0 : signal.t_max(), 0, 'f', 6)
                .arg(signal.empty() ? 0.0 : signal.t_max() - signal.t_min(), 0, 'f', 6)
                .arg(has_samples ? min_value : 0.0, 0, 'f', 6)
                .arg(has_samples ? max_value : 0.0, 0, 'f', 6)
                .arg(has_samples ? mean_value : 0.0, 0, 'f', 6)
                .arg(has_samples ? first_value : 0.0, 0, 'f', 6)
                .arg(has_samples ? last_value : 0.0, 0, 'f', 6)
                .arg(has_step ? min_step : 0.0, 0, 'f', 6)
                .arg(has_step ? max_step : 0.0, 0, 'f', 6));
    }

    auto* buttons = new QDialogButtonBox(
        signal.is_enumerated() ? (QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
                               : QDialogButtonBox::Close,
        &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted || !signal.is_enumerated()) {
        return;
    }

    try {
        push_undo_state();
        const auto result = service_.set_signal_enumeration(
            static_cast<std::size_t>(index),
            parse_enumeration_definition(details_view->toPlainText()));
        if (!result.is_ok()) {
            discard_last_undo_state();
            show_error(tr("Signal options failed"), QString::fromStdString(result.message));
            rebind_plot();
            return;
        }
        mark_active_document_dirty();
        list_panel_->refresh();
        list_panel_->select(index);
        rebind_plot();
        refresh_status(tr("Updated enumeration mapping for %1")
                           .arg(QString::fromStdString(signal.name())));
    } catch (const std::exception& ex) {
        discard_last_undo_state();
        show_error(tr("Signal options failed"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::onRenameRequested(int index, const QString& new_name) {
    if (active_document_index_ < 0 || index < 0) { return; }
    push_undo_state();
    const auto result = service_.rename_signal(static_cast<std::size_t>(index),
                                               new_name.toStdString());
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Rename failed"), QString::fromStdString(result.message));
        list_panel_->refresh();
        return;
    }
    mark_active_document_dirty();
    list_panel_->refresh();
    rebind_plot();
    update_interpolation_box();
}

// ============================================================================
// Signal creation dialog
// ============================================================================

void MainWindow::onAddRequested() {
    double default_t_start = 0.0;
    double default_t_end = 1.0;
    QString default_sampling_time = QStringLiteral("0.01");
    if (active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size()) &&
        !documents_[static_cast<std::size_t>(active_document_index_)].path.trimmed().isEmpty()) {
        const auto& library = service_.library();
        for (const auto& signal : library.items()) {
            if (signal.empty()) {
                continue;
            }
            default_t_start = signal.t_min();
            default_t_end = signal.t_max();
            if (signal.size() >= 2U) {
                const double step =
                    signal.samples()[1].t - signal.samples()[0].t;
                if (step > 0.0) {
                    default_sampling_time =
                        format_line_edit_double(QLocale(), step, 6);
                }
            }
            break;
        }
    }

    QDialog dlg(this);
    apply_application_icon(&dlg);
    dlg.setWindowTitle(tr("New signal"));
    dlg.resize(560, 560);
    auto* layout      = new QVBoxLayout(&dlg);
    auto* common_form = new QFormLayout();

    auto* name_edit             = new QLineEdit(QStringLiteral("new_signal"), &dlg);
    auto* waveform_box          = new QComboBox(&dlg);
    auto* interpolation_box     = new QComboBox(&dlg);
    auto* t_start               = new QDoubleSpinBox(&dlg);
    auto* t_end                 = new QDoubleSpinBox(&dlg);
    auto* sampling_time_edit    = new QLineEdit(default_sampling_time, &dlg);
    auto* computed_samples_label = new QLabel(tr("Not computed yet"), &dlg);

    auto* signal_name_validator = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral(R"(^.*\S.*$)")), name_edit);
    name_edit->setValidator(signal_name_validator);
    sampling_time_edit->setValidator(
        new QRegularExpressionValidator(flexible_decimal_pattern(false), sampling_time_edit));
    sampling_time_edit->setAlignment(Qt::AlignRight);
    computed_samples_label->setObjectName(QStringLiteral("PanelDetail"));

    waveform_box->addItem(tr("Constant"));
    waveform_box->addItem(tr("Sine"));
    waveform_box->addItem(tr("Cosine"));
    waveform_box->addItem(tr("Pulse"));
    waveform_box->addItem(tr("Sawtooth"));
    waveform_box->addItem(tr("Triangle"));
    waveform_box->addItem(tr("Ramp"));
    waveform_box->addItem(tr("Enumerated"));
    interpolation_box->addItem(tr("Linear"));
    interpolation_box->addItem(tr("Step"));

    t_start->setRange(-1e9, 1e9);  t_start->setValue(default_t_start); t_start->setDecimals(6);
    t_end->setRange(-1e9, 1e9);    t_end->setValue(default_t_end);     t_end->setDecimals(6);

    common_form->addRow(tr("Name"),          name_edit);
    common_form->addRow(tr("Waveform"),      waveform_box);
    common_form->addRow(tr("Interpolation"), interpolation_box);
    common_form->addRow(tr("t start"),       t_start);
    common_form->addRow(tr("t end"),         t_end);
    common_form->addRow(tr("Sampling time"), sampling_time_edit);
    common_form->addRow(tr("Computed samples"), computed_samples_label);
    layout->addLayout(common_form);

    auto configure_spin = [](QDoubleSpinBox* box, double value) {
        box->setRange(-1e9, 1e9); box->setDecimals(6); box->setValue(value);
    };

    auto* params_stack = new QStackedWidget(&dlg);

    // ── Constant ──────────────────────────────────────────────────────────────
    auto* constant_page = new QWidget(params_stack);
    auto* constant_form = new QFormLayout(constant_page);
    auto* constant_level = new QDoubleSpinBox(constant_page);
    configure_spin(constant_level, 0.0);
    constant_form->addRow(tr("Level"), constant_level);
    params_stack->addWidget(constant_page);

    // ── Sine / Cosine ─────────────────────────────────────────────────────────
    auto make_periodic_page = [&](QStackedWidget* stack) -> std::tuple<
            QDoubleSpinBox*, QDoubleSpinBox*, QDoubleSpinBox*, QDoubleSpinBox*> {
        auto* page      = new QWidget(stack);
        auto* form      = new QFormLayout(page);
        auto* amplitude = new QDoubleSpinBox(page);
        auto* offset    = new QDoubleSpinBox(page);
        auto* frequency = new QDoubleSpinBox(page);
        auto* phase     = new QDoubleSpinBox(page);
        configure_spin(amplitude, 1.0);
        configure_spin(offset, 0.0);
        configure_spin(frequency, 1.0);
        configure_spin(phase, 0.0);
        frequency->setRange(1e-6, 1e9);
        phase->setRange(-3600.0, 3600.0);
        form->addRow(tr("Amplitude"),       amplitude);
        form->addRow(tr("Offset"),          offset);
        form->addRow(tr("Frequency [Hz]"),  frequency);
        form->addRow(tr("Phase [deg]"),     phase);
        stack->addWidget(page);
        return {amplitude, offset, frequency, phase};
    };
    auto [sine_amplitude, sine_offset, sine_frequency, sine_phase]
        = make_periodic_page(params_stack);
    auto [cosine_amplitude, cosine_offset, cosine_frequency, cosine_phase]
        = make_periodic_page(params_stack);

    // ── Pulse ─────────────────────────────────────────────────────────────────
    auto* pulse_page      = new QWidget(params_stack);
    auto* pulse_form      = new QFormLayout(pulse_page);
    auto* pulse_low       = new QDoubleSpinBox(pulse_page);
    auto* pulse_high      = new QDoubleSpinBox(pulse_page);
    auto* pulse_frequency = new QDoubleSpinBox(pulse_page);
    auto* pulse_duty      = new QDoubleSpinBox(pulse_page);
    auto* pulse_phase     = new QDoubleSpinBox(pulse_page);
    configure_spin(pulse_low, 0.0);
    configure_spin(pulse_high, 1.0);
    configure_spin(pulse_frequency, 1.0);
    configure_spin(pulse_duty, 50.0);
    configure_spin(pulse_phase, 0.0);
    pulse_frequency->setRange(1e-6, 1e9);
    pulse_duty->setRange(0.1, 99.9);
    pulse_phase->setRange(-3600.0, 3600.0);
    pulse_form->addRow(tr("Low level"),       pulse_low);
    pulse_form->addRow(tr("High level"),      pulse_high);
    pulse_form->addRow(tr("Frequency [Hz]"),  pulse_frequency);
    pulse_form->addRow(tr("Duty cycle [%]"),  pulse_duty);
    pulse_form->addRow(tr("Phase [deg]"),     pulse_phase);
    params_stack->addWidget(pulse_page);

    // ── Sawtooth ──────────────────────────────────────────────────────────────
    auto* saw_page      = new QWidget(params_stack);
    auto* saw_form      = new QFormLayout(saw_page);
    auto* saw_min       = new QDoubleSpinBox(saw_page);
    auto* saw_max       = new QDoubleSpinBox(saw_page);
    auto* saw_frequency = new QDoubleSpinBox(saw_page);
    auto* saw_phase     = new QDoubleSpinBox(saw_page);
    configure_spin(saw_min, -1.0); configure_spin(saw_max, 1.0);
    configure_spin(saw_frequency, 1.0); configure_spin(saw_phase, 0.0);
    saw_frequency->setRange(1e-6, 1e9); saw_phase->setRange(-3600.0, 3600.0);
    saw_form->addRow(tr("Min value"),       saw_min);
    saw_form->addRow(tr("Max value"),       saw_max);
    saw_form->addRow(tr("Frequency [Hz]"),  saw_frequency);
    saw_form->addRow(tr("Phase [deg]"),     saw_phase);
    params_stack->addWidget(saw_page);

    // ── Triangle ──────────────────────────────────────────────────────────────
    auto* triangle_page      = new QWidget(params_stack);
    auto* triangle_form      = new QFormLayout(triangle_page);
    auto* triangle_min       = new QDoubleSpinBox(triangle_page);
    auto* triangle_max       = new QDoubleSpinBox(triangle_page);
    auto* triangle_frequency = new QDoubleSpinBox(triangle_page);
    auto* triangle_phase     = new QDoubleSpinBox(triangle_page);
    configure_spin(triangle_min, -1.0); configure_spin(triangle_max, 1.0);
    configure_spin(triangle_frequency, 1.0); configure_spin(triangle_phase, 0.0);
    triangle_frequency->setRange(1e-6, 1e9); triangle_phase->setRange(-3600.0, 3600.0);
    triangle_form->addRow(tr("Min value"),       triangle_min);
    triangle_form->addRow(tr("Max value"),       triangle_max);
    triangle_form->addRow(tr("Frequency [Hz]"),  triangle_frequency);
    triangle_form->addRow(tr("Phase [deg]"),     triangle_phase);
    params_stack->addWidget(triangle_page);

    // ── Ramp ──────────────────────────────────────────────────────────────────
    auto* ramp_page  = new QWidget(params_stack);
    auto* ramp_form  = new QFormLayout(ramp_page);
    auto* ramp_start = new QDoubleSpinBox(ramp_page);
    auto* ramp_end   = new QDoubleSpinBox(ramp_page);
    configure_spin(ramp_start, 0.0); configure_spin(ramp_end, 1.0);
    ramp_form->addRow(tr("Start value"), ramp_start);
    ramp_form->addRow(tr("End value"),   ramp_end);
    params_stack->addWidget(ramp_page);

    // ── Enumerated ────────────────────────────────────────────────────────────
    auto* enum_page   = new QWidget(params_stack);
    auto* enum_layout = new QVBoxLayout(enum_page);
    enum_layout->setContentsMargins(0, 0, 0, 0);
    enum_layout->setSpacing(10);
    auto* enum_intro = new QLabel(
        tr("Define one state per line using LABEL:NUMERIC_VALUE. Example:\n"
           "TRUE:1\nFALSE:0\n"
           "The initial state can be left empty to use the first mapping entry."),
        enum_page);
    enum_intro->setWordWrap(true);
    auto* enum_mapping = new QTextEdit(enum_page);
    enum_mapping->setPlaceholderText(tr("TRUE:1\nFALSE:0"));
    enum_mapping->setMinimumHeight(140);
    auto* enum_initial_form  = new QFormLayout();
    auto* enum_initial_label = new QLineEdit(enum_page);
    auto* enum_initial_validator = new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral(R"(^(|.*\S.*)$)")), enum_initial_label);
    enum_initial_label->setValidator(enum_initial_validator);
    enum_initial_label->setPlaceholderText(tr("TRUE"));
    enum_initial_form->addRow(tr("Initial state"), enum_initial_label);
    enum_layout->addWidget(enum_intro);
    enum_layout->addWidget(enum_mapping);
    enum_layout->addLayout(enum_initial_form);
    enum_layout->addStretch(1);
    params_stack->addWidget(enum_page);

    layout->addWidget(params_stack);

    QObject::connect(waveform_box, qOverload<int>(&QComboBox::currentIndexChanged),
                     &dlg, [params_stack, interpolation_box](int index) {
                         params_stack->setCurrentIndex(index);
                         const bool is_enum =
                             static_cast<WaveformKind>(index) == WaveformKind::Enumerated;
                         if (is_enum) {
                             interpolation_box->setCurrentIndex(
                                 static_cast<int>(Signal::InterpolationMode::Step));
                         }
                         interpolation_box->setEnabled(!is_enum);
                     });

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    auto* ok_button = buttons->button(QDialogButtonBox::Ok);

    /**
     * @brief Refreshes the computed sample count shown in the creation dialog.
     *
     * The update is intentionally connected to `editingFinished`, not to
     * incremental text edits, so the preview only changes after the user
     * completes the sampling-time entry.
     */
    const auto refresh_sample_preview = [=]() {
        const QString sampling_text = sampling_time_edit->text().trimmed();
        if (sampling_text.isEmpty()) {
            computed_samples_label->setText(tr("Complete the sampling time to compute samples."));
            if (ok_button != nullptr) {
                ok_button->setEnabled(false);
            }
            return;
        }

        bool ok = false;
        const double sampling_time =
            parse_flexible_double(sampling_text, &ok);
        if (!ok || !(sampling_time > 0.0)) {
            computed_samples_label->setText(tr("Sampling time must be a positive number."));
            if (ok_button != nullptr) {
                ok_button->setEnabled(false);
            }
            return;
        }

        try {
            const std::size_t sample_count =
                compute_sample_count(t_start->value(), t_end->value(), sampling_time);
            computed_samples_label->setText(
                tr("%1 samples will be generated.")
                    .arg(QString::number(static_cast<qulonglong>(sample_count))));
            if (ok_button != nullptr) {
                ok_button->setEnabled(name_edit->hasAcceptableInput());
            }
        } catch (const std::exception& ex) {
            computed_samples_label->setText(QString::fromLatin1(ex.what()));
            if (ok_button != nullptr) {
                ok_button->setEnabled(false);
            }
        }
    };

    QObject::connect(name_edit, &QLineEdit::editingFinished, &dlg, [=]() {
        if (ok_button != nullptr && !name_edit->hasAcceptableInput()) {
            ok_button->setEnabled(false);
            return;
        }
        refresh_sample_preview();
    });
    QObject::connect(sampling_time_edit, &QLineEdit::editingFinished,
                     &dlg, refresh_sample_preview);
    QObject::connect(t_start, &QDoubleSpinBox::editingFinished, &dlg, refresh_sample_preview);
    QObject::connect(t_end, &QDoubleSpinBox::editingFinished, &dlg, refresh_sample_preview);
    refresh_sample_preview();

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [=, &dlg]() {
        if (!name_edit->hasAcceptableInput()) {
            QMessageBox::warning(&dlg, tr("Invalid name"),
                                 tr("Insert a non-empty signal name."));
            return;
        }
        refresh_sample_preview();
        if (ok_button != nullptr && !ok_button->isEnabled()) {
            QMessageBox::warning(&dlg, tr("Invalid sampling time"),
                                 tr("Insert a valid sampling time before creating the signal."));
            return;
        }
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) { return; }

    const auto waveform = static_cast<WaveformKind>(waveform_box->currentIndex());
    const QString signal_name = name_edit->text().trimmed();
    if (signal_name.isEmpty()) {
        show_error(tr("Create failed"), tr("Signal name must not be empty."));
        return;
    }
    bool sampling_time_ok = false;
    const double sampling_time =
        parse_flexible_double(sampling_time_edit->text().trimmed(), &sampling_time_ok);
    if (!sampling_time_ok || !(sampling_time > 0.0)) {
        show_error(tr("Create failed"), tr("Sampling time must be a positive number."));
        return;
    }

    std::array<double, 4> params_a{0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> params_b{0.0, 0.0, 0.0, 0.0};

    switch (waveform) {
    case WaveformKind::Constant:
        params_a[0] = constant_level->value();
        break;
    case WaveformKind::Sine:
        params_a = {sine_amplitude->value(), sine_offset->value(),
                    sine_frequency->value(), sine_phase->value()};
        break;
    case WaveformKind::Cosine:
        params_a = {cosine_amplitude->value(), cosine_offset->value(),
                    cosine_frequency->value(), cosine_phase->value()};
        break;
    case WaveformKind::Pulse:
        params_a = {pulse_low->value(), pulse_high->value(),
                    pulse_frequency->value(), pulse_duty->value()};
        params_b[0] = pulse_phase->value();
        break;
    case WaveformKind::Sawtooth:
        params_a = {saw_min->value(), saw_max->value(),
                    saw_frequency->value(), saw_phase->value()};
        break;
    case WaveformKind::Triangle:
        params_a = {triangle_min->value(), triangle_max->value(),
                    triangle_frequency->value(), triangle_phase->value()};
        break;
    case WaveformKind::Ramp:
        params_a = {ramp_start->value(), ramp_end->value(), 0.0, 0.0};
        break;
    case WaveformKind::Enumerated:
        break;
    }

    try {
        const std::size_t sample_count =
            compute_sample_count(t_start->value(), t_end->value(), sampling_time);
        QProgressDialog progress_dialog(
            tr("Generating %1 samples...").arg(static_cast<qulonglong>(sample_count)),
            tr("Cancel"),
            0,
            1000,
            this);
        apply_application_icon(&progress_dialog);
        progress_dialog.setWindowModality(Qt::ApplicationModal);
        progress_dialog.setMinimumDuration(sample_count > 250'000U ? 0 : 1000);
        progress_dialog.setValue(0);

        const GenerationProgress progress = [&progress_dialog](std::size_t completed,
                                                               std::size_t total) {
            const int value = total == 0U
                ? 0
                : static_cast<int>((static_cast<unsigned long long>(completed) * 1000ULL) /
                                   static_cast<unsigned long long>(total));
            progress_dialog.setValue(std::clamp(value, 0, 1000));
            QApplication::processEvents(QEventLoop::AllEvents, 50);
            return !progress_dialog.wasCanceled();
        };

        Signal signal = waveform == WaveformKind::Enumerated
            ? generate_enumerated_signal(signal_name,
                                         t_start->value(), t_end->value(),
                                         sampling_time,
                                         parse_enumeration_definition(enum_mapping->toPlainText()),
                                         enum_initial_label->text(),
                                         progress)
            : generate_waveform_signal(waveform, signal_name,
                                       t_start->value(), t_end->value(),
                                       sampling_time,
                                       params_a, params_b,
                                       progress);
        progress_dialog.setValue(1000);

        if (ensure_workspace_document() < 0) {
            show_error(tr("Create failed"),
                       tr("Unable to create an empty workspace for the new signal."));
            return;
        }

        if (!signal.is_enumerated()) {
            signal.set_interpolation(
                static_cast<Signal::InterpolationMode>(interpolation_box->currentIndex()));
        }

        push_undo_state();
        const auto result = service_.add_signal(std::move(signal));
        if (!result.is_ok()) {
            discard_last_undo_state();
            show_error(tr("Create failed"), QString::fromStdString(result.message));
            return;
        }
        mark_active_document_dirty();
        list_panel_->refresh();
        list_panel_->select(static_cast<int>(service_.library().size()) - 1);
        rebind_plot();
    } catch (const std::exception& ex) {
        show_error(tr("Create failed"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::onRemoveRequested(int index) {
    if (active_document_index_ < 0 || index < 0 ||
        index >= static_cast<int>(service_.library().size())) {
        return;
    }
    auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        return;
    }
    const std::vector<int> previous_visible = sheet->visible_signal_indices;
    push_undo_state();
    const auto result = service_.remove_signal(static_cast<std::size_t>(index));
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Remove failed"), QString::fromStdString(result.message));
        return;
    }
    mark_active_document_dirty();
    sheet->visible_signal_indices.clear();
    for (int visible_index : previous_visible) {
        if (visible_index == index) {
            continue;
        }
        sheet->visible_signal_indices.push_back(visible_index > index
                                                    ? visible_index - 1
                                                    : visible_index);
    }
    normalize_visible_signal_indices(*sheet);
    list_panel_->set_visible_signal_indices(sheet->visible_signal_indices);
    list_panel_->refresh();
    if (!service_.library().empty()) {
        const int next_index = std::min(index, static_cast<int>(service_.library().size()) - 1);
        sheet->active_signal_index = next_index;
        list_panel_->select(next_index);
    } else if (active_document_index_ >= 0 &&
               active_document_index_ < static_cast<int>(documents_.size())) {
        sheet->active_signal_index = -1;
    }
    rebind_plot();
}

// ============================================================================
// Plot / table / interpolation slots
// ============================================================================

void MainWindow::onPlotEditStarted() { push_undo_state(); }

void MainWindow::onPlotSampleMoveRequested(std::size_t sample_index, double t, double y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.move_sample_point(static_cast<std::size_t>(signal_index),
                                                   sample_index, t, y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Plot edit failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotSampleInsertRequested(double t, double y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.insert_sample(static_cast<std::size_t>(signal_index), t, y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Insert failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotSampleRemoveRequested(std::size_t sample_index) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.remove_sample(static_cast<std::size_t>(signal_index), sample_index);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Remove failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotSegmentMoveRequested(std::size_t start_index, double delta_y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.move_segment(static_cast<std::size_t>(signal_index),
                                              start_index, delta_y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Segment drag failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotGaussianBrushRequested(double t_center, double delta_y, double sigma) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.apply_gaussian_brush(static_cast<std::size_t>(signal_index),
                                                      t_center, delta_y, sigma);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Brush edit failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onOffsetAllRequested(double delta_y) {
    if (active_document_index_ < 0) {
        return;
    }
    const auto result = service_.apply_offset_to_all_samples(delta_y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Offset failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotSampleOffsetRequested(std::size_t sample_index, double delta_y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.apply_offset_to_sample(static_cast<std::size_t>(signal_index),
                                                        sample_index, delta_y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Offset failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onPlotChanged() {
    if (active_document_index_ < 0) { return; }
    mark_active_document_dirty();
    if (!plot_->is_drag_active()) {
        list_panel_->refresh();
        plot_->refresh();
        table_panel_->refresh();
    }
    refresh_status();
}

void MainWindow::onTableEditStarted() { push_undo_state(); }

void MainWindow::onTableChanged() {
    if (active_document_index_ < 0) { return; }
    mark_active_document_dirty();
    list_panel_->refresh();
    rebind_plot();
    update_interpolation_box();
}

void MainWindow::onTableSampleEdited(int row, double t, double y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 ||
        signal_index < 0 ||
        signal_index >= static_cast<int>(service_.library().size())) {
        return;
    }

    const auto& current_signal = service_.library().at(static_cast<std::size_t>(signal_index));
    if (row < 0 || row >= static_cast<int>(current_signal.size())) {
        return;
    }

    const auto& original_sample = current_signal.samples()[static_cast<std::size_t>(row)];
    signal_editor::Result result = std::fabs(original_sample.t - t) > kSamplingEpsilon
        ? service_.move_sample_point(static_cast<std::size_t>(signal_index),
                                     static_cast<std::size_t>(row), t, y)
        : service_.move_sample(static_cast<std::size_t>(signal_index),
                               static_cast<std::size_t>(row), y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Table edit failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onTableSampleInserted(double t, double y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0) {
        return;
    }
    const auto result = service_.insert_sample(static_cast<std::size_t>(signal_index), t, y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Insert failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onTableSampleRemoved(int row) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0 || row < 0) {
        return;
    }
    const auto result = service_.remove_sample(static_cast<std::size_t>(signal_index),
                                               static_cast<std::size_t>(row));
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Remove failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onTableSampleOffsetRequested(int row, double delta_y) {
    const int signal_index = active_signal_index();
    if (active_document_index_ < 0 || signal_index < 0 || row < 0) {
        return;
    }
    const auto result = service_.apply_offset_to_sample(static_cast<std::size_t>(signal_index),
                                                        static_cast<std::size_t>(row), delta_y);
    if (!result.is_ok()) {
        discard_last_undo_state();
        show_error(tr("Offset failed"), QString::fromStdString(result.message));
        rebind_plot();
    }
}

void MainWindow::onSignalInterpolationChanged(int mode) {
    if (active_document_index_ < 0) { return; }
    const auto plotted_indices = active_visible_signal_indices();
    if (plotted_indices.empty()) {
        return;
    }
    const std::pair<double, double> preserved_time_view =
        plot_ != nullptr ? plot_->time_view() : std::pair<double, double>{0.0, 0.0};
    push_undo_state();
    for (int signal_index : plotted_indices) {
        if (signal_index < 0 || signal_index >= static_cast<int>(service_.library().size())) {
            continue;
        }
        const auto result = service_.set_signal_interpolation(
            static_cast<std::size_t>(signal_index),
            static_cast<Signal::InterpolationMode>(mode));
        if (!result.is_ok()) {
            discard_last_undo_state();
            show_error(tr("Interpolation change failed"), QString::fromStdString(result.message));
            rebind_plot();
            return;
        }
    }
    mark_active_document_dirty();
    list_panel_->refresh();
    plot_->refresh();
    if (plot_ != nullptr) {
        (void)plot_->set_time_view(preserved_time_view.first, preserved_time_view.second);
    }
    table_panel_->refresh();
    update_interpolation_box();
}

void MainWindow::onCursorMoved(double t, double y) {
    const int signal_index = active_signal_index();
    const Signal* signal = (signal_index >= 0 &&
                            signal_index < static_cast<int>(service_.library().size()))
        ? &service_.library().at(static_cast<std::size_t>(signal_index))
        : nullptr;
    refresh_status(QStringLiteral("t = %1   y = %2")
        .arg(t, 0, 'f', 4).arg(format_signal_value(signal, y)));
}

void MainWindow::onPlotZoomInRequested() {
    if (plot_ == nullptr) {
        return;
    }
    plot_->zoom_in_time();
}

void MainWindow::onPlotZoomOutRequested() {
    if (plot_ == nullptr) {
        return;
    }
    plot_->zoom_out_time();
}

void MainWindow::onPlotPanModeToggled(bool checked) {
    if (plot_ == nullptr) {
        return;
    }
    if (checked) {
        plot_->set_navigation_mode(SignalPlotWidget::NavigationMode::Pan);
    } else if (plot_->navigation_mode() == SignalPlotWidget::NavigationMode::Pan) {
        plot_->set_navigation_mode(SignalPlotWidget::NavigationMode::Edit);
    }
    sync_plot_navigation_mode_controls();
}

void MainWindow::onPlotRectModeToggled(bool checked) {
    if (plot_ == nullptr) {
        return;
    }
    if (checked) {
        plot_->set_navigation_mode(SignalPlotWidget::NavigationMode::RectZoom);
    } else if (plot_->navigation_mode() == SignalPlotWidget::NavigationMode::RectZoom) {
        plot_->set_navigation_mode(SignalPlotWidget::NavigationMode::Edit);
    }
    sync_plot_navigation_mode_controls();
}

void MainWindow::onPlotRangeEditingFinished() {
    if (plot_ == nullptr || plot_t_start_edit_ == nullptr || plot_t_end_edit_ == nullptr) {
        return;
    }

    bool ok_start = false;
    bool ok_end = false;
    const double t_start =
        parse_flexible_double(plot_t_start_edit_->text().trimmed(), &ok_start);
    const double t_end =
        parse_flexible_double(plot_t_end_edit_->text().trimmed(), &ok_end);
    if (!ok_start || !ok_end) {
        sync_plot_view_controls();
        return;
    }
    if (!plot_->set_time_view(t_start, t_end)) {
        sync_plot_view_controls();
    }
}

void MainWindow::onPlotTimeViewChanged(double t_start, double t_end) {
    if (plot_t_start_edit_ == nullptr || plot_t_end_edit_ == nullptr) {
        return;
    }
    const QSignalBlocker block_start(plot_t_start_edit_);
    const QSignalBlocker block_end(plot_t_end_edit_);
    plot_t_start_edit_->setText(format_line_edit_double(plot_t_start_edit_->locale(), t_start, 6));
    plot_t_end_edit_->setText(format_line_edit_double(plot_t_end_edit_->locale(), t_end, 6));
}

// ============================================================================
// Private helpers
// ============================================================================

void MainWindow::open_paths(const QStringList& paths) {
    QStringList normalized_paths;
    for (const QString& path : paths) {
        if (!path.trimmed().isEmpty()) {
            normalized_paths.push_back(path);
        }
    }
    if (normalized_paths.isEmpty()) {
        return;
    }

    if (active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size())) {
        sync_active_document_from_service();
    }

    auto* progress = new QProgressDialog(
        normalized_paths.size() == 1
            ? tr("Loading %1...").arg(QFileInfo(normalized_paths.front()).fileName())
            : tr("Loading %1 files...").arg(normalized_paths.size()),
        tr("Cancel"),
        0,
        normalized_paths.size(),
        this);
    apply_application_icon(progress);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    progress->setAutoReset(false);
    progress->setValue(0);
    progress->show();

    auto result = std::make_shared<AsyncLoadResult>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    QPointer<QProgressDialog> progress_guard(progress);
    connect(progress, &QProgressDialog::canceled, this, [cancelled]() {
        cancelled->store(true);
    });

    QThread* worker = QThread::create([this, normalized_paths, result, cancelled, progress_guard]() {
        for (qsizetype index = 0; index < normalized_paths.size(); ++index) {
            if (cancelled->load()) {
                break;
            }

            const QString path = normalized_paths.at(index);
            QMetaObject::invokeMethod(this, [this, progress_guard, path, index, total = normalized_paths.size()]() {
                if (progress_guard == nullptr) {
                    return;
                }
                progress_guard->setLabelText(
                    tr("Loading %1 (%2/%3)...")
                        .arg(QFileInfo(path).fileName())
                        .arg(index + 1)
                        .arg(total));
                progress_guard->setValue(static_cast<int>(index));
            }, Qt::QueuedConnection);

            try {
                result->loaded.push_back(AsyncLoadResult::LoadedPath{
                    path,
                    load_document_state(path),
                });
            } catch (const std::exception& ex) {
                result->failed.push_back(AsyncLoadResult::FailedPath{
                    path,
                    QString::fromLocal8Bit(ex.what()),
                });
            } catch (...) {
                result->failed.push_back(AsyncLoadResult::FailedPath{
                    path,
                    QStringLiteral("Unknown error while loading the document."),
                });
            }
        }
    });

    connect(worker, &QThread::finished, this, [this, worker, progress_guard, result]() mutable {
        if (progress_guard != nullptr) {
            progress_guard->setValue(progress_guard->maximum());
            progress_guard->close();
            progress_guard->deleteLater();
        }
        apply_loaded_documents(std::move(*result));
        worker->deleteLater();
    });
    worker->start();
}

void MainWindow::load_document(const QString& path) {
    const bool had_active_document =
        active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size());
    const int previous_document_index = active_document_index_;
    const int previous_signal_index = active_signal_index();
    const int previous_sheet_index = active_sheet_index();
    if (had_active_document) {
        sync_active_document_from_service();
    }
    const auto previous_documents = documents_;

    try {
        switching_document_ = true;
        list_panel_->set_library(nullptr);
        list_panel_->set_visible_signal_indices({});
        list_panel_->set_active_signal_index(-1);
        plot_->set_library(nullptr, -1, {});
        table_panel_->set_library(nullptr, -1, {});
        switching_document_ = false;

        LoadedDocument document = load_document_state(path);
        documents_.push_back(std::move(document));

        refresh_file_panel();
        activate_document(static_cast<int>(documents_.size()) - 1);
        refresh_status(tr("Loaded %1").arg(path));
    } catch (...) {
        documents_ = previous_documents;
        if (had_active_document) {
            if (previous_sheet_index >= 0 &&
                previous_document_index >= 0 &&
                previous_document_index < static_cast<int>(documents_.size())) {
                documents_[static_cast<std::size_t>(previous_document_index)].active_sheet_index =
                    previous_sheet_index;
            }
            activate_document(previous_document_index, previous_signal_index);
        } else {
            service_.clear();
            active_document_index_ = -1;
            list_panel_->set_library(nullptr);
            list_panel_->set_visible_signal_indices({});
            list_panel_->set_active_signal_index(-1);
            plot_->set_library(nullptr, -1, {});
            table_panel_->set_library(nullptr, -1, {});
            refresh_file_panel();
            refresh_status();
        }
        throw;
    }
}

void MainWindow::apply_loaded_documents(AsyncLoadResult result) {
    if (result.loaded.empty()) {
        for (const auto& failure : result.failed) {
            show_error(tr("Load failed"),
                       QStringLiteral("%1\n\n%2").arg(failure.path, failure.message));
        }
        refresh_status(result.failed.empty() ? tr("Load cancelled") : tr("No files loaded"));
        return;
    }

    const int first_new_index = static_cast<int>(documents_.size());
    for (auto& loaded : result.loaded) {
        documents_.push_back(std::move(loaded.document));
    }

    refresh_file_panel();
    activate_document(static_cast<int>(documents_.size()) - 1);

    if (result.failed.empty()) {
        refresh_status(result.loaded.size() == 1U
                           ? tr("Loaded %1").arg(result.loaded.front().path)
                           : tr("Loaded %1 files").arg(result.loaded.size()));
    } else {
        refresh_status(tr("Loaded %1 files, %2 failed")
                           .arg(result.loaded.size())
                           .arg(result.failed.size()));
        for (const auto& failure : result.failed) {
            show_error(tr("Load failed"),
                       QStringLiteral("%1\n\n%2").arg(failure.path, failure.message));
        }
    }

    if (first_new_index >= 0 && first_new_index < static_cast<int>(documents_.size())) {
        file_panel_->set_opened_index(static_cast<int>(documents_.size()) - 1);
    }
}

MainWindow::LoadedDocument MainWindow::load_document_state(const QString& path) const {
    const DocumentFormat format = document_format_for_path(path);

    auto make_sheet_state = [&](const QString& name, SignalLibrary library) {
        LoadedSheetState sheet;
        sheet.name = name;
        sheet.library = std::move(library);
        sheet.visible_signal_indices = sheet.library.empty() ? std::vector<int>{} : std::vector<int>{0};
        sheet.active_signal_index = sheet.library.empty() ? -1 : 0;
        return sheet;
    };

    LoadedDocument document;
    document.path = path;
    document.display_name = QFileInfo(path).fileName();
    document.format = format;
    document.explicit_sheet_names = false;
    document.active_sheet_index = 0;
    document.undo_stack.clear();
    document.dirty = false;

    switch (format) {
    case DocumentFormat::Csv: {
        CsvSignalRepository repository;
        document.sheets.push_back(make_sheet_state(
            QString{},
            repository.load(std::filesystem::path(path.toStdString()))));
        break;
    }
    case DocumentFormat::SpreadsheetXml: {
        SpreadsheetXmlSignalRepository repository;
        const auto workbook = repository.load_workbook(std::filesystem::path(path.toStdString()));
        document.explicit_sheet_names = true;
        for (const auto& sheet : workbook.sheets) {
            document.sheets.push_back(make_sheet_state(QString::fromStdString(sheet.name), sheet.library));
        }
        break;
    }
    case DocumentFormat::Xlsx: {
        XlsxSignalRepository repository;
        const auto workbook = repository.load_workbook(std::filesystem::path(path.toStdString()));
        document.explicit_sheet_names = true;
        for (const auto& sheet : workbook.sheets) {
            document.sheets.push_back(make_sheet_state(QString::fromStdString(sheet.name), sheet.library));
        }
        break;
    }
    case DocumentFormat::Delimited: {
        DelimitedSignalRepository repository('\t');
        document.sheets.push_back(make_sheet_state(QString{},
                                                   repository.load(std::filesystem::path(path.toStdString()))));
        break;
    }
    case DocumentFormat::Json: {
        JsonSignalRepository repository;
        document.sheets.push_back(make_sheet_state(QString{},
                                                   repository.load(std::filesystem::path(path.toStdString()))));
        break;
    }
    case DocumentFormat::Unknown:
        throw std::runtime_error("Unsupported import format");
    }

    if (document.sheets.empty()) {
        document.sheets.push_back(make_sheet_state(QString{}, SignalLibrary{}));
    }
    return document;
}

signal_editor::Result MainWindow::save_document_state(const LoadedDocument& document, const QString& path) const {
    WorkbookDocument workbook;
    workbook.explicit_sheet_names = document.explicit_sheet_names || document.sheets.size() > 1U;
    for (const auto& sheet : document.sheets) {
        workbook.sheets.push_back(WorkbookSheet{sheet.name.toStdString(), sheet.library});
    }

    const DocumentFormat format = document_format_for_path(path);
    switch (format) {
    case DocumentFormat::Csv: {
        CsvSignalRepository repository;
        if (workbook.sheets.size() > 1U) {
            return signal_editor::Result::error(
                "CSV files do not support multiple worksheets. Save as XLSX or SpreadsheetML XML.");
        }
        return repository.save(std::filesystem::path(path.toStdString()), workbook.sheets.front().library);
    }
    case DocumentFormat::SpreadsheetXml: {
        SpreadsheetXmlSignalRepository repository;
        return repository.save_workbook(std::filesystem::path(path.toStdString()), workbook);
    }
    case DocumentFormat::Xlsx: {
        XlsxSignalRepository repository;
        return repository.save_workbook(std::filesystem::path(path.toStdString()), workbook);
    }
    case DocumentFormat::Delimited: {
        if (workbook.sheets.size() > 1U) {
            return signal_editor::Result::error(
                "Multi-sheet documents cannot be saved as TSV/TXT. Use CSV, XML, or XLSX.");
        }
        DelimitedSignalRepository repository('\t');
        return repository.save(std::filesystem::path(path.toStdString()), workbook.sheets.front().library);
    }
    case DocumentFormat::Json: {
        if (workbook.sheets.size() > 1U) {
            return signal_editor::Result::error(
                "Multi-sheet documents cannot be saved as JSON. Use CSV, XML, or XLSX.");
        }
        JsonSignalRepository repository;
        return repository.save(std::filesystem::path(path.toStdString()), workbook.sheets.front().library);
    }
    case DocumentFormat::Unknown:
        return signal_editor::Result::error("Unsupported export format.");
    }
    return signal_editor::Result::error("Unsupported export format.");
}

/**
 * @brief Ensures that the editor has an active in-memory workspace document.
 *
 * The helper is used when the user starts from an empty application state and
 * creates a signal before importing any file. The created document has no path
 * yet and becomes persistable through the regular Save flow.
 *
 * @return Active document index, or `-1` when activation failed.
 */
int MainWindow::ensure_workspace_document() {
    if (active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size())) {
        return active_document_index_;
    }

    service_.clear();

    LoadedDocument document;
    document.path = {};
    document.format = DocumentFormat::Csv;
    document.explicit_sheet_names = false;
    document.sheets.push_back(LoadedSheetState{
        QString{},
        service_.library(),
        service_.library().empty() ? std::vector<int>{} : std::vector<int>{0},
        service_.library().empty() ? -1 : 0,
    });
    document.active_sheet_index = 0;
    document.undo_stack.clear();
    document.dirty = false;

    const QString base_name = tr("Untitled workspace");
    QString display_name = base_name;
    int suffix = 2;
    while (std::any_of(documents_.begin(), documents_.end(),
                       [&](const LoadedDocument& existing) {
                           return existing.display_name == display_name;
                       })) {
        display_name = tr("%1 %2").arg(base_name).arg(suffix++);
    }
    document.display_name = display_name;

    documents_.push_back(std::move(document));
    refresh_file_panel();
    activate_document(static_cast<int>(documents_.size()) - 1);
    return active_document_index_;
}

void MainWindow::sync_active_document_from_service() {
    auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        return;
    }
    sheet->library = service_.library();
    normalize_visible_signal_indices(*sheet);
}

void MainWindow::activate_document(int index, int preferred_signal_index, bool sync_current_document) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        active_document_index_ = -1;
        service_.clear();
        list_panel_->set_library(nullptr);
        list_panel_->set_visible_signal_indices({});
        list_panel_->set_active_signal_index(-1);
        file_panel_->set_opened_index(-1);
        plot_->set_library(nullptr, -1, {});
        table_panel_->set_library(nullptr, -1, {});
        update_undo_action();
        refresh_status();
        return;
    }
    if (sync_current_document) {
        sync_active_document_from_service();
    }
    switching_document_    = true;
    active_document_index_ = index;
    auto& document = documents_[static_cast<std::size_t>(index)];
    if (document.sheets.empty()) {
        document.sheets.push_back(LoadedSheetState{QString{}, SignalLibrary{}, {}, -1});
    }
    document.active_sheet_index = std::clamp(
        document.active_sheet_index, 0, static_cast<int>(document.sheets.size()) - 1);
    auto& sheet = document.sheets[static_cast<std::size_t>(document.active_sheet_index)];
    service_.set_library(sheet.library);
    normalize_visible_signal_indices(sheet);
    list_panel_->set_library(&service_.library());
    list_panel_->set_visible_signal_indices(sheet.visible_signal_indices);
    int resolved_index = preferred_signal_index;
    if (resolved_index < 0) {
        resolved_index = sheet.active_signal_index;
    }
    if (!service_.library().empty() && resolved_index >= 0) {
        const int bounded_index = std::clamp(resolved_index, 0,
                                             static_cast<int>(service_.library().size()) - 1);
        sheet.active_signal_index = bounded_index;
        list_panel_->set_active_signal_index(bounded_index);
        list_panel_->select(bounded_index);
    } else {
        sheet.active_signal_index = -1;
        list_panel_->set_active_signal_index(-1);
        list_panel_->select(-1);
    }
    file_panel_->select(index, true);
    file_panel_->set_opened_index(index);
    switching_document_ = false;
    rebind_plot();
    update_undo_action();
    refresh_status();
}

void MainWindow::mark_active_document_dirty(bool dirty) {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    sync_active_document_from_service();
    documents_[static_cast<std::size_t>(active_document_index_)].dirty = dirty;
    update_undo_action();
    refresh_file_panel();
}

void MainWindow::push_undo_state() {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    sync_active_document_from_service();
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    UndoState state;
    state.active_sheet_index = document.active_sheet_index;
    state.sheets.reserve(document.sheets.size());
    for (const auto& sheet : document.sheets) {
        state.sheets.push_back(
            UndoState::SheetState{sheet.name, sheet.library, sheet.visible_signal_indices, sheet.active_signal_index});
    }
    document.undo_stack.push_back(std::move(state));
    update_undo_action();
}

void MainWindow::discard_last_undo_state() {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (!document.undo_stack.empty()) { document.undo_stack.pop_back(); }
    update_undo_action();
}

void MainWindow::clear_undo_history() {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return;
    }
    documents_[static_cast<std::size_t>(active_document_index_)].undo_stack.clear();
    update_undo_action();
}

void MainWindow::show_file_details(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) { return; }
    auto& document = documents_[static_cast<std::size_t>(index)];
    QFileInfo file_info(document.path);
    const bool show_sheet_selector = document.format == DocumentFormat::Xlsx;

    QDialog dialog(this);
    apply_application_icon(&dialog);
    dialog.setWindowTitle(tr("File options"));
    dialog.resize(820, 620);
    auto* dlg_layout = new QVBoxLayout(&dialog);

    auto* summary_label = new QLabel(&dialog);
    summary_label->setWordWrap(true);
    QString summary;
    summary += tr("File: %1\n").arg(document.display_name);
    summary += tr("Path: %1\n").arg(document.path);
    summary += tr("Format: %1\n")
        .arg(QFileInfo(document.path).suffix().trimmed().isEmpty()
                 ? tr("workspace")
                 : QFileInfo(document.path).suffix().trimmed().toUpper());
    if (show_sheet_selector) {
        summary += tr("Sheets detected: %1\n").arg(static_cast<qulonglong>(document.sheets.size()));
    }
    summary += tr("Workspace modified: %1\n").arg(document.dirty ? tr("yes") : tr("no"));
    summary += tr("Undo steps available: %1\n")
        .arg(static_cast<qulonglong>(document.undo_stack.size()));
    if (file_info.exists()) {
        summary += tr("Size: %1 bytes\n").arg(file_info.size());
        summary += tr("Last modified: %1\n").arg(file_info.lastModified().toString(Qt::ISODate));
    }
    summary_label->setText(summary);
    dlg_layout->addWidget(summary_label);

    QLabel* sheet_label = nullptr;
    QComboBox* sheet_combo = nullptr;
    if (show_sheet_selector) {
        sheet_label = new QLabel(tr("Active sheet"), &dialog);
        dlg_layout->addWidget(sheet_label);

        sheet_combo = new QComboBox(&dialog);
        for (const auto& sheet : document.sheets) {
            sheet_combo->addItem(sheet.name.isEmpty() ? tr("Sheet") : sheet.name);
        }
        sheet_combo->setCurrentIndex(std::clamp(document.active_sheet_index, 0,
                                                std::max(0, static_cast<int>(document.sheets.size()) - 1)));
        dlg_layout->addWidget(sheet_combo);
    }

    auto* text_view = new QTextEdit(&dialog);
    text_view->setReadOnly(true);
    dlg_layout->addWidget(text_view, 1);

    const auto refresh_sheet_details = [&]() {
        const int sheet_index = show_sheet_selector && sheet_combo != nullptr
            ? sheet_combo->currentIndex()
            : std::clamp(document.active_sheet_index, 0,
                         std::max(0, static_cast<int>(document.sheets.size()) - 1));
        if (sheet_index < 0 || sheet_index >= static_cast<int>(document.sheets.size())) {
            text_view->clear();
            return;
        }

        const auto& sheet = document.sheets[static_cast<std::size_t>(sheet_index)];
        std::size_t total_samples = 0;
        std::size_t enumerated_signals = 0;
        double time_min = 0.0;
        double time_max = 0.0;
        bool has_samples = false;
        QStringList signal_summaries;
        QStringList columns;
        signal_summaries.reserve(static_cast<int>(sheet.library.size()));
        columns.reserve(static_cast<int>(sheet.library.size()) + 1);
        columns.push_back(QStringLiteral("time"));
        for (const auto& signal : sheet.library.items()) {
            columns.push_back(QString::fromStdString(signal.name()));
            total_samples += signal.size();
            if (signal.is_enumerated()) {
                ++enumerated_signals;
            }
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
        if (show_sheet_selector) {
            details += tr("Sheet name: %1\n").arg(sheet.name);
        }
        details += tr("Signals loaded: %1\n").arg(static_cast<qulonglong>(sheet.library.size()));
        details += tr("Visible in plot: %1\n").arg(static_cast<qulonglong>(sheet.visible_signal_indices.size()));
        details += tr("Total samples: %1\n").arg(static_cast<qulonglong>(total_samples));
        details += tr("Enumerated signals: %1\n").arg(static_cast<qulonglong>(enumerated_signals));
        details += tr("Columns: %1\n").arg(columns.join(QStringLiteral(", ")));
        if (has_samples) {
            details += tr("Global time range: [%1, %2]\n").arg(time_min, 0, 'f', 4).arg(time_max, 0, 'f', 4);
        }
        details += tr("\nSignals:\n");
        details += signal_summaries.isEmpty() ? tr("- none") : signal_summaries.join(QStringLiteral("\n"));
        text_view->setPlainText(details);
    };
    refresh_sheet_details();
    if (show_sheet_selector && sheet_combo != nullptr) {
        connect(sheet_combo, &QComboBox::currentIndexChanged, &dialog,
                [&](int) { refresh_sheet_details(); });
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, &dialog);
    QPushButton* apply_button = buttons->button(QDialogButtonBox::Apply);
    apply_button->setVisible(show_sheet_selector);
    apply_button->setEnabled(show_sheet_selector &&
                             sheet_combo != nullptr &&
                             sheet_combo->currentIndex() != document.active_sheet_index);
    if (show_sheet_selector && sheet_combo != nullptr) {
        connect(sheet_combo, &QComboBox::currentIndexChanged, &dialog,
                [&](int new_index) { apply_button->setEnabled(new_index != document.active_sheet_index); });
    }
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(apply_button, &QPushButton::clicked, &dialog, [&]() {
        if (!show_sheet_selector || sheet_combo == nullptr) {
            dialog.accept();
            return;
        }
        const int new_sheet_index = sheet_combo->currentIndex();
        if (new_sheet_index < 0 || new_sheet_index >= static_cast<int>(document.sheets.size())) {
            dialog.reject();
            return;
        }
        if (index == active_document_index_) {
            const bool preserve_time_view = plot_ != nullptr;
            const auto previous_time_view =
                preserve_time_view ? plot_->time_view() : std::pair<double, double>{0.0, 0.0};
            sync_active_document_from_service();
            document.active_sheet_index = new_sheet_index;
            activate_document(index,
                              document.sheets[static_cast<std::size_t>(new_sheet_index)].active_signal_index,
                              false);
            if (preserve_time_view && plot_ != nullptr && active_signal_index() >= 0) {
                (void)plot_->set_time_view(previous_time_view.first, previous_time_view.second);
                sync_plot_view_controls();
            }
        } else {
            document.active_sheet_index = new_sheet_index;
            refresh_file_panel();
        }
        dialog.accept();
    });
    dlg_layout->addWidget(buttons);
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
        file_panel_->select(active_document_index_, true);
    }
    file_panel_->set_opened_index(active_document_index_);
    switching_document_ = false;
}

void MainWindow::normalize_visible_signal_indices(LoadedDocument& document) const {
    for (auto& sheet : document.sheets) {
        normalize_visible_signal_indices(sheet);
    }
}

void MainWindow::normalize_visible_signal_indices(LoadedSheetState& sheet) const {
    std::vector<int> normalized;
    normalized.reserve(sheet.visible_signal_indices.size());
    for (int index : sheet.visible_signal_indices) {
        if (index < 0 || index >= static_cast<int>(sheet.library.size())) {
            continue;
        }
        if (std::find(normalized.begin(), normalized.end(), index) == normalized.end()) {
            normalized.push_back(index);
        }
    }
    sheet.visible_signal_indices = std::move(normalized);
}

std::vector<int> MainWindow::active_visible_signal_indices() const {
    const auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        return {};
    }
    return sheet->visible_signal_indices;
}

int MainWindow::active_signal_index() const {
    const auto* sheet = active_sheet_state();
    if (sheet == nullptr) {
        return -1;
    }
    return sheet->active_signal_index;
}

int MainWindow::active_sheet_index() const {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return -1;
    }
    return documents_[static_cast<std::size_t>(active_document_index_)].active_sheet_index;
}

MainWindow::LoadedSheetState* MainWindow::active_sheet_state() {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return nullptr;
    }
    auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (document.sheets.empty()) {
        return nullptr;
    }
    document.active_sheet_index = std::clamp(
        document.active_sheet_index, 0, static_cast<int>(document.sheets.size()) - 1);
    return &document.sheets[static_cast<std::size_t>(document.active_sheet_index)];
}

const MainWindow::LoadedSheetState* MainWindow::active_sheet_state() const {
    if (active_document_index_ < 0 ||
        active_document_index_ >= static_cast<int>(documents_.size())) {
        return nullptr;
    }
    const auto& document = documents_[static_cast<std::size_t>(active_document_index_)];
    if (document.sheets.empty()) {
        return nullptr;
    }
    const int bounded_sheet_index =
        std::clamp(document.active_sheet_index, 0, static_cast<int>(document.sheets.size()) - 1);
    return &document.sheets[static_cast<std::size_t>(bounded_sheet_index)];
}

void MainWindow::rebind_plot() {
    const int idx = active_signal_index();
    list_panel_->set_visible_signal_indices(active_visible_signal_indices());
    if (idx < 0 || idx >= static_cast<int>(service_.library().size())) {
        list_panel_->set_active_signal_index(-1);
        plot_->set_library(nullptr, -1, active_visible_signal_indices());
        table_panel_->set_library(&service_.library(), -1, active_visible_signal_indices());
    } else {
        list_panel_->set_active_signal_index(idx);
        plot_->set_library(&service_.library(), idx, active_visible_signal_indices());
        table_panel_->set_library(&service_.library(), idx, active_visible_signal_indices());
    }
    sync_plot_view_controls();
    update_interpolation_box();
    refresh_status();
}

void MainWindow::sync_plot_navigation_mode_controls() {
    if (plot_pan_mode_button_ == nullptr || plot_rect_mode_button_ == nullptr || plot_ == nullptr) {
        return;
    }

    const QSignalBlocker pan_blocker(plot_pan_mode_button_);
    const QSignalBlocker rect_blocker(plot_rect_mode_button_);
    plot_pan_mode_button_->setChecked(
        plot_->navigation_mode() == SignalPlotWidget::NavigationMode::Pan);
    plot_rect_mode_button_->setChecked(
        plot_->navigation_mode() == SignalPlotWidget::NavigationMode::RectZoom);
}

void MainWindow::sync_plot_view_controls() {
    const bool has_signal = plot_ != nullptr &&
                            active_signal_index() >= 0 &&
                            active_signal_index() < static_cast<int>(service_.library().size());
    const bool has_plot = plot_ != nullptr;
    if (plot_zoom_in_button_ != nullptr) {
        plot_zoom_in_button_->setEnabled(has_signal);
    }
    if (plot_zoom_out_button_ != nullptr) {
        plot_zoom_out_button_->setEnabled(has_signal);
    }
    if (plot_pan_mode_button_ != nullptr) {
        plot_pan_mode_button_->setEnabled(has_signal);
    }
    if (plot_rect_mode_button_ != nullptr) {
        plot_rect_mode_button_->setEnabled(has_signal);
    }
    if (plot_reset_view_button_ != nullptr) {
        plot_reset_view_button_->setEnabled(has_signal);
    }
    if (plot_t_start_edit_ != nullptr) {
        plot_t_start_edit_->setEnabled(has_signal);
    }
    if (plot_t_end_edit_ != nullptr) {
        plot_t_end_edit_->setEnabled(has_signal);
    }
    if (plot_export_button_ != nullptr) {
        plot_export_button_->setEnabled(has_plot);
    }
    if (act_export_plot_ != nullptr) {
        act_export_plot_->setEnabled(has_plot);
    }

    if (!has_signal) {
        if (plot_t_start_edit_ != nullptr) {
            const QSignalBlocker blocker(plot_t_start_edit_);
            plot_t_start_edit_->clear();
        }
        if (plot_t_end_edit_ != nullptr) {
            const QSignalBlocker blocker(plot_t_end_edit_);
            plot_t_end_edit_->clear();
        }
        if (plot_ != nullptr) {
            plot_->set_navigation_mode(SignalPlotWidget::NavigationMode::Edit);
        }
        sync_plot_navigation_mode_controls();
        return;
    }

    sync_plot_navigation_mode_controls();
    const auto [t_start, t_end] = plot_->time_view();
    onPlotTimeViewChanged(t_start, t_end);
}

void MainWindow::update_undo_action() {
    if (undo_action_ == nullptr) { return; }
    const bool enabled = active_document_index_ >= 0 &&
        active_document_index_ < static_cast<int>(documents_.size()) &&
        !documents_[static_cast<std::size_t>(active_document_index_)].undo_stack.empty();
    undo_action_->setEnabled(enabled);
}

void MainWindow::update_interpolation_box() {
    if (interpolation_box_ == nullptr) { return; }
    const auto plotted_indices = active_visible_signal_indices();
    if (plotted_indices.empty()) {
        interpolation_box_->blockSignals(true);
        interpolation_box_->setCurrentIndex(static_cast<int>(Signal::InterpolationMode::Linear));
        interpolation_box_->blockSignals(false);
        interpolation_box_->setEnabled(false);
        return;
    }

    Signal::InterpolationMode displayed_mode = Signal::InterpolationMode::Linear;
    bool found_reference_signal = false;
    bool all_enumerated = true;
    for (int signal_index : plotted_indices) {
        if (signal_index < 0 || signal_index >= static_cast<int>(service_.library().size())) {
            continue;
        }
        const auto& signal = service_.library().at(static_cast<std::size_t>(signal_index));
        if (!found_reference_signal) {
            displayed_mode = signal.interpolation();
            found_reference_signal = true;
        }
        if (!signal.is_enumerated()) {
            all_enumerated = false;
        }
    }

    interpolation_box_->blockSignals(true);
    interpolation_box_->setCurrentIndex(static_cast<int>(displayed_mode));
    interpolation_box_->blockSignals(false);
    interpolation_box_->setEnabled(!all_enumerated);
}

void MainWindow::refresh_status(const QString& transient_message) {
    if (!transient_message.isEmpty()) {
        statusBar()->showMessage(transient_message, 4000);
    }
    if (active_document_index_ < 0 || active_document_index_ >= static_cast<int>(documents_.size())) {
        if (workspace_title_label_) workspace_title_label_->setText(tr("Signal Editor Workspace"));
        if (workspace_meta_label_)  workspace_meta_label_->setText(tr("No active document"));
        if (workspace_hint_label_) {
            workspace_hint_label_->setText(
                tr("Import a signal file or create one, then edit it through plot and table."));
        }
        if (transient_message.isEmpty()) {
            statusBar()->showMessage(tr("No file loaded"));
        }
        return;
    }
    const auto& document   = documents_[static_cast<std::size_t>(active_document_index_)];
    const auto* sheet = active_sheet_state();
    const QString dirty_text = document.dirty ? tr("Modified") : tr("Synced");
    const QString undo_text  = tr("Undo %1").arg(static_cast<qulonglong>(document.undo_stack.size()));
    const QString summary    =
        tr("%1 file(s) loaded | %2 | %3 | %4")
            .arg(static_cast<qulonglong>(documents_.size()))
            .arg(document.display_name)
            .arg(summarize_counts(service_.library()))
            .arg(dirty_text);

    if (workspace_title_label_) workspace_title_label_->setText(document.display_name);
    if (workspace_meta_label_) {
        if (document.format == DocumentFormat::Xlsx && sheet != nullptr && !sheet->name.isEmpty()) {
            workspace_meta_label_->setText(QStringLiteral("%1 | %2 | %3 | %4")
                .arg(summarize_counts(service_.library()))
                .arg(sheet->name)
                .arg(dirty_text)
                .arg(undo_text));
        } else {
            workspace_meta_label_->setText(QStringLiteral("%1 | %2 | %3")
                .arg(summarize_counts(service_.library()))
                .arg(dirty_text)
                .arg(undo_text));
        }
    }
    const int signal_index   = active_signal_index();
    const Signal* active_signal = (signal_index >= 0 &&
                                   signal_index < static_cast<int>(service_.library().size()))
        ? &service_.library().at(static_cast<std::size_t>(signal_index))
        : nullptr;
    if (workspace_hint_label_) {
        if (active_signal && active_signal->is_enumerated()) {
            workspace_hint_label_->setText(
                tr("Enumerated signals snap to named states and render textual values on the Y axis."));
        } else {
            workspace_hint_label_->setText(
                tr("Drag points, use Shift+drag for Gaussian brushing, or refine samples in the table."));
        }
    }
    if (transient_message.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("%1 | %2").arg(summary).arg(undo_text));
    }
}

void MainWindow::show_error(const QString& title, const QString& message) {
    QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::Ok, this);
    apply_application_icon(&box);
    box.exec();
}

void MainWindow::retranslate_ui() {
    // Window
    setWindowTitle(tr("Signal Editor"));

    // Workspace header
    if (workspace_title_label_) workspace_title_label_->setText(tr("Signal Editor Workspace"));
    if (workspace_meta_label_)  workspace_meta_label_->setText(tr("No active document"));
    if (interp_label_)          interp_label_->setText(tr("Interpolation"));
    if (workspace_title_label_) {
        workspace_title_label_->setToolTip(tr("Overview of the active workspace and document state."));
    }
    if (workspace_meta_label_) {
        workspace_meta_label_->setToolTip(tr("Shows the active document name and whether it has unsaved changes."));
    }
    if (workspace_hint_label_) {
        workspace_hint_label_->setToolTip(tr("Context hint describing what you can do in the current workspace."));
    }
    if (interp_label_) {
        interp_label_->setToolTip(tr("Interpolation method used to render and evaluate the selected signal."));
    }
    if (plot_zoom_in_button_) {
        plot_zoom_in_button_->setToolTip(tr("Reduce the visible time range around the current center."));
        plot_zoom_in_button_->setStatusTip(tr("Zoom in on the plot time axis."));
    }
    if (plot_zoom_out_button_) {
        plot_zoom_out_button_->setToolTip(tr("Expand the visible time range around the current center."));
        plot_zoom_out_button_->setStatusTip(tr("Zoom out on the plot time axis."));
    }
    if (plot_pan_mode_button_) {
        plot_pan_mode_button_->setToolTip(tr("Drag the plot horizontally to move across the current time window."));
        plot_pan_mode_button_->setStatusTip(tr("Enable pan mode for the plot."));
    }
    if (plot_rect_mode_button_) {
        plot_rect_mode_button_->setToolTip(tr("Draw a selection rectangle to zoom into a specific time region."));
        plot_rect_mode_button_->setStatusTip(tr("Enable rectangle zoom mode for the plot."));
    }
    if (plot_reset_view_button_) {
        plot_reset_view_button_->setToolTip(tr("Restore the full visible range of the selected signal."));
        plot_reset_view_button_->setStatusTip(tr("Reset the plot to fit the selected signal."));
    }
    if (plot_export_button_) {
        plot_export_button_->setToolTip(tr("Save a screenshot of the current plot content as an image file."));
        plot_export_button_->setStatusTip(tr("Export the current plot as an image."));
    }
    if (plot_t_start_label_) {
        plot_t_start_label_->setText(tr("Visible t start"));
        plot_t_start_label_->setToolTip(tr("Start value of the currently visible time range."));
    }
    if (plot_t_end_label_) {
        plot_t_end_label_->setText(tr("Visible t end"));
        plot_t_end_label_->setToolTip(tr("End value of the currently visible time range."));
    }
    if (plot_t_start_edit_) {
        plot_t_start_edit_->setPlaceholderText(tr("t start"));
        plot_t_start_edit_->setToolTip(tr("Edit the visible start time for the plot and press Enter or leave the field to apply."));
        plot_t_start_edit_->setStatusTip(tr("Set the visible plot start time."));
    }
    if (plot_t_end_edit_) {
        plot_t_end_edit_->setPlaceholderText(tr("t end"));
        plot_t_end_edit_->setToolTip(tr("Edit the visible end time for the plot and press Enter or leave the field to apply."));
        plot_t_end_edit_->setStatusTip(tr("Set the visible plot end time."));
    }

    // Interpolation combo items (block signals to avoid spurious interp changes)
    if (interpolation_box_) {
        interpolation_box_->blockSignals(true);
        interpolation_box_->setItemText(0, tr("Linear"));
        interpolation_box_->setItemText(1, tr("Step"));
        interpolation_box_->blockSignals(false);
        interpolation_box_->setToolTip(tr("Choose how consecutive samples are connected in the plot and value evaluation."));
        interpolation_box_->setStatusTip(tr("Change the interpolation of the selected signal."));
    }

    // Tabs
    if (workspace_tabs_) {
        workspace_tabs_->setTabText(0, tr("Plot"));
        workspace_tabs_->setTabText(1, tr("Table"));
        workspace_tabs_->setToolTip(tr("Switch between graphical editing and table-based sample editing."));
        workspace_tabs_->setTabToolTip(0, tr("Interactive plot for navigation and direct sample editing."));
        workspace_tabs_->setTabToolTip(1, tr("Editable table of sample points for precise numeric changes."));
    }
    if (plot_ != nullptr) {
        plot_->setToolTip(tr("Interactive signal plot. Use the active navigation mode or drag samples directly in edit mode."));
        plot_->setStatusTip(tr("Inspect and edit the selected signal in the plot."));
    }

    // Menus
    if (menu_file_)     menu_file_->setTitle(tr("&File"));
    if (menu_signal_)   menu_signal_->setTitle(tr("&Signal"));
    if (menu_settings_) menu_settings_->setTitle(tr("&Settings"));
    if (menu_help_)     menu_help_->setTitle(tr("&Help"));

    // File actions
    if (act_open_)    act_open_->setText(tr("&Open signal files..."));
    if (act_save_)    act_save_->setText(tr("&Save current file..."));
    if (act_export_plot_) act_export_plot_->setText(tr("Export &plot image..."));
    if (undo_action_) undo_action_->setText(tr("&Undo"));
    if (act_quit_)    act_quit_->setText(tr("&Quit"));
    if (act_open_) {
        act_open_->setToolTip(tr("Import one or more supported signal files into the workspace."));
        act_open_->setStatusTip(tr("Open supported signal files."));
    }
    if (act_save_) {
        act_save_->setToolTip(tr("Save the active workspace file using its current format."));
        act_save_->setStatusTip(tr("Save the active signal file."));
    }
    if (act_export_plot_) {
        act_export_plot_->setToolTip(tr("Save the current plot content as an image file."));
        act_export_plot_->setStatusTip(tr("Export the current plot as an image."));
    }
    if (undo_action_) {
        undo_action_->setToolTip(tr("Revert the latest change applied to the active document."));
        undo_action_->setStatusTip(tr("Undo the latest change."));
    }
    if (act_quit_) {
        act_quit_->setToolTip(tr("Close the application."));
        act_quit_->setStatusTip(tr("Quit Signal Editor."));
    }

    // Signal actions
    if (act_new_)      act_new_->setText(tr("&New from scratch..."));
    if (rename_action_) rename_action_->setText(tr("Re&name selected"));
    if (act_remove_)   act_remove_->setText(tr("&Remove selected"));
    if (act_new_) {
        act_new_->setToolTip(tr("Create a new signal in the current workspace."));
        act_new_->setStatusTip(tr("Create a new signal from scratch."));
    }
    if (rename_action_) {
        rename_action_->setToolTip(tr("Rename the currently selected signal."));
        rename_action_->setStatusTip(tr("Rename the selected signal."));
    }
    if (act_remove_) {
        act_remove_->setToolTip(tr("Remove the currently selected signal from the active workspace."));
        act_remove_->setStatusTip(tr("Remove the selected signal."));
    }

    // Settings / Help actions
    if (settings_action_) {
        settings_action_->setText(tr("&Preferences..."));
        settings_action_->setToolTip(tr("Open application settings"));
        settings_action_->setStatusTip(tr("Open preferences and visual settings."));
    }
    if (act_manual_) {
        act_manual_->setText(tr("&User manual..."));
        act_manual_->setToolTip(tr("Open the user manual entry point."));
        act_manual_->setStatusTip(tr("Open the user manual placeholder."));
    }
    if (act_about_) {
        act_about_->setText(tr("&About"));
        act_about_->setToolTip(tr("Show application information and supported features."));
        act_about_->setStatusTip(tr("Open the About dialog."));
    }

    // Toolbar title
    if (main_toolbar_) {
        main_toolbar_->setWindowTitle(tr("Main"));
        main_toolbar_->setToolTip(tr("Primary application commands for file and signal management."));
    }

    // Status bar button
    if (settings_btn_) {
        settings_btn_->setToolTip(tr("Open application settings"));
        settings_btn_->setStatusTip(tr("Open preferences and visual settings."));
    }

    // Refresh dynamic status-bar and workspace meta labels
    refresh_status();
}

void MainWindow::register_svg_icon(QAction* action, const QString& resource_path) {
    if (action == nullptr || resource_path.isEmpty()) {
        return;
    }
    svg_action_icons_.push_back({action, resource_path});
}

void MainWindow::register_svg_icon(QAbstractButton* button, const QString& resource_path) {
    if (button == nullptr || resource_path.isEmpty()) {
        return;
    }
    svg_button_icons_.push_back({button, resource_path});
}

bool MainWindow::use_light_svg_icons() const {
    const AppTheme theme = theme_from_string(visual_settings_.theme);
    if (theme == AppTheme::Dark) {
        return true;
    }
    if (theme == AppTheme::Light) {
        return false;
    }

    const QColor window_color = qApp->palette().color(QPalette::Window);
    const double luminance =
        0.299 * window_color.redF() + 0.587 * window_color.greenF() + 0.114 * window_color.blueF();
    return luminance < 0.5;
}

void MainWindow::refresh_registered_svg_icons() {
    const SvgIconMode mode =
        use_light_svg_icons() ? SvgIconMode::LightMonochrome : SvgIconMode::Original;
    const QColor tint = visual_settings_.highContrastMode
        ? QColor(255, 255, 255)
        : QColor(245, 245, 245);

    for (const SvgActionIconBinding& binding : svg_action_icons_) {
        if (binding.action != nullptr) {
            binding.action->setIcon(load_svg_icon(binding.resource_path, mode, tint));
        }
    }

    for (const SvgButtonIconBinding& binding : svg_button_icons_) {
        if (binding.button != nullptr) {
            binding.button->setIcon(load_svg_icon(binding.resource_path, mode, tint));
        }
    }
}

void MainWindow::load_persisted_settings() {
    lib_qt_custom_widgets::AppSettings settings = app_settings_;

#ifdef LIB_QT_UTILS_AVAILABLE
    if (app_state_) {
        const QByteArray geometry = app_state_->windowGeometry(QStringLiteral("main_window"));
        if (!geometry.isEmpty()) {
            restoreGeometry(geometry);
        }
        const QByteArray state = app_state_->windowState(QStringLiteral("main_window"));
        if (!state.isEmpty()) {
            restoreState(state);
        }

        settings.theme      = app_state_->value(QString::fromUtf8(ui::kSettingsKeyTheme),       settings.theme).toString();
        settings.language   = app_state_->value(QString::fromUtf8(ui::kSettingsKeyLanguage),    settings.language).toString();
        settings.fontFamily = app_state_->value(QString::fromUtf8(ui::kSettingsKeyFontFamily),  settings.fontFamily).toString();
        settings.fontSize   = app_state_->value(QString::fromUtf8(ui::kSettingsKeyFontSize),    settings.fontSize).toInt();
        settings.highContrastMode = app_state_->value(
            QString::fromUtf8(ui::kSettingsKeyHighContrast), settings.highContrastMode).toBool();
        settings.widgetDensity = app_state_->value(
            QString::fromUtf8(ui::kSettingsKeyWidgetDensity), settings.widgetDensity).toString();
        settings.animationDurationMs = app_state_->value(
            QString::fromUtf8(ui::kSettingsKeyAnimationDuration), settings.animationDurationMs).toInt();
        const QString color_str = app_state_->value(
            QString::fromUtf8(ui::kSettingsKeyPrimaryColor), settings.primaryColor.name()).toString();
        if (QColor::isValidColorName(color_str)) {
            settings.primaryColor = QColor(color_str);
        }
    }
#endif

    apply_visual_settings(settings, true, true);
    apply_behavior_settings(settings);
    apply_language(settings.language);
    app_settings_ = settings;
}

void MainWindow::persist_settings() {
#ifdef LIB_QT_UTILS_AVAILABLE
    if (app_state_) {
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyTheme),          app_settings_.theme);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyLanguage),       app_settings_.language);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyFontFamily),     app_settings_.fontFamily);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyFontSize),       app_settings_.fontSize);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyHighContrast),   app_settings_.highContrastMode);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyWidgetDensity),  app_settings_.widgetDensity);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyAnimationDuration),
                             app_settings_.animationDurationMs);
        app_state_->setValue(QString::fromUtf8(ui::kSettingsKeyPrimaryColor),   app_settings_.primaryColor.name());
        app_state_->sync();
    }
#endif
}

void MainWindow::apply_visual_settings(const lib_qt_custom_widgets::AppSettings& settings,
                                       bool include_density,
                                       bool force_apply) {
    const bool style_changed =
        settings.theme != visual_settings_.theme ||
        settings.primaryColor != visual_settings_.primaryColor ||
        settings.highContrastMode != visual_settings_.highContrastMode ||
        settings.fontFamily != visual_settings_.fontFamily ||
        settings.fontSize != visual_settings_.fontSize;

    if (force_apply || style_changed) {
        QApplication::setFont(QFont(settings.fontFamily, settings.fontSize));
        apply_theme(*qApp, theme_from_string(settings.theme), settings.primaryColor);
        qApp->setStyleSheet(qApp->styleSheet() +
                            build_font_qss(settings) +
                            build_accessibility_qss(settings));
    }

    if (include_density &&
        (force_apply || settings.widgetDensity != visual_settings_.widgetDensity)) {
        apply_density(settings.widgetDensity);
    }

    visual_settings_ = settings;
    if (force_apply || style_changed) {
        refresh_registered_svg_icons();
    }
}

void MainWindow::apply_density(const QString& density) {
    int outer_margin = 16;
    int spacing = 10;

    if (density == QStringLiteral("compact")) {
        outer_margin = 10;
        spacing = 6;
    } else if (density == QStringLiteral("spacious")) {
        outer_margin = 22;
        spacing = 14;
    }

    if (QWidget* central = centralWidget(); central != nullptr) {
        if (QLayout* root_layout = central->layout(); root_layout != nullptr) {
            root_layout->setContentsMargins(outer_margin, outer_margin - 2,
                                            outer_margin, outer_margin - 2);
            root_layout->setSpacing(spacing);
        }

        const auto layouts = central->findChildren<QLayout*>();
        for (QLayout* layout : layouts) {
            if (layout == nullptr || layout == central->layout()) {
                continue;
            }
            layout->setSpacing(spacing);
        }
    }
}

void MainWindow::apply_language(const QString& language) {
    if (active_translator_) {
        QCoreApplication::removeTranslator(active_translator_);
        delete active_translator_;
        active_translator_ = nullptr;
    }

    const QString lang = normalize_language_code(language);
    if (lang != QStringLiteral("en")) {
        const QString translation_base = ui::translation_base_name() + lang;
        const QString app_dir = QCoreApplication::applicationDirPath();
        const QStringList candidates =
            translation_candidates(app_dir, translation_base);

        for (const QString& candidate : candidates) {
            auto* translator = new QTranslator(this);
            if (translator->load(candidate) && translator_has_app_ui_strings(*translator)) {
                active_translator_ = translator;
                break;
            }
            delete translator;
        }

        if (active_translator_ == nullptr) {
            const QString resource_translation =
                QStringLiteral(":/i18n/%1.qm").arg(translation_base);
            auto* translator = new QTranslator(this);
            if (translator->load(resource_translation) &&
                translator_has_app_ui_strings(*translator)) {
                active_translator_ = translator;
            } else {
                delete translator;
            }
        }

        if (active_translator_ != nullptr) {
            QCoreApplication::installTranslator(active_translator_);
        }
    }

    retranslate_ui();
    QEvent language_change_event(QEvent::LanguageChange);
    const auto objects = findChildren<QObject*>();
    for (QObject* object : objects) {
        if (object != nullptr) {
            QCoreApplication::sendEvent(object, &language_change_event);
        }
    }
}

void MainWindow::apply_behavior_settings(const lib_qt_custom_widgets::AppSettings& settings) {
    qApp->setProperty("signalEditor.animationDurationMs", settings.animationDurationMs);
    set_ui_effects_enabled(settings.animationDurationMs > 0);
}

#undef tr

}  // namespace signal_editor::adapters::qt
