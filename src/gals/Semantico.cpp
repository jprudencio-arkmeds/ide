#include "Semantico.h"
#include "Constants.h"

void Semantico::reset() {
    m_table.reset();
    m_errors.clear();
    m_pendingType.clear();
}

void Semantico::executeAction(int action, const Token* token) {
    switch (action) {

        // Registra o tipo da declaração em andamento.
        // Ativado ao reconhecer uma palavra-chave de tipo (int, float, …).
        case SA_SET_TYPE:
            m_pendingType = token ? token->getLexeme() : "";
            break;

        // Declara variável no escopo corrente com o tipo acumulado.
        // Ativado ao reconhecer o ID após um tipo em declaracao_variavel.
        case SA_DECLARE_VAR:
            if (token) {
                const std::string& name = token->getLexeme();
                if (!m_table.addSymbol(name, m_pendingType, token->getPosition())) {
                    m_errors.emplace_back(
                        "Variável '" + name + "' já declarada neste escopo.",
                        token->getPosition()
                    );
                }
                m_pendingType.clear();
            }
            break;

        // Declara função no escopo corrente (sempre no escopo global na prática).
        // Ativado ao reconhecer o ID em declaracao_funcao.
        case SA_DECLARE_FUN:
            if (token) {
                const std::string& name = token->getLexeme();
                std::string funType = "fun:" + m_pendingType;
                if (!m_table.addSymbol(name, funType, token->getPosition())) {
                    m_errors.emplace_back(
                        "Função '" + name + "' já declarada neste escopo.",
                        token->getPosition()
                    );
                }
                m_pendingType.clear();
            }
            break;

        // Abre novo escopo — ativado ao reconhecer '{'.
        case SA_ENTER_SCOPE:
            m_table.enterScope();
            break;

        // Fecha escopo corrente — ativado ao reconhecer '}'.
        case SA_EXIT_SCOPE:
            m_table.exitScope();
            break;

        // Verifica uso de variável em expressão.
        // Ativado ao reconhecer ID em expressao_primaria.
        case SA_USE_VAR:
            if (token) {
                const std::string& name = token->getLexeme();
                if (!m_table.lookupSymbol(name)) {
                    m_errors.emplace_back(
                        "Variável '" + name + "' não declarada.",
                        token->getPosition()
                    );
                }
            }
            break;

        default:
            break;
    }
}
