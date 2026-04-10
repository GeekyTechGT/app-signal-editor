#include "signal_editor/adapters/qt/theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace myprj::signal_editor::adapters::qt {

void apply_dark_fusion_theme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window,         QColor( 30,  32,  44));
    p.setColor(QPalette::WindowText,     QColor(220, 224, 235));
    p.setColor(QPalette::Base,           QColor( 22,  24,  34));
    p.setColor(QPalette::AlternateBase,  QColor( 28,  30,  42));
    p.setColor(QPalette::ToolTipBase,    QColor(255, 240, 200));
    p.setColor(QPalette::ToolTipText,    QColor( 20,  20,  20));
    p.setColor(QPalette::Text,           QColor(220, 224, 235));
    p.setColor(QPalette::Button,         QColor( 40,  44,  60));
    p.setColor(QPalette::ButtonText,     QColor(220, 224, 235));
    p.setColor(QPalette::BrightText,     ::Qt::red);
    p.setColor(QPalette::Link,           QColor( 90, 200, 250));
    p.setColor(QPalette::Highlight,      QColor( 90, 200, 250));
    p.setColor(QPalette::HighlightedText,QColor( 10,  15,  25));
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(120, 124, 140));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 124, 140));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 124, 140));
    app.setPalette(p);

    app.setStyleSheet(QStringLiteral(R"qss(
        QMainWindow      { background-color: #1e202c; }
        QLabel#PanelTitle{ color: #d2d7e6; padding: 2px 0 6px 2px; letter-spacing: 0.5px; }
        QListWidget {
            background: #161824;
            border: 1px solid #2a2d3e;
            border-radius: 6px;
            padding: 4px;
            outline: 0;
        }
        QListWidget::item        { padding: 6px 8px; border-radius: 4px; }
        QListWidget::item:selected{
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #5ac8fa, stop:1 #2a8cff);
            color: #0a0f19;
        }
        QPushButton {
            background: #2a2e42;
            color: #e0e4f0;
            border: 1px solid #3a3f55;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 600;
        }
        QPushButton:hover    { background: #353a55; }
        QPushButton:pressed  { background: #5ac8fa; color: #0a0f19; }
        QToolBar {
            background: #1a1d2a;
            border: none;
            spacing: 6px;
            padding: 6px;
        }
        QToolButton {
            color: #d2d7e6;
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 4px 10px;
        }
        QToolButton:hover    { background: #2a2d3e; border-color: #3a3f55; }
        QStatusBar           { background: #1a1d2a; color: #b0b6c8; }
        QMenuBar             { background: #1a1d2a; color: #d2d7e6; }
        QMenuBar::item:selected { background: #2a2d3e; }
        QMenu                { background: #20232f; color: #d2d7e6; border: 1px solid #2a2d3e; }
        QMenu::item:selected { background: #5ac8fa; color: #0a0f19; }
        QSplitter::handle    { background: #2a2d3e; }
    )qss"));
}

}  // namespace myprj::signal_editor::adapters::qt
