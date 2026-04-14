#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

// Full token type enum — used by Lexico, Sintatico, AST and Interpreter.
enum class TokenType {
    WS,
    COMMENT_LINE,
    COMMENT_MULTILINE,

    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    SEPARATOR,      // ,
    END_LINE,       // ;

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

// GALS alias — keeps the Semantico/Token GALS interface compiling.
using TokenId = TokenType;

QString tokenTypeName(TokenType type);
bool    isTypeKeyword(TokenType type);

#endif
