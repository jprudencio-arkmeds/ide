#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "AssemblyGenerator.h"
#include "GalsDef.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Token.h"

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
    std::string getAssembly() const { return m_assemblyGen.getAssembly(); }

private:
    std::shared_ptr<Symbol> lookupSymbol(const std::string& lexeme, const int position);

    bool isAssign       (const TokenId& tokId) const;
    bool isDefaultAssign(const TokenId& tokId) const;
    bool isUnaryOperator(const TokenId& tokId) const;

    void trackExprType(const std::string& type, int position);
    int checkUseOfUninitializedInParams(const std::vector<Token>& tokens, int index);
    void checkUseOfUninitialized(const std::shared_ptr<Symbol>& symbol, const Token& tok);
    void unaryCompatibilityCheck(const std::shared_ptr<Symbol>& symbol);

    void appendAssemblyData(Symbol* symbol);

    int getVectorIndex(const std::vector<Token>& tokens, size_t& index) const;

    SymbolTable                  m_table;
    std::vector<SemanticError>   m_errors;
    std::vector<SemanticWarning> m_warnings;
    std::stack<std::shared_ptr<Symbol>> m_operatingVars;
    std::string m_exprLeftType;
    TokenId     m_exprOp = EPSILON;

    AssemblyGenerator m_assemblyGen;
};

#endif
