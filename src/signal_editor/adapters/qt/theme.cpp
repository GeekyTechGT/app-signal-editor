#include "signal_editor/adapters/qt/theme.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

namespace signal_editor::adapters::qt {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

QString load_qss(const QString& resource_path) {
    QFile file(resource_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

// Returns a high-contrast text color for text rendered on top of `bg`.
QColor contrast_text(const QColor& bg) {
    const double lum = 0.299 * bg.redF() + 0.587 * bg.greenF() + 0.114 * bg.blueF();
    return lum > 0.5 ? QColor(18, 18, 18) : QColor(245, 245, 245);
}

// App-specific widget-ID overrides (layered on top of the base QSS).
// `accent` is the user-selected primary color; `on_accent` is the contrasting
// text color computed for readability on top of the accent background.
QString app_overrides_dark(const QColor& accent) {
    const QString a  = accent.name();
    const QString at = contrast_text(accent).name();
    QString qss = QStringLiteral(R"qss(
QWidget#WorkspaceHeader {
    background: #101821;
    border: 1px solid rgba(102, 128, 153, 0.20);
    border-radius: 16px;
}
QLabel#WorkspaceTitle {
    color: #f6f8fb;
    font-size: 20px;
    font-weight: 700;
    letter-spacing: 0.3px;
}
QLabel#WorkspaceMeta {
    color: #ACCENT#;
    font-size: 11px;
    font-weight: 600;
}
QLabel#WorkspaceHint {
    color: #a8b7c7;
    font-size: 11px;
}
QWidget#PanelCard {
    background: #111923;
    border: 1px solid rgba(118, 142, 166, 0.18);
    border-radius: 16px;
}
QWidget#PlotControlCard {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 rgba(22, 32, 45, 0.96),
                                stop:1 rgba(14, 22, 31, 0.92));
    border: 1px solid rgba(118, 142, 166, 0.22);
    border-radius: 14px;
}
QLabel#PanelTitle {
    color: #f4f7fb;
    padding: 0 0 2px 0;
    letter-spacing: 0.4px;
}
QLabel#PanelSummary {
    color: #ACCENT#;
    font-size: 12px;
    font-weight: 600;
}
QLabel#PanelDetail {
    color: #94a6b8;
    font-size: 12px;
}
QTableWidget#SignalSamplesTable {
    alternate-background-color: #111922;
    selection-background-color: #ACCENT#;
    selection-color: #ACCENT_TEXT#;
}
QTableWidget#SignalSamplesTable::item {
    padding: 10px 8px;
    border-bottom: 1px solid rgba(108, 132, 156, 0.10);
}
QTableWidget#SignalSamplesTable::item:selected {
    background-color: #ACCENT#;
    color: #ACCENT_TEXT#;
}
QTableWidget#SignalSamplesTable QLineEdit,
QTableWidget#SignalSamplesTable QComboBox {
    background: #1E1E1E;
    color: #F5F5F5;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    padding: 2px 8px;
    margin: 0;
}
QTableWidget#SignalSamplesTable QLineEdit:focus,
QTableWidget#SignalSamplesTable QComboBox:focus {
    border-color: #ACCENT#;
}
QTableWidget#SignalSamplesTable QComboBox::drop-down {
    border: 0;
    width: 20px;
    background: transparent;
}
QTableWidget#SignalSamplesTable QComboBox QAbstractItemView {
    background: #2A2A2A;
    color: #F5F5F5;
    selection-background-color: #ACCENT#;
    selection-color: #ACCENT_TEXT#;
    border: 1px solid #ACCENT#;
    outline: 0;
    padding: 4px;
}
QTabWidget#WorkspaceTabs::pane {
    background: #1E1E1E;
    border: 1px solid #3A3A3A;
    border-radius: 8px;
    top: -1px;
}
QTabWidget#WorkspaceTabs QTabBar::tab {
    background: #2A2A2A;
    color: #A0A0A0;
    border: 1px solid #3A3A3A;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    padding: 6px 16px;
    margin-right: 4px;
    font-weight: 700;
}
QTabWidget#WorkspaceTabs QTabBar::tab:selected {
    background: #1E1E1E;
    color: #ACCENT#;
    border-bottom: 1px solid #1E1E1E;
}
QTabWidget#WorkspaceTabs QTabBar::tab:hover:!selected {
    background: #333333;
    color: #ACCENT#;
}
QWidget#WorkspaceTabPage {
    background: #1E1E1E;
    border-radius: 6px;
}
QLineEdit#PlotRangeEdit {
    background: rgba(8, 14, 20, 0.82);
    color: #f4f7fb;
    border: 1px solid rgba(118, 142, 166, 0.26);
    border-radius: 10px;
    padding: 8px 12px;
    min-width: 110px;
}
QLineEdit#PlotRangeEdit:focus {
    border-color: #ACCENT#;
}
    )qss");
    qss.replace(QStringLiteral("#ACCENT_TEXT#"), at);
    qss.replace(QStringLiteral("#ACCENT#"), a);
    return qss;
}

