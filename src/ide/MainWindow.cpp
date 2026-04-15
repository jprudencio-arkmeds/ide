#include "MainWindow.h"
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"
#include "interpreter/Interpreter.h"

#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QFont>
#include <QAction>
#include <QKeySequence>
#include <QFileInfo>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QLabel>

// ── helpers ────────────────────────────────────────────────────────────────

static QFont panelFont() {
    QFont f;
    f.setFamilies({"Fira Code", "Fira Mono", "Courier New", "monospace"});
    f.setPointSize(14);
    f.setFixedPitch(true);
    return f;
}

static QPalette darkPalette() {
    QPalette p;
    p.setColor(QPalette::Base,            QColor("#0F0529"));
    p.setColor(QPalette::Text,            QColor("#e8d5ff"));
    p.setColor(QPalette::Highlight,       QColor("#7338A0"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Window,          QColor("#0F0529"));
    p.setColor(QPalette::WindowText,      QColor("#e8d5ff"));
    return p;
}

// ── MainWindow ──────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupMenus();
    setupStatusBar();
    setWindowTitle("Compiler IDE");
    resize(1280, 780);
}

void MainWindow::setupUI() {
    auto* outer = new QSplitter(Qt::Vertical, this);
    outer->setHandleWidth(4);

    auto* inner = new QSplitter(Qt::Horizontal, outer);
    inner->setHandleWidth(4);

    m_editor = new CodeEditor(this);
    m_editor->setPlaceholderText("Type your program here...");

    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto* tokenLabel = new QLabel("  Tokens", rightWidget);
    tokenLabel->setFont(panelFont());
    tokenLabel->setStyleSheet(
        "background:#4A2574; color:#9E72C3;"
        "padding:6px 10px; border-bottom:1px solid #7338A0;");

    m_tokenPanel = new QPlainTextEdit(rightWidget);
    m_tokenPanel->setReadOnly(true);
    m_tokenPanel->setFont(panelFont());
    m_tokenPanel->setPalette(darkPalette());
    m_tokenPanel->setPlaceholderText("Tokens will appear here after compilation...");

    rightLayout->addWidget(tokenLabel);
    rightLayout->addWidget(m_tokenPanel);

    inner->addWidget(m_editor);
    inner->addWidget(rightWidget);
    inner->setStretchFactor(0, 6);
    inner->setStretchFactor(1, 4);

    m_tabs = new QTabWidget(this);
    m_tabs->setFont(panelFont());
    m_tabs->setStyleSheet(
        "QTabWidget::pane  { border:none; background:#0F0529; }"
        "QTabBar::tab      { background:#4A2574; color:#9E72C3;"
        "                    padding:6px 18px; border-radius:6px 6px 0 0; margin-right:2px; }"
        "QTabBar::tab:selected { background:#7338A0; color:#ffffff; }"
        "QTabBar::tab:hover    { background:#7338A0; color:#e8d5ff; }");

    m_compilePanel = new QTextEdit(this);
    m_compilePanel->setReadOnly(true);
    m_compilePanel->setFont(panelFont());
    m_compilePanel->setPalette(darkPalette());

    m_outputPanel = new QTextEdit(this);
    m_outputPanel->setReadOnly(true);
    m_outputPanel->setFont(panelFont());
    m_outputPanel->setPalette(darkPalette());

    m_tabs->addTab(m_compilePanel, "Compilation");
    m_tabs->addTab(m_outputPanel,  "Output");

    outer->addWidget(inner);
    outer->addWidget(m_tabs);
    outer->setStretchFactor(0, 7);
    outer->setStretchFactor(1, 3);

    setCentralWidget(outer);

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, [this](bool modified) {
        QString title = "Compiler IDE";
        if (!m_currentFile.isEmpty())
            title += "  \u2014  " + QFileInfo(m_currentFile).fileName();
        if (modified) title += "  *";
        setWindowTitle(title);
    });
}

void MainWindow::setupMenus() {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction("&New",        this, &MainWindow::newFile,    QKeySequence::New);
    file->addAction("&Open...",    this, &MainWindow::openFile,   QKeySequence::Open);
    file->addSeparator();
    file->addAction("&Save",       this, &MainWindow::saveFile,   QKeySequence::Save);
    file->addAction("Save &As...", this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("E&xit",       this, &QWidget::close,         QKeySequence::Quit);

    QMenu* run = menuBar()->addMenu("&Run");
    run->addAction("&Compile + Run  (F5)", this, &MainWindow::compile,
                   QKeySequence(Qt::Key_F5));
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setFont(panelFont());
    statusBar()->addWidget(m_statusLabel);
}

static bool confirmDiscard(QWidget* p) {
    return QMessageBox::question(p, "Unsaved Changes",
        "Current file has unsaved changes. Discard?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

void MainWindow::loadFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    m_editor->setPlainText(QTextStream(&file).readAll());
    m_currentFile = path;
    m_editor->document()->setModified(false);
    setWindowTitle("Compiler IDE  \u2014  " + QFileInfo(path).fileName());
    m_statusLabel->setText("Opened: " + path);
}

void MainWindow::newFile() {
    if (m_editor->document()->isModified() && !confirmDiscard(this)) return;
    m_editor->clear(); clearMessages(); m_tokenPanel->clear();
    m_currentFile.clear();
    m_editor->document()->setModified(false);
    setWindowTitle("Compiler IDE");
    m_statusLabel->setText("New file");
}

void MainWindow::openFile() {
    if (m_editor->document()->isModified() && !confirmDiscard(this)) return;
    QString path = QFileDialog::getOpenFileName(this, "Open File", "",
        "Source files (*.txt *.src *.lang);;All files (*)");
    if (path.isEmpty()) return;
    loadFile(path);
}

bool MainWindow::saveFile() {
    if (m_currentFile.isEmpty()) return saveFileAs();
    QFile file(m_currentFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot save: " + m_currentFile);
        return false;
    }
    QTextStream(&file) << m_editor->toPlainText();
    m_editor->document()->setModified(false);
    setWindowTitle("Compiler IDE  \u2014  " + QFileInfo(m_currentFile).fileName());
    m_statusLabel->setText("Saved");
    return true;
}

bool MainWindow::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this, "Save As", "",
        "Source files (*.txt *.src *.lang);;All files (*)");
    if (path.isEmpty()) return false;
    m_currentFile = path;
    return saveFile();
}

