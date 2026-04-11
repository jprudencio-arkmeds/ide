#pragma once
#include "Token.h"
#include <vector>
#include <QString>

class Lexer {
public:
    explicit Lexer(const QString& source);

    std::vector<Token> tokenize();

private:
    QString m_source;
    int     m_pos;
    int     m_line;
    int     m_col;

    bool  atEnd() const;
    QChar current() const;
    QChar peekChar(int offset = 1) const;
    QChar advance();

    Token nextToken();
    Token readLineComment();
    Token readMultilineComment();
    Token readString();
    Token readCharLit();
    Token readNumber();
    Token readIdentOrKeyword();

    Token makeToken(TokenType type, const QString& value, int startLine, int startCol);

    static TokenType keywordType(const QString& word);
};
