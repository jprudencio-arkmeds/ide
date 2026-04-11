#pragma once
#include "AST.h"
#include "Value.h"
#include <functional>
#include <QMap>
#include <memory>
#include <stdexcept>

struct BreakSignal    {};
struct ContinueSignal {};
struct ReturnSignal   { Value val; };
struct RuntimeError   { QString msg; };

class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;
    static Ptr make(Ptr parent = nullptr) {
        return std::make_shared<Environment>(parent);
    }
    explicit Environment(Ptr parent = nullptr) : m_parent(parent) {}

    void  declareVar(const QString& name, int arraySize = 0);
    Value getVar    (const QString& name) const;
    void  setVar    (const QString& name, const Value& val);
    Value getArray  (const QString& name, long long idx) const;
    void  setArray  (const QString& name, long long idx, const Value& val);

private:
    struct Var {
        Value              val;
        std::vector<Value> arr;
        bool               isArray = false;
    };
    QMap<QString, Var> m_vars;
    Ptr                m_parent;

    Var* findVar(const QString& name);
    const Var* findVar(const QString& name) const;
};

class Interpreter {
public:
    using OutputFn = std::function<void(const QString&)>;
    using InputFn  = std::function<QString(const QString&)>;

    Interpreter(OutputFn out, InputFn in);

    void run(const std::shared_ptr<ProgramNode>& program);

private:
    OutputFn m_out;
    InputFn  m_in;

    QMap<QString, std::shared_ptr<FuncDefNode>> m_funcs;
    int m_steps = 0;
    static constexpr int MAX_STEPS = 500'000;

    void  checkSteps();

    void  execList (const std::vector<StmtPtr>& stmts, Environment::Ptr env);
    void  exec     (const StmtPtr& s, Environment::Ptr env);
    Value eval     (const ExprPtr& e, Environment::Ptr env);

    void execVarDecl   (const VarDeclNode*   n, Environment::Ptr env);
    void execAssign    (const AssignNode*     n, Environment::Ptr env);
    void execCallStmt  (const CallStmtNode*   n, Environment::Ptr env);
    void execIf        (const IfNode*         n, Environment::Ptr env);
    void execWhile     (const WhileNode*      n, Environment::Ptr env);
    void execFor       (const ForNode*        n, Environment::Ptr env);
    void execDoWhile   (const DoWhileNode*    n, Environment::Ptr env);
    void execRead      (const ReadNode*       n, Environment::Ptr env);
    void execWrite     (const WriteNode*      n, Environment::Ptr env);

    Value evalBinOp (const BinOpExpr*  e, Environment::Ptr env);
    Value evalUnary (const UnaryExpr*  e, Environment::Ptr env);
    Value callFunc  (const QString& name, const std::vector<ExprPtr>& args, Environment::Ptr env);

    static Value applyBinOp(TokenType op, const Value& l, const Value& r);
};