// ── Compile pipeline ────────────────────────────────────────────────────────

void MainWindow::compile() {
    clearMessages();
    m_tokenPanel->clear();
    const QString source = m_editor->toPlainText();

    if (source.trimmed().isEmpty()) {
        appendMessage(m_compilePanel, "  No source code to analyze.", MSG_WARNING);
        return;
    }

    // ── Lexical Analysis ──────────────────────────────────────────────────
    appendMessage(m_compilePanel, "=== Lexical Analysis ===");

    Lexico lexico(source);
    const std::vector<Token> tokens = lexico.tokenize();

    int unknowns = 0;
    for (const Token& t : tokens)
        if (t.type == TokenType::UNKNOWN) {
            appendMessage(m_compilePanel,
                QString("  [ERROR] Line %1, Col %2: Unknown token '%3'")
                    .arg(t.line).arg(t.col).arg(t.value), MSG_ERROR);
            ++unknowns;
        }

    if (unknowns == 0)
        appendMessage(m_compilePanel,
            QString("  %1 token(s) recognized — no lexical errors.")
                .arg((int)tokens.size() - 1), MSG_SUCCESS);
    else
        appendMessage(m_compilePanel,
            QString("  %1 lexical error(s) found.").arg(unknowns), MSG_ERROR);

    showTokens(tokens);

    if (unknowns > 0) {
        m_statusLabel->setText(QString("Lexical errors: %1").arg(unknowns));
        m_tabs->setCurrentIndex(0);
        return;
    }

    // ── Syntactic Analysis ────────────────────────────────────────────────
    appendMessage(m_compilePanel, "");
    appendMessage(m_compilePanel, "=== Syntactic Analysis ===");

    Lexico   lexico2(source);
    Semantico semantico;
    Sintatico sintatico;
    sintatico.parse(&lexico2, &semantico);

    if (sintatico.hasErrors()) {
        for (const ParseError& e : sintatico.errors())
            appendMessage(m_compilePanel,
                QString("  [ERROR] Line %1, Col %2: %3")
                    .arg(e.line).arg(e.col).arg(e.message), MSG_ERROR);
        appendMessage(m_compilePanel,
            QString("\n  %1 syntax error(s) found.")
                .arg(sintatico.errors().size()), MSG_ERROR);
        m_statusLabel->setText(
            QString("Compilation failed: %1 error(s)").arg(sintatico.errors().size()));
        m_tabs->setCurrentIndex(0);
        return;
    }

    appendMessage(m_compilePanel, "  Program parsed successfully.", MSG_SUCCESS);
}

void MainWindow::showTokens(const std::vector<Token>& tokens) {
    m_tokenPanel->clear();
    QString out;
    out += QString(" %1  %2  %3s  %4\n").arg("Line", 4).arg("Col", 3).arg("Type").arg("Value");
    out += QString(60, '-') + "\n";

    for (const Token& t : tokens) {
        if (t.type == TokenType::END_OF_FILE) continue;
        out += QString(" %1  %2  %3  %4\n")
            .arg(t.line, 4)
            .arg(t.col,  3)
            .arg(tokenTypeName(t.type), -20)
            .arg(t.value);
    }
    m_tokenPanel->setPlainText(out);
}

void MainWindow::appendMessage(QTextEdit* panel, const QString& text, MessageKind kind) {
    static const QColor colors[] = {
        QColor("#c9a8f5"),
        QColor("#5dba7d"),
        QColor("#f5c842"),
        QColor("#ff6b6b"),
    };
    QTextCharFormat fmt;
    fmt.setForeground(colors[static_cast<int>(kind)]);
    QTextCursor c = panel->textCursor();
    c.movePosition(QTextCursor::End);
    c.insertText(text + "\n", fmt);
    panel->setTextCursor(c);
    panel->ensureCursorVisible();
}

void MainWindow::clearMessages() {
    m_compilePanel->clear();
    m_outputPanel->clear();
}
