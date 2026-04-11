#pragma once
#include <QString>

enum class TokenType {
    WS,
    COMMENT_LINE,
    COMMENT_MULTILINE,

    LEFT_BRACKET,
    RIGHT_BRACKET,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    SEPARATOR,
    END_LINE,

    KW_IF,
    KW_THEN,
    KW_ELSE,
    KW_END,
    KW_INT,
    KW_FLOAT,
    KW_CHAR,
    KW_STRING,
    KW_VOID,
    KW_WHILE,
    KW_FOR,
    KW_DO,
    KW_RETURN,
    KW_BREAK,
    KW_CONTINUE,
    KW_READ,
    KW_WRITE,

    PLUS,
    MINUS,
    ASSIGN,
    MULTIPLY,
    DIVIDE,
    MOD,

    GREATER_EQUAL,
    LESS_EQUAL,
    EQUAL,
    NOT_EQUAL,
    GREATER,
    LESS,

    AND,
    OR,
    NOT,

    SHIFT_RIGHT,
    SHIFT_LEFT,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,

    LIT_INT,
    LIT_BINARY,
    LIT_HEX,
    LIT_REAL,
    LIT_CHAR,
    LIT_STRING,

    ID,

    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    QString   value;
    int       line;
    int       col;

    Token() : type(TokenType::UNKNOWN), value(""), line(0), col(0) {}
    Token(TokenType t, const QString& v, int l, int c)
        : type(t), value(v), line(l), col(c) {}
};

QString tokenTypeName(TokenType type);
bool    isTypeKeyword(TokenType type);
