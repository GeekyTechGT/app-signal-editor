#include "signal_editor/adapters/qt/theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace myprj::signal_editor::adapters::qt {

void apply_dark_fusion_theme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(18, 24, 32));
    palette.setColor(QPalette::WindowText, QColor(229, 235, 241));
    palette.setColor(QPalette::Base, QColor(11, 17, 24));
    palette.setColor(QPalette::AlternateBase, QColor(20, 28, 38));
    palette.setColor(QPalette::ToolTipBase, QColor(245, 238, 225));
    palette.setColor(QPalette::ToolTipText, QColor(18, 22, 28));
    palette.setColor(QPalette::Text, QColor(229, 235, 241));
    palette.setColor(QPalette::Button, QColor(24, 32, 44));
    palette.setColor(QPalette::ButtonText, QColor(229, 235, 241));
    palette.setColor(QPalette::BrightText, QColor(255, 111, 97));
    palette.setColor(QPalette::Link, QColor(93, 211, 196));
    palette.setColor(QPalette::Highlight, QColor(244, 166, 74));
    palette.setColor(QPalette::HighlightedText, QColor(18, 24, 32));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(108, 120, 135));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(108, 120, 135));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(108, 120, 135));
    app.setPalette(palette);

    app.setStyleSheet(QStringLiteral(R"qss(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0b1118, stop:0.4 #111925, stop:1 #18202b);
        }
        QWidget {
            color: #e5ebf1;
            selection-background-color: #f4a64a;
            selection-color: #121820;
        }
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
            color: #f4a64a;
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
        QLabel#PanelTitle {
            color: #f4f7fb;
            padding: 0 0 2px 0;
            letter-spacing: 0.4px;
        }
        QLabel#PanelSummary {
            color: #f4a64a;
            font-size: 12px;
            font-weight: 600;
        }
        QLabel#PanelDetail {
            color: #94a6b8;
            font-size: 12px;
            line-height: 1.3em;
        }
        QListWidget, QTableWidget, QTextEdit {
            background: #091017;
            border: 1px solid rgba(108, 132, 156, 0.22);
            border-radius: 12px;
            padding: 6px;
            gridline-color: rgba(92, 110, 130, 0.35);
            outline: 0;
        }
        QListWidget::item {
            padding: 8px 10px;
            margin: 2px 0;
            border-radius: 8px;
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #f4a64a, stop:1 #ffd089);
            color: #101720;
        }
        QTableWidget#SignalSamplesTable {
            alternate-background-color: #111922;
            selection-background-color: #f4a64a;
            selection-color: #101720;
        }
        QTableWidget#SignalSamplesTable::item {
            padding: 10px 8px;
            border-bottom: 1px solid rgba(108, 132, 156, 0.10);
        }
        QTableWidget#SignalSamplesTable::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ffd089, stop:1 #f4a64a);
            color: #101720;
        }
        QTableWidget#SignalSamplesTable QTableCornerButton::section {
            background: #17202b;
            border: 0;
            border-bottom: 1px solid rgba(118, 142, 166, 0.18);
        }
        QHeaderView::section {
            background: #17202b;
            color: #dfe6ee;
            border: 0;
            border-bottom: 1px solid rgba(118, 142, 166, 0.18);
            padding: 8px 10px;
            font-weight: 600;
        }
        QPushButton, QComboBox, QLineEdit, QDoubleSpinBox, QSpinBox {
            background: #1b2430;
            color: #edf2f7;
            border: 1px solid rgba(116, 139, 162, 0.28);
            border-radius: 10px;
            padding: 7px 12px;
        }
        QPushButton {
            font-weight: 600;
        }
        QPushButton:hover, QComboBox:hover, QLineEdit:hover, QDoubleSpinBox:hover, QSpinBox:hover {
            border-color: rgba(244, 166, 74, 0.60);
            background: rgba(34, 45, 60, 0.98);
        }
        QPushButton:pressed {
            background: #f4a64a;
            color: #121820;
        }
        QPushButton#AccentButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ffd089, stop:1 #f4a64a);
            color: #101720;
            border-color: rgba(255, 214, 153, 0.85);
        }
        QPushButton#AccentButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ffe0a7, stop:1 #ffb25c);
        }
        QPushButton#SubtleButton {
            background: #16202b;
        }
        QToolBar {
            background: #0c1219;
            border: 1px solid rgba(102, 128, 153, 0.16);
            border-radius: 14px;
            spacing: 8px;
            padding: 8px;
        }
        QToolButton {
            color: #dce4ed;
            background: transparent;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 6px 10px;
        }
        QToolButton:hover {
            background: rgba(34, 45, 60, 0.98);
            border-color: rgba(244, 166, 74, 0.40);
        }
        QStatusBar {
            background: #0a0f16;
            color: #a8b7c7;
        }
        QMenuBar {
            background: #0c1219;
            color: #dfe6ee;
        }
        QMenuBar::item:selected {
            background: rgba(36, 48, 64, 0.98);
        }
        QMenu {
            background: #141c26;
            color: #e5ebf1;
            border: 1px solid rgba(108, 132, 156, 0.25);
        }
        QMenu::item:selected {
            background: #f4a64a;
            color: #121820;
        }
        QTabWidget#WorkspaceTabs::pane {
            background: #0c121a;
            border: 1px solid rgba(118, 142, 166, 0.18);
            border-radius: 14px;
            margin-top: 6px;
            padding: 8px;
        }
        QTabWidget#WorkspaceTabs QTabBar::tab {
            background: #131b26;
            color: #c6d2de;
            border: 1px solid rgba(118, 142, 166, 0.18);
            border-bottom: 0;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            padding: 8px 18px;
            margin-right: 6px;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0.25px;
        }
        QTabWidget#WorkspaceTabs QTabBar::tab:selected {
            color: #101720;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ffd089, stop:0.55 #f4a64a, stop:1 #ff8a5b);
            border-color: rgba(255, 214, 153, 0.90);
            margin-top: -2px;
            padding-top: 10px;
            padding-bottom: 10px;
        }
        QTabWidget#WorkspaceTabs QTabBar::tab:hover:!selected {
            background: rgba(34, 45, 60, 0.98);
            color: #f3f7fb;
            border-color: rgba(244, 166, 74, 0.45);
        }
        QWidget#WorkspaceTabPage {
            background: #0c121a;
            border-radius: 10px;
        }
        QWidget#WorkspaceTabPage > QWidget#PanelCard {
            border: 1px solid rgba(244, 166, 74, 0.10);
        }
        QSplitter::handle {
            background: rgba(64, 82, 100, 0.55);
            margin: 2px;
            border-radius: 4px;
        }
        QSplitter::handle:hover {
            background: rgba(244, 166, 74, 0.72);
        }
    )qss"));
}

}  // namespace myprj::signal_editor::adapters::qt
