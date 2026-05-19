#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "GalsDef.h"

#include <string>
#include <stack>
#include <vector>

class _GALS_CLASS Semantico {
public:
    void executeAction(int action, const Token* token);

    void analyze(const std::vector<Token>& tokens);

    const SymbolTable&                  symbolTable() const { return m_table; }
    const std::vector<SemanticError>&   errors()      const { return m_errors; }
    const std::vector<SemanticWarning>& warnings()    const { return m_warnings; }

private:
    std::shared_ptr<Symbol> lookupSymbol(const std::string& lexeme, const int position);

    bool isTypeStart(const TokenId& tokId) const;

    bool isTypeToken(const TokenId& tokId) const;

    bool isBuiltinFunction(const std::string& lexeme) const;

    bool isAssign(const TokenId& tokId) const;

    bool isDefaultAssign(const TokenId& tokId) const;

    bool isUnaryOperator(const TokenId& tokId) const;

    bool isLiteral(const TokenId& tokId) const;

    std::string literalType(const TokenId& tokId) const;

    SymbolTable                  m_table;
    std::vector<SemanticError>   m_errors;
    std::vector<SemanticWarning> m_warnings;
    std::stack<std::shared_ptr<Symbol>> m_operatingVars;
};

#endif
