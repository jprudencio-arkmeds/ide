#include "Constants.h"

QString tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::WS:                return "WS";
        case TokenType::COMMENT_LINE:      return "COMMENT_LINE";
        case TokenType::COMMENT_MULTILINE: return "COMMENT_MULTILINE";
        case TokenType::LEFT_BRACKET:      return "'['";
        case TokenType::RIGHT_BRACKET:     return "']'";
        case TokenType::LEFT_PAREN:        return "'('";
        case TokenType::RIGHT_PAREN:       return "')'";
        case TokenType::SEPARATOR:         return "','";
        case TokenType::END_LINE:          return "';'";
        case TokenType::KW_IF:             return "'if'";
        case TokenType::KW_THEN:           return "'{'";
        case TokenType::KW_ELSE:           return "'else'";
        case TokenType::KW_END:            return "'}'";
        case TokenType::KW_INT:            return "'int'";
        case TokenType::KW_FLOAT:          return "'float'";
        case TokenType::KW_CHAR:           return "'char'";
        case TokenType::KW_STRING:         return "'string'";
        case TokenType::KW_VOID:           return "'void'";
        case TokenType::KW_WHILE:          return "'while'";
        case TokenType::KW_FOR:            return "'for'";
        case TokenType::KW_DO:             return "'do'";
        case TokenType::KW_RETURN:         return "'return'";
        case TokenType::KW_BREAK:          return "'break'";
        case TokenType::KW_CONTINUE:       return "'continue'";
        case TokenType::KW_READ:           return "'read'";
        case TokenType::KW_WRITE:          return "'write'";
        case TokenType::PLUS:              return "'+'";
        case TokenType::MINUS:             return "'-'";
        case TokenType::ASSIGN:            return "'='";
        case TokenType::MULTIPLY:          return "'*'";
        case TokenType::DIVIDE:            return "'/'";
        case TokenType::MOD:               return "'%'";
        case TokenType::GREATER_EQUAL:     return "'>='";
        case TokenType::LESS_EQUAL:        return "'<='";
        case TokenType::EQUAL:             return "'=='";
        case TokenType::NOT_EQUAL:         return "'!='";
        case TokenType::GREATER:           return "'>'";
        case TokenType::LESS:              return "'<'";
        case TokenType::AND:               return "'&&'";
        case TokenType::OR:                return "'||'";
        case TokenType::NOT:               return "'!'";
        case TokenType::SHIFT_RIGHT:       return "'>>'";
        case TokenType::SHIFT_LEFT:        return "'<<'";
        case TokenType::BIT_AND:           return "'&'";
        case TokenType::BIT_OR:            return "'|'";
        case TokenType::BIT_XOR:           return "'^'";
        case TokenType::BIT_NOT:           return "'~'";
        case TokenType::LIT_INT:           return "INT";
        case TokenType::LIT_BINARY:        return "BINARY";
        case TokenType::LIT_HEX:           return "HEX";
        case TokenType::LIT_REAL:          return "REAL";
        case TokenType::LIT_CHAR:          return "CHAR_LIT";
        case TokenType::LIT_STRING:        return "STRING_LIT";
        case TokenType::ID:                return "ID";
        case TokenType::END_OF_FILE:       return "EOF";
        case TokenType::UNKNOWN:           return "UNKNOWN";
    }
    return "?";
}

bool isTypeKeyword(TokenType type) {
    return type == TokenType::KW_INT    ||
           type == TokenType::KW_FLOAT  ||
           type == TokenType::KW_CHAR   ||
           type == TokenType::KW_STRING ||
           type == TokenType::KW_VOID;
}
