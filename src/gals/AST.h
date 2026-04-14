#pragma once
#include "Constants.h"
#include <memory>
#include <vector>
#include <QString>

struct Stmt; struct Expr;
using StmtPtr = std::shared_ptr<Stmt>;
using ExprPtr = std::shared_ptr<Expr>;

struct Stmt { virtual ~Stmt() = default; };
struct Expr { virtual ~Expr() = default; };

struct ProgramNode : Stmt {
    std::vector<StmtPtr> items;
};

struct FuncParam {
    QString type, name;
    bool    isArray = false;
};

struct FuncDefNode : Stmt {
    QString                retType;
    QString                name;
    std::vector<FuncParam> params;
    std::vector<StmtPtr>   body;
};

struct VarEntry {
    QString name;
    int     arraySize = 0;
};

struct VarDeclNode : Stmt {
    QString               type;
    std::vector<VarEntry> vars;
};

struct AssignNode : Stmt {
    QString var;
    ExprPtr index;
    ExprPtr value;
};

struct CallStmtNode : Stmt {
    QString              func;
    std::vector<ExprPtr> args;
};

struct IfNode : Stmt {
    ExprPtr              cond;
    std::vector<StmtPtr> thenBody;
    std::vector<StmtPtr> elseBody;
};

struct WhileNode : Stmt {
    ExprPtr              cond;
    std::vector<StmtPtr> body;
};

struct ForNode : Stmt {
    QString              initVar;
    ExprPtr              initVal;
    ExprPtr              cond;
    QString              postVar;
    ExprPtr              postVal;
    std::vector<StmtPtr> body;
};

struct DoWhileNode : Stmt {
    std::vector<StmtPtr> body;
    ExprPtr              cond;
};

struct ReadNode : Stmt {
    QString var;
    ExprPtr index;
};

struct WriteNode : Stmt {
    std::vector<ExprPtr> args;
};

struct ReturnNode : Stmt {
    ExprPtr value;
};

struct BreakNode    : Stmt {};
struct ContinueNode : Stmt {};

struct BinOpExpr : Expr {
    TokenType op;
    ExprPtr   left, right;
};

struct UnaryExpr : Expr {
    TokenType op;
    ExprPtr   operand;
};

struct IntLitExpr    : Expr { long long value = 0; };
struct RealLitExpr   : Expr { double    value = 0; };
struct StringLitExpr : Expr { QString   value;     };
struct CharLitExpr   : Expr { QChar     value;     };

struct VarExpr : Expr {
    QString name;
};

struct ArrayExpr : Expr {
    QString name;
    ExprPtr index;
};

struct CallExpr : Expr {
    QString              func;
    std::vector<ExprPtr> args;
};
