#include "MainWindow.h"
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"
#include "gals/LexicalError.h"
#include "gals/SyntacticError.h"
#include "gals/SemanticError.h"

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
#include <QHeaderView>

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

static QFont panelFont(int pt = 12) {
    QFont f;
    f.setFamilies({"Fira Code", "Fira Mono", "Cascadia Code", "Courier New", "monospace"});
    f.setPointSize(pt);
    f.setFixedPitch(true);
    return f;
}

static QPalette darkPalette() {
    QPalette p;
    p.setColor(QPalette::Base,            QColor("#13131a"));
    p.setColor(QPalette::Text,            QColor("#c8cce0"));
    p.setColor(QPalette::Highlight,       QColor("#3d2f8c"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Window,          QColor("#13131a"));
    p.setColor(QPalette::WindowText,      QColor("#c8cce0"));
    return p;
}

static const char* TAB_STYLE =
    "QTabWidget::pane  { border:none; background:#13131a; }"
    "QTabBar::tab      { background:#0e0e18; color:#45456a;"
    "                    padding:7px 18px; margin-right:1px;"
    "                    border-top: 2px solid transparent; }"
    "QTabBar::tab:selected { background:#1a1a26; color:#c8cce0;"
    "                         border-top: 2px solid #5a44b0; }"
    "QTabBar::tab:hover    { background:#14141e; color:#9090c0; }";

static const char* TABLE_STYLE =
    "QTableWidget { background:#13131a; color:#c8cce0;"
    "               gridline-color:#1e1e2e; border:none; }"
    "QTableWidget::item { padding:4px 8px;"
    "                     border-bottom:1px solid #1a1a28; }"
    "QTableWidget::item:selected { background:#3d2f8c; color:#ffffff; }"
    "QHeaderView::section { background:#0e0e18; color:#45456a;"
    "                        padding:6px 8px; border:none;"
    "                        border-bottom:1px solid #2a2a40;"
    "                        font-size:11px; text-transform:uppercase;"
    "                        letter-spacing:1px; }";

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
    outer->setHandleWidth(1);

    auto* inner = new QSplitter(Qt::Horizontal, outer);
    inner->setHandleWidth(1);

    // ── Editor ──────────────────────────────────────────────────────────────
    m_editor = new CodeEditor(this);
    m_editor->setPlaceholderText("Type your program here...");
    m_editor->setFont(panelFont(13));

    // ── Right panel: Tokens ──────────────────────────────────────────────────
    m_rightTabs = new QTabWidget(this);
    m_rightTabs->setFont(panelFont());
    m_rightTabs->setStyleSheet(TAB_STYLE);

    m_tokenTable = new QTableWidget(0, 4, this);
    m_tokenTable->setHorizontalHeaderLabels({"Line", "Col", "Type", "Value"});
    m_tokenTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_tokenTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tokenTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tokenTable->setShowGrid(false);
    m_tokenTable->verticalHeader()->setVisible(false);
    m_tokenTable->verticalHeader()->setDefaultSectionSize(24);
    m_tokenTable->setFont(panelFont());
    m_tokenTable->setPalette(darkPalette());
    m_tokenTable->setStyleSheet(TABLE_STYLE);

    m_asmPanel = new QPlainTextEdit(this);
    QFont font;
    font.setFamilies({ "Fira Code", "Fira Mono", "Courier New", "monospace" });
    font.setPointSize(14);
    font.setFixedPitch(true);
    m_asmPanel->setFont(font);
    m_asmPanel->setReadOnly(true);
    m_asmPanel->setPlainText( // place holder
      ".data\n"
      "\t"
      ".text\n"
      "\t"
    );

    m_rightTabs->addTab(m_tokenTable, "Tokens");
    m_rightTabs->addTab(m_asmPanel, "Assembly");

    inner->addWidget(m_editor);
    inner->addWidget(m_rightTabs);
    inner->setStretchFactor(0, 63);
    inner->setStretchFactor(1, 37);

    // ── Footer tabs ──────────────────────────────────────────────────────────
    m_footerTabs = new QTabWidget(this);
    m_footerTabs->setFont(panelFont());
    m_footerTabs->setStyleSheet(TAB_STYLE);

    m_compilePanel = new QTextEdit(this);
    m_compilePanel->setReadOnly(true);
    m_compilePanel->setFont(panelFont());
    m_compilePanel->setPalette(darkPalette());
    m_compilePanel->setStyleSheet(
        "QTextEdit { background:#13131a; padding:8px 12px;"
        "            border:none; line-height:1.5; }");

    m_outputPanel = new QTextEdit(this);
    m_outputPanel->setReadOnly(true);
    m_outputPanel->setFont(panelFont());
    m_outputPanel->setPalette(darkPalette());
    m_outputPanel->setStyleSheet(
        "QTextEdit { background:#13131a; padding:8px 12px; border:none; }");

    m_symbolTable = new QTableWidget(0, 6, this);
    m_symbolTable->setHorizontalHeaderLabels(
        {"Nome", "Tipo", "Modalidade", "Escopo", "Inicializado", "Usado"});
    m_symbolTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_symbolTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_symbolTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_symbolTable->setShowGrid(false);
    m_symbolTable->verticalHeader()->setVisible(false);
    m_symbolTable->verticalHeader()->setDefaultSectionSize(26);
    m_symbolTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_symbolTable->horizontalHeader()->setStretchLastSection(true);
    m_symbolTable->setFont(panelFont());
    m_symbolTable->setPalette(darkPalette());
    m_symbolTable->setStyleSheet(TABLE_STYLE);
    m_symbolTable->setColumnWidth(0, 140);
    m_symbolTable->setColumnWidth(1, 80);
    m_symbolTable->setColumnWidth(2, 110);
    m_symbolTable->setColumnWidth(3, 170);
    m_symbolTable->setColumnWidth(4, 100);

    m_footerTabs->addTab(m_compilePanel, "Compilation");
    m_footerTabs->addTab(m_outputPanel,  "Output");
    m_footerTabs->addTab(m_symbolTable,  "Símbolos");

    outer->addWidget(inner);
    outer->addWidget(m_footerTabs);
    outer->setStretchFactor(0, 68);
    outer->setStretchFactor(1, 32);

    setCentralWidget(outer);

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, [this](bool modified) {
        QString title = "Compiler IDE";
        if (!m_currentFile.isEmpty())
            title += "  \u2014  " + QFileInfo(m_currentFile).fileName();
        if (modified) title += "  *";
        setWindowTitle(title);
    });

    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorPosition);
}

