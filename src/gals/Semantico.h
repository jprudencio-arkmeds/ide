#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"

#include <string>
#include <vector>

/**
 * IDs das ações semânticas disparadas pelo parser (cmd[1] em Sintatico.cpp).
 * Para ativá-las, embutir os #N correspondentes nas regras de exp.gals e
 * regenerar as tabelas com GALS.
 */
enum SemanticAction {
    SA_SET_TYPE    = 1, // token anterior é palavra-chave de tipo
    SA_DECLARE_VAR = 2, // token anterior é ID após declaração de tipo
    SA_DECLARE_FUN = 3, // token anterior é ID de função
    SA_ENTER_SCOPE = 4, // token anterior é '{'
    SA_EXIT_SCOPE  = 5, // token anterior é '}'
    SA_USE_VAR     = 6, // token anterior é ID em contexto de expressão
};

class Semantico {
public:
    /**
     * Ponto de entrada do analisador semântico.
     * Chamado pelo parser para cada ação semântica encontrada.
     */
    void executeAction(int action, const Token* token);

    /** Acesso somente-leitura à tabela de símbolos. */
    const SymbolTable& symbolTable() const { return m_table; }

    /** Erros semânticos acumulados durante a análise. */
    const std::vector<SemanticError>& errors() const { return m_errors; }

    /** Reinicia estado para nova compilação. */
    void reset();

private:
    SymbolTable              m_table;
    std::vector<SemanticError> m_errors;
    std::string              m_pendingType; // tipo da declaração em curso
};

#endif
