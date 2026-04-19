#include "MainWindow.h"
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"
#include "gals/LexicalError.h"
#include "gals/SyntacticError.h"

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

static void posToLineCol(const QString& src, int pos, int &line, int &col) {
    line = 1; col = 1;
    for (int i = 0; i < pos && i < src.size(); ++i) {
        if (src[i] == '\n') { ++line; col = 1; }
        else ++col;
    }
}

static QString tokenIdName(TokenId id) {
    switch (id) {
        case EPSILON: return "EPSILON";
        case DOLLAR:  return "$";
        default:      return QString("t_%1").arg((int)id);
    }
}

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

    std::string srcStd = source.toStdString();
    Lexico lexico(srcStd.c_str());
    std::vector<Token> tokens;
    int lexErrors = 0;

    try {
        while (true) {
            Token* t = lexico.nextToken();
            if (t == nullptr || t->getId() == DOLLAR) {
                if (t) tokens.push_back(*t);
                delete t;
                break;
            }
            tokens.push_back(*t);
            delete t;
        }
    } catch (LexicalError& e) {
        int line, col;
        posToLineCol(source, e.getPosition(), line, col);
        appendMessage(m_compilePanel,
            QString("  [ERROR] Line %1, Col %2: %3")
                .arg(line).arg(col).arg(e.getMessage()), MSG_ERROR);
        ++lexErrors;
    }

    if (lexErrors == 0)
        appendMessage(m_compilePanel,
            QString("  %1 token(s) recognized \u2014 no lexical errors.")
                .arg((int)tokens.size()), MSG_SUCCESS);
    else
        appendMessage(m_compilePanel,
            QString("  %1 lexical error(s) found.").arg(lexErrors), MSG_ERROR);

    showTokens(tokens);

    if (lexErrors > 0) {
        m_statusLabel->setText(QString("Lexical errors: %1").arg(lexErrors));
        m_tabs->setCurrentIndex(0);
        return;
    }

    // ── Syntactic Analysis ────────────────────────────────────────────────
    appendMessage(m_compilePanel, "");
    appendMessage(m_compilePanel, "=== Syntactic Analysis ===");

    try {
        Lexico   lexico2(srcStd.c_str());
        Semantico semantico;
        Sintatico sintatico;
        sintatico.parse(&lexico2, &semantico);
        appendMessage(m_compilePanel, "  Program parsed successfully.", MSG_SUCCESS);
    } catch (SyntacticError& e) {
        int line, col;
        posToLineCol(source, e.getPosition(), line, col);
        appendMessage(m_compilePanel,
            QString("  [ERROR] Line %1, Col %2: %3")
                .arg(line).arg(col).arg(e.getMessage()), MSG_ERROR);
        appendMessage(m_compilePanel, "\n  1 syntax error(s) found.", MSG_ERROR);
        m_statusLabel->setText("Compilation failed: 1 error(s)");
        m_tabs->setCurrentIndex(0);
        return;
    } catch (LexicalError& e) {
        int line, col;
        posToLineCol(source, e.getPosition(), line, col);
        appendMessage(m_compilePanel,
            QString("  [ERROR] Line %1, Col %2: %3")
                .arg(line).arg(col).arg(e.getMessage()), MSG_ERROR);
        m_tabs->setCurrentIndex(0);
        return;
    }
}

void MainWindow::showTokens(const std::vector<Token>& tokens) {
    m_tokenPanel->clear();
    const QString source = m_editor->toPlainText();
    QString out;
    out += QString(" %1  %2  %3  %4\n").arg("Line", 4).arg("Col", 3).arg("Type", -20).arg("Value");
    out += QString(60, '-') + "\n";

    for (const Token& t : tokens) {
        if (t.getId() == DOLLAR) continue;
        int line, col;
        posToLineCol(source, t.getPosition(), line, col);
        out += QString(" %1  %2  %3  %4\n")
            .arg(line, 4)
            .arg(col,  3)
            .arg(tokenIdName(t.getId()), -20)
            .arg(QString::fromStdString(t.getLexeme()));
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
