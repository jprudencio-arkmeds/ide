#pragma once
#include "lexer/Token.h"
#include "interpreter/AST.h"
#include <vector>
#include <QString>
#include <memory>

struct ParseError {
    int     line, col;
    QString message;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    bool parse();

    const std::vector<ParseError>&      errors()    const { return m_errors; }
    bool                                hasErrors() const { return !m_errors.empty(); }
    std::shared_ptr<ProgramNode>        ast()       const { return m_ast; }

private:
    std::vector<Token>           m_tokens;
    size_t                       m_pos;
    std::vector<ParseError>      m_errors;
    std::shared_ptr<ProgramNode> m_ast;

    const Token& current()             const;
    const Token& peekToken(int offset) const;
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
    ExprPtr parseExprNot();
    ExprPtr parseExprRel();
    ExprPtr parseRelTail    (ExprPtr left);
    ExprPtr parseExprAdd();
    ExprPtr parseExprAddTail(ExprPtr left);
    ExprPtr parseExprMul();
    ExprPtr parseExprMulTail(ExprPtr left);
    ExprPtr parseExprUnary();
    ExprPtr parseExprPrimary();
    ExprPtr parseIdExprTail (const Token& id);
};