void MainWindow::setupMenus() {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction("&New",        this, &MainWindow::newFile,    QKeySequence::New);
    file->addAction("&Open",       this, &MainWindow::openFile,   QKeySequence::Open);
    file->addAction("&Save",       this, &MainWindow::saveFile,   QKeySequence::Save);
    file->addAction("Save &As...", this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    auto* compileAction = new QAction("&Compile (F5)", this);
    compileAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(compileAction, &QAction::triggered, this, &MainWindow::compile);
    menuBar()->addAction(compileAction);
}

void MainWindow::setupStatusBar() {
    statusBar()->setStyleSheet(
        "QStatusBar { background:#0e0e18; border-top:1px solid #1e1e2e;"
        "             padding:0 10px; font-size:12px; }");

    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setFont(panelFont());
    m_statusLabel->setStyleSheet("color:#45456a;");
    statusBar()->addWidget(m_statusLabel);

    m_cursorLabel = new QLabel("Ln 1, Col 1", this);
    m_cursorLabel->setFont(panelFont());
    m_cursorLabel->setStyleSheet("color:#45456a; padding-right:4px;");
    statusBar()->addPermanentWidget(m_cursorLabel);
}

void MainWindow::updateCursorPosition() {
    QTextCursor c = m_editor->textCursor();
    m_cursorLabel->setText(
        QString("Ln %1, Col %2")
            .arg(c.blockNumber() + 1)
            .arg(c.positionInBlock() + 1));
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
    m_editor->clear();
    m_editor->clearErrorSelections();
    m_currentFile.clear();
    clearMessages();
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
    m_editor->clearErrorSelections();
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
        appendError(line, col, e.getMessage());
        ++lexErrors;
    }

    if (lexErrors == 0)
        appendMessage(m_compilePanel,
            QString("  %1 token(s) recognized \u2014 no lexical errors.")
                .arg((int)tokens.size()), MSG_SUCCESS);
    else
        appendMessage(m_compilePanel, QString("  %1 lexical error(s) found.").arg(lexErrors), MSG_ERROR);

    showTokens(tokens);

    if (lexErrors > 0) {
        m_statusLabel->setText(QString("Lexical errors: %1").arg(lexErrors));
        m_footerTabs->setCurrentIndex(0);
        return;
    }

    // ── Syntactic Analysis ────────────────────────────────────────────────
    appendMessage(m_compilePanel, "");
    appendMessage(m_compilePanel, "=== Syntactic Analysis ===");

    try {
        Lexico    lexico2(srcStd.c_str());
        Semantico semantico;
        Sintatico sintatico;
        sintatico.parse(&lexico2, &semantico);
        appendMessage(m_compilePanel, "  Program parsed successfully.", MSG_SUCCESS);

        // ── Semantic Analysis ─────────────────────────────────────────────
        appendMessage(m_compilePanel, "");
        appendMessage(m_compilePanel, "=== Semantic Analysis ===");
        semantico.analyze(tokens);
        const auto& errors   = semantico.errors();
        const auto& warnings = semantico.warnings();

        // Destaque de linhas no editor
        QList<QTextEdit::ExtraSelection> editorSels;
        auto makeLineSel = [&](int pos, QColor bg) {
            int line, col; posToLineCol(source, pos, line, col);
            QTextCursor cur(m_editor->document());
            cur.movePosition(QTextCursor::Start);
            cur.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
            cur.select(QTextCursor::LineUnderCursor);
            QTextEdit::ExtraSelection sel;
            sel.cursor = cur;
            sel.format.setBackground(bg);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            editorSels.append(sel);
        };
        for (const auto& w : warnings) makeLineSel(w.getPosition(), QColor("#2a1e00"));
        for (const auto& e : errors)   makeLineSel(e.getPosition(), QColor("#2d0c0c"));
        m_editor->setErrorSelections(editorSels);

        if (errors.empty()) {
            appendMessage(m_compilePanel, "  No semantic errors.", MSG_SUCCESS);
        } else {
            for (const auto& e : errors) {
                int line, col;
                posToLineCol(source, e.getPosition(), line, col);
                appendError(line, col, e.getMessage());
            }
            appendMessage(m_compilePanel,
                QString("  %1 semantic error(s) found.").arg((int)errors.size()), MSG_ERROR);
        }
        if (!warnings.empty()) {
            for (const auto& w : warnings) {
                int line, col;
                posToLineCol(source, w.getPosition(), line, col);
                appendWarning(line, col, w.getMessage());
            }
            appendMessage(m_compilePanel,
                QString("  %1 semantic warning(s) found.").arg((int)warnings.size()), MSG_WARNING);
        }

        // Status bar e troca automática de aba
        if (errors.empty() && warnings.empty()) {
            m_statusLabel->setText("Compilation successful — no errors");
            m_footerTabs->setCurrentIndex(2); // Símbolos
        } else if (!errors.empty()) {
            m_statusLabel->setText(
                QString("Compilation: %1 error(s), %2 warning(s)")
                    .arg(errors.size()).arg(warnings.size()));
            m_footerTabs->setCurrentIndex(0);
        } else {
            m_statusLabel->setText(
                QString("Compilation: %1 warning(s)").arg(warnings.size()));
            m_footerTabs->setCurrentIndex(0);
        }

        showSymbolTable(semantico.symbolTable());
    }
    catch (LexicalError& e) {
      int line, col;
      posToLineCol(source, e.getPosition(), line, col);
      appendError(line, col, e.getMessage());
      m_footerTabs->setCurrentIndex(0);
      return;
    }
    catch (SyntacticError& e) {
      int line, col;
      posToLineCol(source, e.getPosition(), line, col);
      appendError(line, col, e.getMessage());
      appendMessage(m_compilePanel, "\n  1 syntax error(s) found.", MSG_ERROR);
      m_statusLabel->setText("Compilation failed: 1 error(s)");
      m_footerTabs->setCurrentIndex(0);
      return;
    }
    catch (SemanticError& e) {
      int line, col;
      posToLineCol(source, e.getPosition(), line, col);
      appendError(line, col, e.getMessage());
      appendMessage(m_compilePanel, "\n  1 syntax error(s) found.", MSG_ERROR);
      m_statusLabel->setText("Compilation failed: 1 error(s)");
      m_footerTabs->setCurrentIndex(0);
      return;
    }
}

