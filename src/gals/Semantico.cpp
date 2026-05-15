#include "Semantico.h"
#include "TypeOperationsTable.h"

static bool isTypeBase(TokenId id) {
    return id == t_KEY_INT    || id == t_KEY_FLOAT  || id == t_KEY_DOUBLE ||
           id == t_KEY_CHAR   || id == t_KEY_VOID   || id == t_KEY_STRING;
}

static bool isModifier(TokenId id) {
    return id == t_KEY_CONST    || id == t_KEY_STATIC   || id == t_KEY_EXTERN  ||
           id == t_KEY_REGISTER || id == t_KEY_VOLATILE  || id == t_KEY_AUTO   ||
           id == t_KEY_SIGNED   || id == t_KEY_UNSIGNED  || id == t_KEY_SHORT  ||
           id == t_KEY_LONG;
}


void Semantico::executeAction(int /*action*/, const Token* /*token*/) {
}

enum State { IDLE, IN_TYPE, IN_ASSIGN };

State       state = IDLE;
bool        isDeclaring = false;
std::string pendingType;
TokenId     prevId = EPSILON;
bool        skipNextBrace = false;

Operator lastOperator = ASSIGN;

void Semantico::analyze(const std::vector<Token>& tokens) {

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& tok = tokens[i];
        TokenId      id  = tok.getId();

        if (id == DOLLAR) break;

        switch (state) {

        case IDLE:
            if (isModifier(id) || isTypeBase(id)) {
                pendingType = tok.getLexeme();
                state = IN_TYPE;

            } else if (id == t_KEY_LEFT_BRACE) {
                if (skipNextBrace)
                    skipNextBrace = false;
                else
                    m_table.enterScope();

            } else if (id == t_KEY_RIGHT_BRACE) {
                m_table.exitScope();

            } else if (id == t_ID) {
                // Acesso a membro (obj.campo / obj->campo) não é uso de variável.
                if (prevId != t_KEY_DOT && prevId != t_KEY_ARROW) {
                    std::shared_ptr<Symbol> symbol = lookupSymbol(tok.getLexeme(), tok.getPosition());
                    if (symbol) {
                        m_operatingVars.push(symbol);

                        const TokenId nextId = (i + 1 < tokens.size()) ? tokens[i + 1].getId() : EPSILON;

                        if (isDefaultAssign(nextId)) {
                          state = IN_ASSIGN;
                        }
                        else if (isAssign(nextId)) {
                          m_errors.emplace_back(
                            "Não é possível realizar operações compostas na variável '" + tok.getLexeme() + "' em sua declaração.",
                            tok.getPosition());

                          pendingType.clear();
                        }
                        else {
                          state = IDLE;
                          pendingType.clear();
                        }
                    }
                }
            }
            break;

        case IN_TYPE:
            isDeclaring = true;
            if (isModifier(id) || isTypeBase(id)) {
                pendingType += " " + tok.getLexeme();
            } else if (id == t_KEY_MULTIPLY) {
                pendingType += "*";
            } else if (id == t_ID) {
                bool isFunction = tokens[i + 1].getId() == t_KEY_LEFT_PARENTHESIS;

                std::shared_ptr<Symbol> symbol = m_table.addSymbol(tok.getLexeme(), pendingType, isFunction, tok.getPosition());
                if (!symbol) {
                    m_errors.emplace_back(
                        "Função ou variável '" + tok.getLexeme() + "' já declarada neste escopo.",
                        tok.getPosition());
                }

                symbol = symbol.get() == nullptr ? lookupSymbol(tok.getLexeme(), tok.getPosition()) : symbol;

                if (isFunction) {
                    m_table.enterScope();
                    skipNextBrace = true;
                }

                const TokenId nextId = (i + 1 < tokens.size()) ? tokens[i + 1].getId() : EPSILON;

                if (isDefaultAssign(nextId)) {
                    m_operatingVars.push(symbol);
                    state = IN_ASSIGN;
                }
                else {
                    state = IDLE;
                    pendingType.clear();
                }

            } else if (id == t_KEY_LEFT_BRACE) {
                if (skipNextBrace)
                    skipNextBrace = false;
                else
                    m_table.enterScope();
                pendingType.clear();
                state = IDLE;

            } else if (id == t_KEY_RIGHT_BRACE) {
                m_table.exitScope();
                pendingType.clear();
                state = IDLE;

            } else {
                pendingType.clear();
                state = IDLE;
            }
            break;
        case IN_ASSIGN:
            if (isAssign(id) || isUnaryOperator(id)) {
                lastOperator = static_cast<Operator>(id);
                break;
            }

            Symbol* lastSymbol = m_operatingVars.top().get();

            if (id == t_ID) {
                if (auto& symbol = lookupSymbol(tok.getLexeme(), tok.getPosition())) {
                    if (!TypeOperationsTable::isCompatible(symbol->type, pendingType, lastOperator)) {
                        m_errors.emplace_back(
                            "Tipo incompatível na atribuição à variável '" + tok.getLexeme() + "'.",
                            tok.getPosition());
                    }

                    if (!symbol->isFunction && !symbol->isInitialized) {
                        m_warnings.emplace_back(
                            "Uso de variável '" + tok.getLexeme() + "' sem inicialização. Possível uso de lixo de memória.",
                            tok.getPosition());
                    }
                }
                else {
                    m_errors.emplace_back(
                        "Variável ou função '" + tok.getLexeme() + "' não declarada.",
                        tok.getPosition());
                }
            }
            else {
                std::string literalType;
                switch (id) {
                  case t_INT:
                  case t_BINARY:
                  case t_HEX:    literalType = "int"; break;
                  case t_REAL:   literalType = "float"; break;
                  case t_CHAR:   literalType = "char"; break;
                  case t_STRING: literalType = "string"; break;
                  default:       literalType = ""; break;
                }
                if (!TypeOperationsTable::isCompatible(literalType, pendingType, lastOperator)) {
                    Symbol* lastSymbol = m_operatingVars.top().get();
                    m_errors.emplace_back(
                        "Tipo incompatível na atribuição à variável '" + lastSymbol->type + "'.",
                        tok.getPosition());
                    pendingType.clear();
                }
            }

            if (isDeclaring) {
              m_operatingVars.top()->isInitialized = true;
              isDeclaring = false;
            }

            m_operatingVars.pop();
            state = IDLE;
            break;
        }

        prevId = id;
    }
}

