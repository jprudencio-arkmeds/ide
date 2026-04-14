#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include "CodeEditor.h"
#include "gals/Token.h"
#include "gals/AST.h"

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

private:
    void setupUI();
    void setupMenus();
    void setupStatusBar();

    enum MessageKind { MSG_INFO, MSG_SUCCESS, MSG_WARNING, MSG_ERROR };
    void appendMessage(QTextEdit* panel, const QString& text, MessageKind kind = MSG_INFO);
    void clearMessages();
    void showTokens    (const std::vector<Token>& tokens);
    void runInterpreter(const std::shared_ptr<ProgramNode>& ast);

    CodeEditor*     m_editor;
    QPlainTextEdit* m_tokenPanel;
    QTextEdit*      m_compilePanel;
    QTextEdit*      m_outputPanel;
    QTabWidget*     m_tabs;
    QLabel*         m_statusLabel;

    QString m_currentFile;
};
