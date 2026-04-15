#include "Lexico.h"
#include <QHash>

void Lexico::setInput(const char* input) {
    m_source = QString::fromUtf8(input);
    m_pos = 0; m_line = 1; m_col = 1;
}

void Lexico::setInput(const QString& input) {
    m_source = input;
    m_pos = 0; m_line = 1; m_col = 1;
}

bool  Lexico::atEnd()              const { return m_pos >= m_source.length(); }
QChar Lexico::current()            const { return atEnd() ? QChar() : m_source[m_pos]; }
QChar Lexico::peekChar(int offset) const {
    int idx = m_pos + offset;
    return (idx < m_source.length()) ? m_source[idx] : QChar();
}

QChar Lexico::advance() {
    QChar c = current();
    m_pos++;
    if (c == '\n') { m_line++; m_col = 1; } else { m_col++; }
    return c;
}

Token Lexico::makeToken(TokenType type, const QString& value,
                        int startLine, int startCol) const {
    return Token(type, value, startLine, startCol, m_pos);
}

// ── Public API ─────────────────────────────────────────────────────────────

Token* Lexico::nextToken() {
    // Skip whitespace and comments silently.
    for (;;) {
        if (atEnd()) return nullptr;

        QChar c = current();

        // Whitespace
        if (c.isSpace()) { advance(); continue; }

        // Line comment
        if (c == '/' && peekChar() == '/') {
            while (!atEnd() && current() != '\n') advance();
            continue;
        }

        // Multiline comment
        if (c == '/' && peekChar() == '*') {
            int sl = m_line, sc = m_col;
            advance(); advance();   // consume /*
            bool closed = false;
            while (!atEnd()) {
                if (current() == '*' && peekChar() == '/') {
                    advance(); advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed)
                throw LexicalError("Comentario multilinha nao fechado",
                                   Token(TokenType::UNKNOWN, "/*", sl, sc).position);
            continue;
        }

        break;
    }

    Token t = nextTokenInternal();

    // WS / comments produced by the internal scanner are also skipped.
    if (t.type == TokenType::WS ||
        t.type == TokenType::COMMENT_LINE ||
        t.type == TokenType::COMMENT_MULTILINE)
        return nextToken();

    if (t.type == TokenType::END_OF_FILE) return nullptr;

    return new Token(t);
}

std::vector<Token> Lexico::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        Token* tp = nextToken();
        if (!tp) break;
        tokens.push_back(*tp);
        delete tp;
    }
    tokens.emplace_back(TokenType::END_OF_FILE, "", m_line, m_col);
    return tokens;
}

// ── Internal scanner ────────────────────────────────────────────────────────

Token Lexico::nextTokenInternal() {
    if (atEnd())
        return makeToken(TokenType::END_OF_FILE, "", m_line, m_col);

    int   sl = m_line, sc = m_col;
    QChar c  = current();

    if (c.isSpace()) {
        QString ws;
        while (!atEnd() && current().isSpace()) ws += advance();
        return makeToken(TokenType::WS, ws, sl, sc);
    }

    if (c == '/') {
        if (peekChar() == '/') return readLineComment();
        if (peekChar() == '*') return readMultilineComment();
        advance();
        return makeToken(TokenType::DIVIDE, "/", sl, sc);
    }

    if (c == '"')  return readString();
    if (c == '\'') return readCharLit();
    if (c.isDigit()) return readNumber();
    if (c.isLetter() || c == '_') return readIdentOrKeyword();

    advance();
    switch (c.toLatin1()) {
        case '[': return makeToken(TokenType::LEFT_BRACKET,  "[", sl, sc);
        case ']': return makeToken(TokenType::RIGHT_BRACKET, "]", sl, sc);
        case '(': return makeToken(TokenType::LEFT_PAREN, "(", sl, sc);
        case ')': return makeToken(TokenType::RIGHT_PAREN, ")", sl, sc);
        case '{': return makeToken(TokenType::LEFT_BRACE, "{", sl, sc);
        case '}': return makeToken(TokenType::RIGHT_BRACE, "}", sl, sc);
        case ',': return makeToken(TokenType::SEPARATOR,     ",", sl, sc);
        case ';': return makeToken(TokenType::END_LINE,      ";", sl, sc);
        case '+': return makeToken(TokenType::PLUS,          "+", sl, sc);
        case '-': return makeToken(TokenType::MINUS,         "-", sl, sc);
        case '*': return makeToken(TokenType::MULTIPLY,      "*", sl, sc);
        case '%': return makeToken(TokenType::MOD,           "%", sl, sc);
        case '^': return makeToken(TokenType::BIT_XOR,       "^", sl, sc);
        case '~': return makeToken(TokenType::BIT_NOT,       "~", sl, sc);

        case '>':
            if (current() == '>') { advance(); return makeToken(TokenType::SHIFT_RIGHT,   ">>", sl, sc); }
            if (current() == '=') { advance(); return makeToken(TokenType::GREATER_EQUAL, ">=", sl, sc); }
            return makeToken(TokenType::GREATER, ">", sl, sc);

        case '<':
            if (current() == '<') { advance(); return makeToken(TokenType::SHIFT_LEFT, "<<", sl, sc); }
            if (current() == '=') { advance(); return makeToken(TokenType::LESS_EQUAL, "<=", sl, sc); }
            return makeToken(TokenType::LESS, "<", sl, sc);

        case '=':
            if (current() == '=') { advance(); return makeToken(TokenType::EQUAL,  "==", sl, sc); }
            return makeToken(TokenType::ASSIGN, "=", sl, sc);

        case '!':
            if (current() == '=') { advance(); return makeToken(TokenType::NOT_EQUAL, "!=", sl, sc); }
            return makeToken(TokenType::NOT, "!", sl, sc);

        case '&':
            if (current() == '&') { advance(); return makeToken(TokenType::AND,     "&&", sl, sc); }
            return makeToken(TokenType::BIT_AND, "&", sl, sc);

        case '|':
            if (current() == '|') { advance(); return makeToken(TokenType::OR,     "||", sl, sc); }
            return makeToken(TokenType::BIT_OR, "|", sl, sc);
    }

    return makeToken(TokenType::UNKNOWN, QString(c), sl, sc);
}

