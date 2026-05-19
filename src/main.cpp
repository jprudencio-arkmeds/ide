#include <QApplication>
#include <QDir>
#include "ide/MainWindow.h"

static const char* STYLESHEET = R"(
* { outline: none; }

QWidget {
    background-color: #13131a;
    color: #c8cce0;
    selection-background-color: #3d2f8c;
    selection-color: #ffffff;
    border: none;
}

QMenuBar {
    background-color: #0e0e18;
    color: #7070a0;
    padding: 1px 6px;
    border-bottom: 1px solid #1e1e2e;
    font-size: 13px;
    spacing: 2px;
}
QMenuBar::item { padding: 4px 12px; border-radius: 5px; }
QMenuBar::item:selected { background-color: #1e1e30; color: #c8cce0; }

QMenu {
    background-color: #1a1a26;
    color: #c8cce0;
    border: 1px solid #2a2a40;
    border-radius: 8px;
    padding: 4px;
}
QMenu::item { padding: 5px 28px 5px 14px; border-radius: 4px; }
QMenu::item:selected { background-color: #252538; }
QMenu::separator { height: 1px; background: #252538; margin: 4px 8px; }

QSplitter::handle:vertical   { background-color: #1e1e2e; height: 1px; }
QSplitter::handle:horizontal { background-color: #1e1e2e; width:  1px; }

QStatusBar {
    background-color: #0e0e18;
    color: #45456a;
    border-top: 1px solid #1e1e2e;
    padding: 2px 10px;
    font-size: 12px;
}
QStatusBar QLabel { color: #45456a; font-size: 12px; }

QScrollBar:vertical {
    background: #13131a; width: 7px; margin: 0;
}
QScrollBar::handle:vertical {
    background: #2a2a42; border-radius: 4px; min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: #3d2f8c; }
QScrollBar::add-line:vertical,  QScrollBar::sub-line:vertical  { height: 0; }
QScrollBar::add-page:vertical,  QScrollBar::sub-page:vertical  { background: none; }

QScrollBar:horizontal {
    background: #13131a; height: 7px; margin: 0;
}
QScrollBar::handle:horizontal {
    background: #2a2a42; border-radius: 4px; min-width: 24px;
}
QScrollBar::handle:horizontal:hover { background: #3d2f8c; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }

QDialog, QMessageBox { background-color: #1a1a26; border-radius: 10px; }
QMessageBox QLabel   { color: #c8cce0; }
QMessageBox QPushButton, QInputDialog QPushButton {
    background-color: #4a38aa; color: #ffffff;
    border-radius: 6px; padding: 6px 20px; min-width: 64px;
}
QMessageBox QPushButton:hover, QInputDialog QPushButton:hover {
    background-color: #5a48ba;
}

QInputDialog QLineEdit {
    background-color: #1a1a26; color: #c8cce0;
    border: 1px solid #2a2a42; border-radius: 5px; padding: 4px 8px;
}

QFileDialog QWidget { background-color: #1a1a26; color: #c8cce0; }
QFileDialog QListView, QFileDialog QTreeView {
    background-color: #13131a; color: #c8cce0;
    border: 1px solid #2a2a42; border-radius: 5px;
    selection-background-color: #3d2f8c;
}
QFileDialog QLineEdit {
    background-color: #1a1a26; color: #c8cce0;
    border: 1px solid #2a2a42; border-radius: 5px; padding: 4px 8px;
}
QFileDialog QPushButton {
    background-color: #4a38aa; color: #ffffff;
    border-radius: 5px; padding: 5px 14px;
}
QFileDialog QPushButton:hover { background-color: #5a48ba; }

QComboBox {
    background-color: #1a1a26; color: #c8cce0;
    border: 1px solid #2a2a42; border-radius: 5px; padding: 3px 8px;
}
)";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Compiler IDE");
    app.setApplicationVersion("1.0");
    app.setStyleSheet(STYLESHEET);

    MainWindow window;
    window.show();

    if (argc > 1)
        window.loadFile(argv[1]);

    return app.exec();
}