std::shared_ptr<Symbol> Semantico::lookupSymbol(const std::string& lexeme, const int position) {
    std::shared_ptr<Symbol> symbol = m_table.lookupSymbol(lexeme);
    if (!symbol) {
        m_errors.emplace_back(
            "Variável '" + lexeme + "' não declarada.",
            position);
    }
    return symbol;
}

bool Semantico::isAssign(const TokenId& tokId) const {
    return isDefaultAssign(tokId) || tokId == t_KEY_PLUS_ASSIGN || tokId == t_KEY_MINUS_ASSIGN ||
        tokId == t_KEY_MULTIPLY_ASSIGN || tokId == t_KEY_DIVIDE_ASSIGN ||
        tokId == t_KEY_MOD_ASSIGN || tokId == t_KEY_AND_ASSIGN ||
        tokId == t_KEY_OR_ASSIGN || tokId == t_KEY_XOR_ASSIGN ||
        tokId == t_KEY_SHIFT_LEFT_ASSIGN || tokId == t_KEY_SHIFT_RIGHT_ASSIGN;
}

bool Semantico::isDefaultAssign(const TokenId& tokId) const {
    return tokId == t_KEY_ASSIGN;
}

bool Semantico::isUnaryOperator(const TokenId& tokId) const {
    return tokId == t_KEY_INCREMENT || tokId == t_KEY_DECREMENT;
}