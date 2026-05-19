#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QResizeEvent>

static const QColor BG_EDITOR       ("#0c1a0c");   // dark forest — referência à imagem
static const QColor BG_GUTTER       ("#0a1509");   // ligeiramente mais escuro
static const QColor COLOR_LINENO    ("#385438");   // verde acinzentado
static const QColor COLOR_CURLINE   ("#132613");   // linha atual: tint verde suave
static const QColor COLOR_SELECTION ("#1e401e");   // seleção: verde mais profundo
static const QColor COLOR_TEXT      ("#c4d0c0");   // texto: branco-esverdeado quente

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);

    QFont font;
    font.setFamilies({"Fira Code", "Fira Mono", "Courier New", "monospace"});
    font.setPointSize(14);
    font.setFixedPitch(true);
    setFont(font);

    QPalette p = palette();
    p.setColor(QPalette::Base,            BG_EDITOR);
    p.setColor(QPalette::Text,            COLOR_TEXT);
    p.setColor(QPalette::Highlight,       COLOR_SELECTION);
    p.setColor(QPalette::HighlightedText, Qt::white);
    setPalette(p);

    setCursorWidth(2);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max    = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> all = m_errorSelections;

    QTextEdit::ExtraSelection cur;
    cur.format.setBackground(COLOR_CURLINE);
    cur.format.setProperty(QTextFormat::FullWidthSelection, true);
    cur.cursor = textCursor();
    cur.cursor.clearSelection();
    all.append(cur);

    setExtraSelections(all);
}

void CodeEditor::setErrorSelections(const QList<QTextEdit::ExtraSelection>& sels) {
    m_errorSelections = sels;
    highlightCurrentLine();
}

void CodeEditor::clearErrorSelections() {
    m_errorSelections.clear();
    highlightCurrentLine();
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), BG_GUTTER);

    painter.setPen(QColor("#1a2e1a"));
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());

    QTextBlock block      = firstVisibleBlock();
    int blockNumber       = block.blockNumber();
    int top    = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            bool isCurrent = (blockNumber == textCursor().blockNumber());
            painter.setPen(isCurrent ? Qt::white : COLOR_LINENO);
            painter.drawText(0, top,
                             m_lineNumberArea->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block       = block.next();
        top         = bottom;
        bottom      = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
