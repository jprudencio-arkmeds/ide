#include <QApplication>
#include <QDir>
#include "ide/MainWindow.h"

static const char* STYLESHEET = R"(
* { outline: none; }

QWidget {
    background-color: #0F0529;
    color: #e8d5ff;
    selection-background-color: #7338A0;
    selection-color: #ffffff;
    border: none;
}

QMenuBar {
    background-color: #1E0B42;
    color: #c9a8f5;
    padding: 2px 4px;
    border-bottom: 1px solid #4A2574;
}
QMenuBar::item { padding: 5px 14px; border-radius: 6px; }
QMenuBar::item:selected { background-color: #4A2574; color: #ffffff; }

QMenu {
    background-color: #1E0B42;
    color: #e8d5ff;
    border: 1px solid #4A2574;
    border-radius: 8px;
    padding: 4px;
}
QMenu::item { padding: 6px 28px; border-radius: 5px; }
QMenu::item:selected { background-color: #4A2574; }
QMenu::separator { height: 1px; background: #4A2574; margin: 4px 8px; }

QSplitter::handle:vertical   { background-color: #4A2574; height: 2px; }
QSplitter::handle:horizontal { background-color: #4A2574; width:  2px; }

QStatusBar {
    background-color: #1E0B42;
    color: #9E72C3;
    border-top: 1px solid #4A2574;
    padding: 2px 8px;
}

QScrollBar:vertical {
    background: #0F0529; width: 8px; margin: 0; border-radius: 4px;
}
QScrollBar::handle:vertical {
    background: #4A2574; border-radius: 4px; min-height: 20px;
}
QScrollBar::handle:vertical:hover { background: #7338A0; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }

QScrollBar:horizontal {
    background: #0F0529; height: 8px; margin: 0; border-radius: 4px;
}
QScrollBar::handle:horizontal {
    background: #4A2574; border-radius: 4px; min-width: 20px;
}
QScrollBar::handle:horizontal:hover { background: #7338A0; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }

QDialog, QMessageBox { background-color: #1E0B42; border-radius: 10px; }
QMessageBox QLabel   { color: #e8d5ff; }
QMessageBox QPushButton, QInputDialog QPushButton {
    background-color: #7338A0; color: #ffffff;
    border-radius: 6px; padding: 6px 20px; min-width: 64px;
}
QMessageBox QPushButton:hover, QInputDialog QPushButton:hover {
    background-color: #924DBF;
}

QInputDialog QLineEdit {
    background-color: #0F0529; color: #e8d5ff;
    border: 1px solid #4A2574; border-radius: 6px; padding: 4px 8px;
}

QFileDialog QWidget { background-color: #1E0B42; color: #e8d5ff; }
QFileDialog QListView, QFileDialog QTreeView {
    background-color: #0F0529; color: #e8d5ff;
    border: 1px solid #4A2574; border-radius: 6px;
    selection-background-color: #7338A0;
}
QFileDialog QLineEdit {
    background-color: #0F0529; color: #e8d5ff;
    border: 1px solid #4A2574; border-radius: 6px; padding: 4px 8px;
}
QFileDialog QPushButton {
    background-color: #7338A0; color: #ffffff;
    border-radius: 6px; padding: 5px 16px;
}
QFileDialog QPushButton:hover { background-color: #924DBF; }

QComboBox {
    background-color: #1E0B42; color: #e8d5ff;
    border: 1px solid #4A2574; border-radius: 6px; padding: 4px 10px;
}
)";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Compiler IDE");
    app.setApplicationVersion("1.0");
    app.setStyleSheet(STYLESHEET);

    MainWindow window;
    window.show();

    window.loadFile(QDir::homePath() + "/compilador/exemplo.txt");

    return app.exec();
}
