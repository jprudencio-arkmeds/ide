#include "Lexer.h"
#include <QHash>

Lexer::Lexer(const QString& source)
    : m_source(source), m_pos(0), m_line(1), m_col(1)
{}

bool Lexer::atEnd() const {
    return m_pos >= m_source.length();
}

QChar Lexer::current() const {
    if (atEnd()) return QChar();
    return m_source[m_pos];
}

QChar Lexer::peekChar(int offset) const {
    int idx = m_pos + offset;
    if (idx >= m_source.length()) return QChar();
    return m_source[idx];
}

QChar Lexer::advance() {
    QChar c = current();
    m_pos++;
    if (c == '\n') { m_line++; m_col = 1; }
    else           { m_col++; }
    return c;
}

Token Lexer::makeToken(TokenType type, const QString& value, int startLine, int startCol) {
    return Token(type, value, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!atEnd()) {
        Token t = nextToken();
        if (t.type != TokenType::WS &&
            t.type != TokenType::COMMENT_LINE &&
            t.type != TokenType::COMMENT_MULTILINE) {
            tokens.push_back(t);
        }
    }
    tokens.push_back(Token(TokenType::END_OF_FILE, "", m_line, m_col));
    return tokens;
}

Token Lexer::nextToken() {
    int startLine = m_line;
    int startCol  = m_col;
    QChar c       = current();

    if (c.isSpace()) {
        QString ws;
        while (!atEnd() && current().isSpace())
            ws += advance();
        return makeToken(TokenType::WS, ws, startLine, startCol);
    }

    if (c == '/') {
        if (peekChar() == '/') return readLineComment();
        if (peekChar() == '*') return readMultilineComment();
        advance();
        return makeToken(TokenType::DIVIDE, "/", startLine, startCol);
    }

    if (c == '"') return readString();

    if (c == '\'') return readCharLit();

    if (c.isDigit()) return readNumber();

    if (c.isLetter() || c == '_') return readIdentOrKeyword();

    advance();
    switch (c.toLatin1()) {
        case '[': return makeToken(TokenType::LEFT_BRACKET,  "[", startLine, startCol);
        case ']': return makeToken(TokenType::RIGHT_BRACKET, "]", startLine, startCol);
        case '(': return makeToken(TokenType::LEFT_PAREN,    "(", startLine, startCol);
        case ')': return makeToken(TokenType::RIGHT_PAREN,   ")", startLine, startCol);
        case '{': return makeToken(TokenType::LEFT_BRACE,    "{", startLine, startCol);
        case '}': return makeToken(TokenType::RIGHT_BRACE,   "}", startLine, startCol);
        case ',': return makeToken(TokenType::SEPARATOR,     ",", startLine, startCol);
        case ';': return makeToken(TokenType::END_LINE,      ";", startLine, startCol);
        case '+': return makeToken(TokenType::PLUS,          "+", startLine, startCol);
        case '-': return makeToken(TokenType::MINUS,         "-", startLine, startCol);
        case '*': return makeToken(TokenType::MULTIPLY,      "*", startLine, startCol);
        case '%': return makeToken(TokenType::MOD,           "%", startLine, startCol);
        case '^': return makeToken(TokenType::BIT_XOR,       "^", startLine, startCol);
        case '~': return makeToken(TokenType::BIT_NOT,       "~", startLine, startCol);

        case '>':
            if (current() == '>') { advance(); return makeToken(TokenType::SHIFT_RIGHT,   ">>", startLine, startCol); }
            if (current() == '=') { advance(); return makeToken(TokenType::GREATER_EQUAL, ">=", startLine, startCol); }
            return makeToken(TokenType::GREATER, ">", startLine, startCol);

        case '<':
            if (current() == '<') { advance(); return makeToken(TokenType::SHIFT_LEFT,  "<<", startLine, startCol); }
            if (current() == '=') { advance(); return makeToken(TokenType::LESS_EQUAL,  "<=", startLine, startCol); }
            return makeToken(TokenType::LESS, "<", startLine, startCol);

        case '=':
            if (current() == '=') { advance(); return makeToken(TokenType::EQUAL,  "==", startLine, startCol); }
            return makeToken(TokenType::ASSIGN, "=", startLine, startCol);

        case '!':
            if (current() == '=') { advance(); return makeToken(TokenType::NOT_EQUAL, "!=", startLine, startCol); }
            return makeToken(TokenType::NOT, "!", startLine, startCol);

        case '&':
            if (current() == '&') { advance(); return makeToken(TokenType::AND,     "&&", startLine, startCol); }
            return makeToken(TokenType::BIT_AND, "&", startLine, startCol);

        case '|':
            if (current() == '|') { advance(); return makeToken(TokenType::OR,     "||", startLine, startCol); }
            return makeToken(TokenType::BIT_OR, "|", startLine, startCol);
    }

    return makeToken(TokenType::UNKNOWN, QString(c), startLine, startCol);
}

