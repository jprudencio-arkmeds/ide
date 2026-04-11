#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QResizeEvent>

static const QColor BG_EDITOR       ("#0F0529");
static const QColor BG_GUTTER       ("#4A2574");
static const QColor COLOR_LINENO    ("#9E72C3");
static const QColor COLOR_CURLINE   ("#1a083d");
static const QColor COLOR_SELECTION ("#7338A0");
static const QColor COLOR_TEXT      ("#f0e6ff");

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
    QTextEdit::ExtraSelection sel;
    sel.format.setBackground(COLOR_CURLINE);
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    sel.cursor = textCursor();
    sel.cursor.clearSelection();
    setExtraSelections({sel});
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), BG_GUTTER);

    painter.setPen(QColor("#7338A0"));
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
