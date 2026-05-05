#include "Semantico.h"
#include "Constants.h"

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

void Semantico::analyze(const std::vector<Token>& tokens) {
    enum State { IDLE, IN_TYPE };

    State       state         = IDLE;
    std::string pendingType;
    TokenId     prevId        = EPSILON;
    bool        skipNextBrace = false;

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
                    if (!m_table.lookupSymbol(tok.getLexeme())) {
                        m_errors.emplace_back(
                            "Variável '" + tok.getLexeme() + "' não declarada.",
                            tok.getPosition());
                    }
                }
            }
            break;

        case IN_TYPE:
            if (isModifier(id) || isTypeBase(id)) {
                pendingType += " " + tok.getLexeme();
            } else if (id == t_KEY_MULTIPLY) {
                pendingType += "*";
            } else if (id == t_ID) {
                bool isFunction = (i + 1 < tokens.size() &&
                                   tokens[i + 1].getId() == t_KEY_LEFT_PARENTHESIS);

                std::string symType = isFunction ? ("fun:" + pendingType) : pendingType;

                if (!m_table.addSymbol(tok.getLexeme(), symType, tok.getPosition())) {
                    std::string kind = isFunction ? "Função" : "Variável";
                    m_errors.emplace_back(
                        kind + " '" + tok.getLexeme() + "' já declarada neste escopo.",
                        tok.getPosition());
                }

                if (isFunction) {
                    m_table.enterScope();
                    skipNextBrace = true;
                }

                pendingType.clear();
                state = IDLE;

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
        }

        prevId = id;
    }
}