Token Lexer::readLineComment() {
    int startLine = m_line, startCol = m_col;
    QString text;
    while (!atEnd() && current() != '\n')
        text += advance();
    return makeToken(TokenType::COMMENT_LINE, text, startLine, startCol);
}

Token Lexer::readMultilineComment() {
    int startLine = m_line, startCol = m_col;
    QString text;
    text += advance();
    text += advance();
    while (!atEnd()) {
        if (current() == '*' && peekChar() == '/') {
            text += advance();
            text += advance();
            break;
        }
        text += advance();
    }
    return makeToken(TokenType::COMMENT_MULTILINE, text, startLine, startCol);
}

Token Lexer::readString() {
    int startLine = m_line, startCol = m_col;
    QString text;
    text += advance();
    while (!atEnd() && current() != '"' && current() != '\n')
        text += advance();
    if (!atEnd() && current() == '"')
        text += advance();
    return makeToken(TokenType::LIT_STRING, text, startLine, startCol);
}

Token Lexer::readCharLit() {
    int startLine = m_line, startCol = m_col;
    QString text;
    text += advance();
    if (!atEnd() && current() != '\'')
        text += advance();
    if (!atEnd() && current() == '\'')
        text += advance();
    return makeToken(TokenType::LIT_CHAR, text, startLine, startCol);
}

Token Lexer::readNumber() {
    int startLine = m_line, startCol = m_col;
    QString text;

    if (current() == '0' && (peekChar() == 'b' || peekChar() == 'B')) {
        text += advance();
        text += advance();
        while (!atEnd() && (current() == '0' || current() == '1'))
            text += advance();
        return makeToken(TokenType::LIT_BINARY, text, startLine, startCol);
    }

    if (current() == '0' && (peekChar() == 'x' || peekChar() == 'X')) {
        text += advance();
        text += advance();
        while (!atEnd() && (current().isDigit() ||
               (current().toLatin1() >= 'a' && current().toLatin1() <= 'f') ||
               (current().toLatin1() >= 'A' && current().toLatin1() <= 'F')))
            text += advance();
        return makeToken(TokenType::LIT_HEX, text, startLine, startCol);
    }

    while (!atEnd() && current().isDigit())
        text += advance();

    if (!atEnd() && current() == '.' && peekChar().isDigit()) {
        text += advance();
        while (!atEnd() && current().isDigit())
            text += advance();
        return makeToken(TokenType::LIT_REAL, text, startLine, startCol);
    }

    return makeToken(TokenType::LIT_INT, text, startLine, startCol);
}

Token Lexer::readIdentOrKeyword() {
    int startLine = m_line, startCol = m_col;
    QString text;
    while (!atEnd() && (current().isLetterOrNumber() || current() == '_'))
        text += advance();
    TokenType kw = keywordType(text);
    return makeToken(kw, text, startLine, startCol);
}

TokenType Lexer::keywordType(const QString& word) {
    static const QHash<QString, TokenType> kw = {
        {"if",       TokenType::KW_IF},
        {"then",     TokenType::KW_THEN},
        {"else",     TokenType::KW_ELSE},
        {"end",      TokenType::KW_END},
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