void MainWindow::showTokens(const std::vector<Token>& tokens) {
    m_tokenTable->setRowCount(0);
    const QString source = m_editor->toPlainText();

    // Configura colunas na primeira exibição
    auto* hdr = m_tokenTable->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::Fixed);
    hdr->setSectionResizeMode(1, QHeaderView::Fixed);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(3, QHeaderView::Stretch);
    m_tokenTable->setColumnWidth(0, 50);
    m_tokenTable->setColumnWidth(1, 45);

    for (const Token& t : tokens) {
        if (t.getId() == DOLLAR) continue;
        const int row = m_tokenTable->rowCount();
        m_tokenTable->insertRow(row);

        int line, col;
        posToLineCol(source, t.getPosition(), line, col);

        auto* lineItem = new QTableWidgetItem(QString::number(line));
        auto* colItem  = new QTableWidgetItem(QString::number(col));

        m_tokenTable->setItem(row, 0, lineItem);
        m_tokenTable->setItem(row, 1, colItem);
        m_tokenTable->setItem(row, 2, new QTableWidgetItem(tokenIdName(t.getId())));
        m_tokenTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(t.getLexeme())));
    }
}

void MainWindow::appendError(const int line, const int col, const QString& message) {
    appendMessage(m_compilePanel,
        QString("  [ERROR] Line %1, Col %2: %3").arg(line).arg(col).arg(message), MSG_ERROR);
}

