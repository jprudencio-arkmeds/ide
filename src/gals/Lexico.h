#ifndef LEXICO_H
#define LEXICO_H

#include "Token.h"
#include "LexicalError.h"
#include <QString>
#include <vector>

// Hand-written lexer.  Implements the original GALS nextToken() interface
// and also exposes tokenize() for use by Sintatico.

class Lexico {
public:
    explicit Lexico(const char* input = "") { setInput(input); }
    explicit Lexico(const QString& input)   { setInput(input); }

    void setInput(const char* input);
    void setInput(const QString& input);

    // GALS interface: returns heap-allocated Token* or nullptr at EOF.
    // Caller owns the pointer.
    Token* nextToken();

    // Convenience: tokenize the whole input, skip WS and comments,
    // append END_OF_FILE.  Returns Token values (not pointers).
    std::vector<Token> tokenize();

private:
    QString m_source;
    int     m_pos  = 0;
    int     m_line = 1;
    int     m_col  = 1;

    bool  atEnd()                    const;
    QChar current()                  const;
    QChar peekChar(int offset = 1)   const;
    QChar advance();

    Token nextTokenInternal();
    Token readLineComment();
    Token readMultilineComment();
    Token readString();
    Token readCharLit();
    Token readNumber();
    Token readIdentOrKeyword();

    Token makeToken(TokenType type, const QString& value,
                    int startLine, int startCol) const;

    static TokenType keywordType(const QString& word);
};

#endif
