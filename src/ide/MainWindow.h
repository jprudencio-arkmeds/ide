#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include "CodeEditor.h"
#include "gals/Token.h"
#include "gals/SymbolTable.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadFile(const QString& path);

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void compile();
    void exportAsm();
    void updateCursorPosition();

private:
    void setupUI();
    void setupMenus();
    void setupStatusBar();

    enum MessageKind { MSG_INFO, MSG_SUCCESS, MSG_WARNING, MSG_ERROR };
    void appendMessage(QTextEdit* panel, const QString& text, MessageKind kind = MSG_INFO);
    void appendError(const int line, const int col, const QString& message);
    void appendWarning(const int line, const int col, const QString& message);
    void clearMessages();
    void showTokens     (const std::vector<Token>& tokens);
    void showSymbolTable(const SymbolTable& table);

    CodeEditor*     m_editor;
    QTableWidget*   m_tokenTable;
    QPlainTextEdit* m_asmPanel;
    QTextEdit*      m_compilePanel;
    QTextEdit*      m_outputPanel;
    QTableWidget*   m_symbolTable;
    QTabWidget*     m_rightTabs;
    QTabWidget*     m_footerTabs;
    QLabel*         m_statusLabel;

    QString m_currentFile;
    QLabel* m_cursorLabel;
};