void MainWindow::appendWarning(const int line, const int col, const QString& message) {
    appendMessage(m_compilePanel,
        QString("  [WARNING] Line %1, Col %2: %3").arg(line).arg(col).arg(message), MSG_WARNING);
}

void MainWindow::appendMessage(QTextEdit* panel, const QString& text, MessageKind kind) {
    static const QColor colors[] = {
        QColor("#8888b0"),   // INFO    — cinza-roxo
        QColor("#4ec97a"),   // SUCCESS — verde
        QColor("#d4a542"),   // WARNING — âmbar
        QColor("#e05c5c"),   // ERROR   — vermelho
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
    m_tokenTable->setRowCount(0);
    m_symbolTable->setRowCount(0);
}

void MainWindow::showSymbolTable(const SymbolTable& table) {
    m_symbolTable->setRowCount(0);

    for (const auto& record : table.allSymbols()) {
        const int row = m_symbolTable->rowCount();
        m_symbolTable->insertRow(row);

        QString modality;
        switch (record.symbol->modality) {
            case Modality::VARIABLE:  modality = "Variável";  break;
            case Modality::ARRAY:     modality = "Vetor";     break;
            case Modality::PARAMETER: modality = "Parâmetro"; break;
            case Modality::FUNCTION:  modality = "Função";    break;
        }

        const int depth = record.symbol->scopeDepth;
        const QString scope = depth <= 1 ? "Global" : QString("Local (nível %1)").arg(depth);

        m_symbolTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(record.name)));
        m_symbolTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(record.symbol->type)));
        m_symbolTable->setItem(row, 2, new QTableWidgetItem(modality));
        m_symbolTable->setItem(row, 3, new QTableWidgetItem(scope));
        m_symbolTable->setItem(row, 4, new QTableWidgetItem(record.symbol->isInitialized ? "Sim" : "Não"));
        m_symbolTable->setItem(row, 5, new QTableWidgetItem(record.symbol->isUsed        ? "Sim" : "Não"));

        // Colorir linha conforme estado do símbolo
        QColor rowBg;
        if (!record.symbol->isUsed && record.symbol->modality != Modality::FUNCTION)
            rowBg = QColor("#221c00");         // âmbar escuro — declarado mas não usado
        else if (!record.symbol->isInitialized && record.symbol->modality == Modality::VARIABLE)
            rowBg = QColor("#220a0a");         // vermelho escuro — não inicializado
        if (rowBg.isValid())
            for (int c = 0; c < 6; ++c)
                if (auto* item = m_symbolTable->item(row, c))
                    item->setBackground(rowBg);
    }
}
