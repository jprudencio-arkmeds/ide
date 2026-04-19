#ifndef CONSTANTS_H
#define CONSTANTS_H

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_KEY_LEFT_BRACKET = 2,
    t_KEY_RIGHT_BRACKET = 3,
    t_KEY_LEFT_PARENTHESIS = 4,
    t_KEY_RIGHT_PARENTHESIS = 5,
    t_KEY_LEFT_BRACE = 6,
    t_KEY_RIGHT_BRACE = 7,
    t_SEPARATOR = 8,
    t_END_LINE = 9,
    t_COLON = 10,
    t_QUESTION = 11,
    t_KEY_DOT = 12,
    t_KEY_ARROW = 13,
    t_KEY_IF = 14,
    t_KEY_ELSE = 15,
    t_KEY_WHILE = 16,
    t_KEY_FOR = 17,
    t_KEY_DO = 18,
    t_KEY_SWITCH = 19,
    t_KEY_CASE = 20,
    t_KEY_DEFAULT = 21,
    t_KEY_BREAK = 22,
    t_KEY_CONTINUE = 23,
    t_KEY_RETURN = 24,
    t_KEY_GOTO = 25,
    t_KEY_SIZEOF = 26,
    t_KEY_TYPEDEF = 27,
    t_KEY_STRUCT = 28,
    t_KEY_UNION = 29,
    t_KEY_ENUM = 30,
    t_KEY_CONST = 31,
    t_KEY_STATIC = 32,
    t_KEY_EXTERN = 33,
    t_KEY_REGISTER = 34,
    t_KEY_VOLATILE = 35,
    t_KEY_AUTO = 36,
    t_KEY_SIGNED = 37,
    t_KEY_UNSIGNED = 38,
    t_KEY_SHORT = 39,
    t_KEY_LONG = 40,
    t_KEY_INT = 41,
    t_KEY_FLOAT = 42,
    t_KEY_DOUBLE = 43,
    t_KEY_CHAR = 44,
    t_KEY_VOID = 45,
    t_KEY_STRING = 46,
    t_KEY_INCREMENT = 47,
    t_KEY_DECREMENT = 48,
    t_KEY_PLUS = 49,
    t_KEY_MINUS = 50,
    t_KEY_MULTIPLY = 51,
    t_KEY_DIVIDE = 52,
    t_KEY_MOD = 53,
    t_KEY_ASSIGN = 54,
    t_KEY_PLUS_ASSIGN = 55,
    t_KEY_MINUS_ASSIGN = 56,
    t_KEY_MULTIPLY_ASSIGN = 57,
    t_KEY_DIVIDE_ASSIGN = 58,
    t_KEY_MOD_ASSIGN = 59,
    t_KEY_AND_ASSIGN = 60,
    t_KEY_OR_ASSIGN = 61,
    t_KEY_XOR_ASSIGN = 62,
    t_KEY_SHIFT_LEFT_ASSIGN = 63,
    t_KEY_SHIFT_RIGHT_ASSIGN = 64,
    t_KEY_GREATER_EQUAL = 65,
    t_KEY_LESS_EQUAL = 66,
    t_KEY_EQUAL = 67,
    t_KEY_NOT_EQUAL = 68,
    t_KEY_GREATER = 69,
    t_KEY_LESS = 70,
    t_KEY_AND = 71,
    t_KEY_OR = 72,
    t_KEY_NOT = 73,
    t_KEY_SHIFT_RIGHT = 74,
    t_KEY_SHIFT_LEFT = 75,
    t_KEY_BIT_AND = 76,
    t_KEY_BIT_OR = 77,
    t_KEY_BIT_XOR = 78,
    t_KEY_BIT_NOT = 79,
    t_INT = 80,
    t_BINARY = 81,
    t_HEX = 82,
    t_REAL = 83,
    t_CHAR = 84,
    t_STRING = 85,
    t_ID = 86
};

const int STATES_COUNT = 210;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 144;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[243][144][2];

extern const int PRODUCTIONS[157][2];

extern const char *PARSER_ERROR[243];

#endif
