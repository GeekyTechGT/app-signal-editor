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
            background: rgba(16, 24, 33, 0.92);
            border: 1px solid rgba(102, 128, 153, 0.20);
            border-radius: 18px;
        }
        QLabel#WorkspaceTitle {
            color: #f6f8fb;
            font-size: 24px;
            font-weight: 700;
            letter-spacing: 0.3px;
        }
        QLabel#WorkspaceMeta {
            color: #f4a64a;
            font-size: 12px;
            font-weight: 600;
        }
        QLabel#WorkspaceHint {
            color: #a8b7c7;
            font-size: 12px;
        }
        QWidget#PanelCard {
            background: rgba(17, 25, 35, 0.92);
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
            background: rgba(9, 15, 22, 0.96);
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
        QHeaderView::section {
            background: rgba(23, 31, 43, 0.98);
            color: #dfe6ee;
            border: 0;
            border-bottom: 1px solid rgba(118, 142, 166, 0.18);
            padding: 8px 10px;
            font-weight: 600;
        }
        QPushButton, QComboBox, QLineEdit, QDoubleSpinBox, QSpinBox {
            background: rgba(27, 36, 48, 0.94);
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
        QToolBar {
            background: rgba(12, 18, 25, 0.82);
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
            background: rgba(10, 15, 22, 0.92);
            color: #a8b7c7;
        }
        QMenuBar {
            background: rgba(12, 18, 25, 0.82);
            color: #dfe6ee;
        }
        QMenuBar::item:selected {
            background: rgba(36, 48, 64, 0.98);
        }
        QMenu {
            background: rgba(20, 28, 38, 0.98);
            color: #e5ebf1;
            border: 1px solid rgba(108, 132, 156, 0.25);
        }
        QMenu::item:selected {
            background: #f4a64a;
            color: #121820;
        }
        QSplitter::handle {
            background: rgba(64, 82, 100, 0.55);
            margin: 4px;
            border-radius: 2px;
        }
    )qss"));
}

}  // namespace myprj::signal_editor::adapters::qt
