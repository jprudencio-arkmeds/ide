#ifndef SINTATICO_H
#define SINTATICO_H

#include "Lexico.h"
#include "Semantico.h"
#include "SyntacticError.h"
#include "AST.h"
#include <vector>
#include <memory>
#include <QString>

struct ParseError {
    int     line, col;
    QString message;
};

// Recursive-descent LL(1) parser.
// parse() tokenises via Lexico, builds an AST and stores any errors.
// The Semantico parameter is accepted for GALS interface compatibility
// but is not used — semantic actions are embedded in the parser.

class Sintatico {
public:
    Sintatico() = default;

    void parse(Lexico* scanner, Semantico* semanticAnalyser);

    std::shared_ptr<ProgramNode>   ast()       const { return m_ast; }
    const std::vector<ParseError>& errors()    const { return m_errors; }
    bool                           hasErrors() const { return !m_errors.empty(); }

private:
    std::vector<Token>           m_tokens;
    size_t                       m_pos = 0;
    std::vector<ParseError>      m_errors;
    std::shared_ptr<ProgramNode> m_ast;

    static const Token& eofToken();

    const Token& current()             const;
    const Token& peek(int offset)      const;
    Token        consume();
    bool         check(TokenType type) const;
    bool         isType()              const;
    bool         atEnd()               const;
    bool         expect(TokenType type, const QString& ctx = "");
    void         synchronize();
    void         addError(const QString& msg);

    std::shared_ptr<ProgramNode> parseProgram();
    std::vector<StmtPtr>         parseItemList();
    StmtPtr                      parseItem();
    StmtPtr                      parseItemSuffix(const Token& name, const QString& typeName);

    std::vector<VarEntry>        parseMoreDeclIds();
    VarEntry                     parseDeclVar();

    std::vector<FuncParam>       parseParamListOpt();
    std::vector<FuncParam>       parseParamList();
    FuncParam                    parseParam();

    std::vector<StmtPtr>         parseStmtList();
    StmtPtr                      parseStatement();
    StmtPtr                      parseAssignOrCall();
    StmtPtr                      parseAssignOrCallTail(const Token& id);

    StmtPtr                      parseIfStmt();
    std::vector<StmtPtr>         parseElsePart();
    StmtPtr                      parseWhileStmt();
    StmtPtr                      parseForStmt();
    std::pair<QString,ExprPtr>   parseForInit();
    std::pair<QString,ExprPtr>   parseForPost();
    StmtPtr                      parseDoWhileStmt();

    StmtPtr                      parseReadStmt();
    StmtPtr                      parseWriteStmt();
    std::vector<ExprPtr>         parseWriteArgList();

    StmtPtr                      parseReturnStmt();
    StmtPtr                      parseBreakStmt();
    StmtPtr                      parseContinueStmt();

    std::vector<ExprPtr>         parseArgListOpt();
    std::vector<ExprPtr>         parseArgList();

    ExprPtr parseExpr();
    ExprPtr parseExprOrTail (ExprPtr left);
    ExprPtr parseExprAnd();
    ExprPtr parseExprAndTail(ExprPtr left);
    ExprPtr parseExprEquality();
    ExprPtr parseExprEqualityTail(ExprPtr left);
    ExprPtr parseExprBitOr();
    ExprPtr parseExprBitOrTail (ExprPtr left);
    ExprPtr parseExprBitXor();
    ExprPtr parseExprBitXorTail(ExprPtr left);
    ExprPtr parseExprBitAnd();
    ExprPtr parseExprBitAndTail(ExprPtr left);
    ExprPtr parseExprRelStrict();
    ExprPtr parseExprRelStrictTail(ExprPtr left);
    ExprPtr parseExprShift();
    ExprPtr parseExprShiftTail(ExprPtr left);
    ExprPtr parseExprAdd();
    ExprPtr parseExprAddTail(ExprPtr left);
    ExprPtr parseExprMul();
    ExprPtr parseExprMulTail(ExprPtr left);
    ExprPtr parseExprUnary();
    ExprPtr parseExprPrimary();
    ExprPtr parseIdExprTail (const Token& id);
};

#endif
