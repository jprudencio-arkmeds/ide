#ifndef TOKEN_H
#define TOKEN_H

#include "Constants.h"
#include <QString>
#include <string>

// Single Token class that satisfies both the hand-written parser interface
// (type / value / line / col) and the original GALS interface
// (getId() / getLexeme() / getPosition()).

class Token {
public:
    Token()
        : type(TokenType::UNKNOWN), value(""), line(0), col(0), position(0) {}

    Token(TokenType t, const QString& v, int l, int c, int pos = 0)
        : type(t), value(v), line(l), col(c), position(pos) {}

    // ── Parser / old-lexer interface ─────────────────
    TokenType type;
    QString   value;
    int       line;
    int       col;
    int       position;   // byte offset (GALS compat)

    // ── GALS interface ────────────────────────────────
    TokenId     getId()      const { return type; }
    std::string getLexeme()  const { return value.toStdString(); }
    int         getPosition()const { return position; }
};

#endif
