#define _USE_MATH_DEFINES
#include "gui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("RPLidar C1 Research Visualizer");
    app.setApplicationVersion("2.0");
    app.setStyle(QStyleFactory::create("Fusion"));

    // Deep research-grade dark palette
    QPalette p;
    p.setColor(QPalette::Window,          QColor(18, 22, 30));
    p.setColor(QPalette::WindowText,      QColor(200, 215, 235));
    p.setColor(QPalette::Base,            QColor(12, 16, 24));
    p.setColor(QPalette::AlternateBase,   QColor(22, 28, 40));
    p.setColor(QPalette::ToolTipBase,     QColor(30, 40, 60));
    p.setColor(QPalette::ToolTipText,     QColor(200, 215, 235));
    p.setColor(QPalette::Text,            QColor(200, 215, 235));
    p.setColor(QPalette::Button,          QColor(24, 32, 48));
    p.setColor(QPalette::ButtonText,      QColor(200, 215, 235));
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       QColor(29, 100, 180));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link,            QColor(80, 160, 220));
    p.setColor(QPalette::Mid,             QColor(30, 40, 56));
    p.setColor(QPalette::Dark,            QColor(10, 14, 20));
    p.setColor(QPalette::Shadow,          QColor(5, 8, 12));
    app.setPalette(p);

    // Global stylesheet tweaks
    app.setStyleSheet(R"(
        QGroupBox {
            border: 1px solid #1e2d42;
            border-radius: 4px;
            margin-top: 6px;
            font-size: 10px;
            font-weight: bold;
            color: #4a7aaa;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px;
        }
        QPushButton {
            border: 1px solid #1e2d42;
            border-radius: 4px;
            padding: 4px 8px;
            background: #0d1520;
            color: #a0b8d0;
        }
        QPushButton:hover  { background: #162030; border-color: #2a4060; }
        QPushButton:pressed{ background: #0a0f18; }
        QPushButton:checked{ background: #0d2040; border-color: #1d64b0; color: #5aadee; }
        QSlider::groove:horizontal {
            height: 3px;
            background: #1e2d42;
            border-radius: 1px;
        }
        QSlider::handle:horizontal {
            width: 12px; height: 12px;
            margin: -5px 0;
            border-radius: 6px;
            background: #2a6ab0;
        }
        QSlider::sub-page:horizontal { background: #1d64b0; border-radius: 1px; }
        QComboBox {
            border: 1px solid #1e2d42;
            border-radius: 4px;
            padding: 3px 6px;
            background: #0d1520;
            color: #a0b8d0;
        }
        QSpinBox {
            border: 1px solid #1e2d42;
            border-radius: 4px;
            padding: 2px 4px;
            background: #0d1520;
            color: #a0b8d0;
        }
        QCheckBox { color: #8aaccc; spacing: 5px; }
        QCheckBox::indicator {
            width: 13px; height: 13px;
            border: 1px solid #2a4060;
            border-radius: 3px;
            background: #0d1520;
        }
        QCheckBox::indicator:checked { background: #1d64b0; border-color: #2a80d0; }
        QStatusBar { color: #4a7aaa; font-size: 11px; }
        QScrollBar:vertical { width: 6px; background: #0d1520; }
        QScrollBar::handle:vertical { background: #1e2d42; border-radius: 3px; }
    )");

    MainWindow w;
    w.show();
    return app.exec();
}