QString app_overrides_light(const QColor& accent) {
    const QString a  = accent.name();
    const QString at = contrast_text(accent).name();
    QString qss = QStringLiteral(R"qss(
QWidget#WorkspaceHeader {
    background: #EEF2F8;
    border: 1px solid rgba(100, 130, 170, 0.22);
    border-radius: 16px;
}
QLabel#WorkspaceTitle {
    color: #1A2236;
    font-size: 20px;
    font-weight: 700;
    letter-spacing: 0.3px;
}
QLabel#WorkspaceMeta {
    color: #ACCENT#;
    font-size: 11px;
    font-weight: 600;
}
QLabel#WorkspaceHint {
    color: #6B7280;
    font-size: 11px;
}
QWidget#PanelCard {
    background: #FFFFFF;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 16px;
}
QWidget#PlotControlCard {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                stop:0 rgba(255, 255, 255, 0.98),
                                stop:1 rgba(239, 245, 251, 0.96));
    border: 1px solid rgba(100, 130, 170, 0.18);
    border-radius: 14px;
}
QLabel#PanelTitle {
    color: #1A2236;
    padding: 0 0 2px 0;
    letter-spacing: 0.4px;
}
QLabel#PanelSummary {
    color: #ACCENT#;
    font-size: 12px;
    font-weight: 600;
}
QLabel#PanelDetail {
    color: #6B7280;
    font-size: 12px;
}
QTableWidget#SignalSamplesTable {
    alternate-background-color: #F8FAFC;
    selection-background-color: #ACCENT#;
    selection-color: #ACCENT_TEXT#;
}
QTableWidget#SignalSamplesTable::item:selected {
    background-color: #ACCENT#;
    color: #ACCENT_TEXT#;
}
QTabWidget#WorkspaceTabs QTabBar::tab:selected {
    color: #ACCENT#;
    font-weight: 700;
}
QLineEdit#PlotRangeEdit {
    background: rgba(255, 255, 255, 0.94);
    color: #1A2236;
    border: 1px solid rgba(100, 130, 170, 0.22);
    border-radius: 10px;
    padding: 8px 12px;
    min-width: 110px;
}
QLineEdit#PlotRangeEdit:focus {
    border-color: #ACCENT#;
}
    )qss");
    qss.replace(QStringLiteral("#ACCENT_TEXT#"), at);
    qss.replace(QStringLiteral("#ACCENT#"), a);
    return qss;
}

QPalette make_dark_palette(const QColor& accent) {
    const QColor on_accent = contrast_text(accent);
    QPalette p;
    p.setColor(QPalette::Window,          QColor(18, 24, 32));
    p.setColor(QPalette::WindowText,      QColor(245, 245, 245));
    p.setColor(QPalette::Base,            QColor(30, 30, 30));
    p.setColor(QPalette::AlternateBase,   QColor(42, 42, 42));
    p.setColor(QPalette::ToolTipBase,     QColor(42, 42, 42));
    p.setColor(QPalette::ToolTipText,     QColor(245, 245, 245));
    p.setColor(QPalette::Text,            QColor(245, 245, 245));
    p.setColor(QPalette::Button,          QColor(30, 30, 30));
    p.setColor(QPalette::ButtonText,      QColor(245, 245, 245));
    p.setColor(QPalette::BrightText,      QColor(255, 100, 100));
    p.setColor(QPalette::Link,            accent);
    p.setColor(QPalette::Highlight,       accent);
    p.setColor(QPalette::HighlightedText, on_accent);
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(90, 90, 90));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(90, 90, 90));
    return p;
}

QPalette make_light_palette(const QColor& accent) {
    const QColor on_accent = contrast_text(accent);
    QPalette p;
    p.setColor(QPalette::Window,          QColor(240, 244, 250));
    p.setColor(QPalette::WindowText,      QColor(18, 18, 18));
    p.setColor(QPalette::Base,            QColor(255, 255, 255));
    p.setColor(QPalette::AlternateBase,   QColor(248, 250, 252));
    p.setColor(QPalette::ToolTipBase,     QColor(255, 255, 255));
    p.setColor(QPalette::ToolTipText,     QColor(18, 18, 18));
    p.setColor(QPalette::Text,            QColor(18, 18, 18));
    p.setColor(QPalette::Button,          QColor(240, 244, 250));
    p.setColor(QPalette::ButtonText,      QColor(18, 18, 18));
    p.setColor(QPalette::BrightText,      QColor(220, 50, 50));
    p.setColor(QPalette::Link,            accent);
    p.setColor(QPalette::Highlight,       accent);
    p.setColor(QPalette::HighlightedText, on_accent);
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(160, 160, 160));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
    return p;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AppTheme theme_from_string(const QString& name) {
    if (name == QStringLiteral("dark"))  { return AppTheme::Dark;  }
    if (name == QStringLiteral("light")) { return AppTheme::Light; }
    return AppTheme::System;
}

void apply_theme(QApplication& app, AppTheme theme, const QColor& accent) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    switch (theme) {
    case AppTheme::Dark: {
        app.setPalette(make_dark_palette(accent));
        QString qss = load_qss(QStringLiteral(":/themes/dark.qss"));
        qss += app_overrides_dark(accent);
        app.setStyleSheet(qss);
        break;
    }
    case AppTheme::Light: {
        app.setPalette(make_light_palette(accent));
        QString qss = load_qss(QStringLiteral(":/themes/light.qss"));
        qss += app_overrides_light(accent);
        app.setStyleSheet(qss);
        break;
    }
    case AppTheme::System:
        app.setPalette(app.style()->standardPalette());
        app.setStyleSheet({});
        break;
    }
}

}  // namespace signal_editor::adapters::qt
