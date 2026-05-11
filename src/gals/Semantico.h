#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "GalsDef.h"

#include <string>
#include <vector>

class _GALS_CLASS Semantico {
public:
    void executeAction(int action, const Token* token);

    void analyze(const std::vector<Token>& tokens);

    const SymbolTable&               symbolTable() const { return m_table; }
    const std::vector<SemanticError>& errors()     const { return m_errors; }

private:
    SymbolTable                m_table;
    std::vector<SemanticError> m_errors;
};

#endif