Token Lexico::readLineComment() {
    int sl = m_line, sc = m_col;
    QString text;
    while (!atEnd() && current() != '\n') text += advance();
    return makeToken(TokenType::COMMENT_LINE, text, sl, sc);
}

Token Lexico::readMultilineComment() {
    int sl = m_line, sc = m_col;
    QString text;
    text += advance(); text += advance();   // /*
    while (!atEnd()) {
        if (current() == '*' && peekChar() == '/') {
            text += advance(); text += advance();
            break;
        }
        text += advance();
    }
    return makeToken(TokenType::COMMENT_MULTILINE, text, sl, sc);
}

Token Lexico::readString() {
    int sl = m_line, sc = m_col;
    QString text;
    text += advance();   // opening "
    while (!atEnd() && current() != '"' && current() != '\n')
        text += advance();
    if (!atEnd() && current() == '"') text += advance();
    return makeToken(TokenType::LIT_STRING, text, sl, sc);
}

Token Lexico::readCharLit() {
    int sl = m_line, sc = m_col;
    QString text;
    text += advance();   // opening '
    if (!atEnd() && current() != '\'') text += advance();
    if (!atEnd() && current() == '\'') text += advance();
    return makeToken(TokenType::LIT_CHAR, text, sl, sc);
}

Token Lexico::readNumber() {
    int sl = m_line, sc = m_col;
    QString text;

    if (current() == '0' && (peekChar() == 'b' || peekChar() == 'B')) {
        text += advance(); text += advance();
        while (!atEnd() && (current() == '0' || current() == '1')) text += advance();
        return makeToken(TokenType::LIT_BINARY, text, sl, sc);
    }

    if (current() == '0' && (peekChar() == 'x' || peekChar() == 'X')) {
        text += advance(); text += advance();
        while (!atEnd() && (current().isDigit() ||
               (current().toLatin1() >= 'a' && current().toLatin1() <= 'f') ||
               (current().toLatin1() >= 'A' && current().toLatin1() <= 'F')))
            text += advance();
        return makeToken(TokenType::LIT_HEX, text, sl, sc);
    }

    while (!atEnd() && current().isDigit()) text += advance();

    if (!atEnd() && current() == '.' && peekChar().isDigit()) {
        text += advance();
        while (!atEnd() && current().isDigit()) text += advance();
        return makeToken(TokenType::LIT_REAL, text, sl, sc);
    }

    return makeToken(TokenType::LIT_INT, text, sl, sc);
}

Token Lexico::readIdentOrKeyword() {
    int sl = m_line, sc = m_col;
    QString text;
    while (!atEnd() && (current().isLetterOrNumber() || current() == '_'))
        text += advance();
    return makeToken(keywordType(text), text, sl, sc);
}

TokenType Lexico::keywordType(const QString& word) {
    static const QHash<QString, TokenType> kw = {
        {"if",       TokenType::KW_IF},
        {"else",     TokenType::KW_ELSE},
        {"int",      TokenType::KW_INT},
        {"float",    TokenType::KW_FLOAT},
        {"char",     TokenType::KW_CHAR},
        {"string",   TokenType::KW_STRING},
        {"void",     TokenType::KW_VOID},
        {"while",    TokenType::KW_WHILE},
        {"for",      TokenType::KW_FOR},
        {"do",       TokenType::KW_DO},
        {"return",   TokenType::KW_RETURN},
        {"break",    TokenType::KW_BREAK},
        {"continue", TokenType::KW_CONTINUE},
        {"read",     TokenType::KW_READ},
        {"write",    TokenType::KW_WRITE},
    };
    return kw.value(word, TokenType::ID);
}
