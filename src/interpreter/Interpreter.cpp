#include "Interpreter.h"
#include <cmath>

void Environment::declareVar(const QString& name, int arraySize) {
    Var v;
    v.isArray = arraySize > 0;
    if (v.isArray) {
        v.arr.resize(arraySize, Value::ofInt(0));
    }
    m_vars[name] = v;
}

Environment::Var* Environment::findVar(const QString& name) {
    auto it = m_vars.find(name);
    if (it != m_vars.end()) return &it.value();
    if (m_parent)            return m_parent->findVar(name);
    return nullptr;
}

const Environment::Var* Environment::findVar(const QString& name) const {
    auto it = m_vars.find(name);
    if (it != m_vars.end()) return &it.value();
    if (m_parent)            return m_parent->findVar(name);
    return nullptr;
}

Value Environment::getVar(const QString& name) const {
    const Var* v = findVar(name);
    return v ? v->val : Value::ofInt(0);
}

void Environment::setVar(const QString& name, const Value& val) {
    Var* v = findVar(name);
    if (v) { v->val = val; return; }
    m_vars[name].val = val;
}

Value Environment::getArray(const QString& name, long long idx) const {
    const Var* v = findVar(name);
    if (v && v->isArray && idx >= 0 && idx < (long long)v->arr.size())
        return v->arr[(int)idx];
    return Value::ofInt(0);
}

void Environment::setArray(const QString& name, long long idx, const Value& val) {
    Var* v = findVar(name);
    if (v && v->isArray && idx >= 0 && idx < (long long)v->arr.size())
        v->arr[(int)idx] = val;
}

Interpreter::Interpreter(OutputFn out, InputFn in)
    : m_out(std::move(out)), m_in(std::move(in))
{}

void Interpreter::checkSteps() {
    if (++m_steps > MAX_STEPS)
        throw RuntimeError{"Execution limit reached (possible infinite loop)"};
}

void Interpreter::run(const std::shared_ptr<ProgramNode>& program) {
    m_funcs.clear();
    m_steps = 0;

    for (const auto& item : program->items) {
        if (auto* fn = dynamic_cast<FuncDefNode*>(item.get()))
            m_funcs[fn->name] = std::dynamic_pointer_cast<FuncDefNode>(item);
    }

    auto globalEnv = Environment::make();
    for (const auto& item : program->items) {
        if (!dynamic_cast<FuncDefNode*>(item.get()))
            exec(item, globalEnv);
    }
}

void Interpreter::execList(const std::vector<StmtPtr>& stmts, Environment::Ptr env) {
    for (const auto& s : stmts)
        exec(s, env);
}

void Interpreter::exec(const StmtPtr& s, Environment::Ptr env) {
    if (!s) return;
    checkSteps();

    if (auto* n = dynamic_cast<VarDeclNode*>  (s.get())) { execVarDecl(n, env);   return; }
    if (auto* n = dynamic_cast<AssignNode*>   (s.get())) { execAssign(n, env);    return; }
    if (auto* n = dynamic_cast<CallStmtNode*> (s.get())) { execCallStmt(n, env);  return; }
    if (auto* n = dynamic_cast<IfNode*>       (s.get())) { execIf(n, env);        return; }
    if (auto* n = dynamic_cast<WhileNode*>    (s.get())) { execWhile(n, env);     return; }
    if (auto* n = dynamic_cast<ForNode*>      (s.get())) { execFor(n, env);       return; }
    if (auto* n = dynamic_cast<DoWhileNode*>  (s.get())) { execDoWhile(n, env);   return; }
    if (auto* n = dynamic_cast<ReadNode*>     (s.get())) { execRead(n, env);      return; }
    if (auto* n = dynamic_cast<WriteNode*>    (s.get())) { execWrite(n, env);     return; }

    if (dynamic_cast<BreakNode*>   (s.get())) throw BreakSignal{};
    if (dynamic_cast<ContinueNode*>(s.get())) throw ContinueSignal{};

    if (auto* n = dynamic_cast<ReturnNode*>(s.get())) {
        Value v = n->value ? eval(n->value, env) : Value::ofVoid();
        throw ReturnSignal{v};
    }
}

void Interpreter::execVarDecl(const VarDeclNode* n, Environment::Ptr env) {
    for (const auto& v : n->vars)
        env->declareVar(v.name, v.arraySize);
}

void Interpreter::execAssign(const AssignNode* n, Environment::Ptr env) {
    Value val = eval(n->value, env);
    if (n->index) {
        long long idx = eval(n->index, env).toInt();
        env->setArray(n->var, idx, val);
    } else {
        env->setVar(n->var, val);
    }
}

void Interpreter::execCallStmt(const CallStmtNode* n, Environment::Ptr env) {
    callFunc(n->func, n->args, env);
}

void Interpreter::execIf(const IfNode* n, Environment::Ptr env) {
    auto child = Environment::make(env);
    if (eval(n->cond, env).toBool())
        execList(n->thenBody, child);
    else if (!n->elseBody.empty())
        execList(n->elseBody, Environment::make(env));
}

void Interpreter::execWhile(const WhileNode* n, Environment::Ptr env) {
    while (eval(n->cond, env).toBool()) {
        checkSteps();
        try {
            execList(n->body, Environment::make(env));
        } catch (BreakSignal&)    { break; }
          catch (ContinueSignal&) { }
    }
}

void Interpreter::execFor(const ForNode* n, Environment::Ptr env) {
    auto loopEnv = Environment::make(env);

    if (!n->initVar.isEmpty() && n->initVal)
        loopEnv->setVar(n->initVar, eval(n->initVal, env));

    while (!n->cond || eval(n->cond, loopEnv).toBool()) {
        checkSteps();
        bool doContinue = false;
        try {
            execList(n->body, Environment::make(loopEnv));
        } catch (BreakSignal&)    { break; }
          catch (ContinueSignal&) { doContinue = true; }
        (void)doContinue;

        if (!n->postVar.isEmpty() && n->postVal)
            loopEnv->setVar(n->postVar, eval(n->postVal, loopEnv));
    }
}

void Interpreter::execDoWhile(const DoWhileNode* n, Environment::Ptr env) {
    do {
        checkSteps();
        try {
            execList(n->body, Environment::make(env));
        } catch (BreakSignal&)    { break; }
          catch (ContinueSignal&) { }
    } while (eval(n->cond, env).toBool());
}

void Interpreter::execRead(const ReadNode* n, Environment::Ptr env) {
    QString input = m_in("Enter value for " + n->var + ": ");
    Value val;
    bool ok;
    long long iv = input.toLongLong(&ok);
    if (ok)               val = Value::ofInt(iv);
    else { double dv = input.toDouble(&ok);
           val = ok ? Value::ofReal(dv) : Value::ofString(input); }

    if (n->index) {
        long long idx = eval(n->index, env).toInt();
        env->setArray(n->var, idx, val);
    } else {
        env->setVar(n->var, val);
    }
}

void Interpreter::execWrite(const WriteNode* n, Environment::Ptr env) {
    QString line;
    for (const auto& a : n->args)
        line += eval(a, env).toString();
    m_out(line);
}

Value Interpreter::callFunc(const QString& name,
                            const std::vector<ExprPtr>& args,
                            Environment::Ptr env) {
    if (!m_funcs.contains(name))
        throw RuntimeError{"Undefined function: " + name};

    auto fn      = m_funcs[name];
    auto funcEnv = Environment::make();

    for (int i = 0; i < (int)fn->params.size() && i < (int)args.size(); ++i) {
        Value val = eval(args[i], env);
        funcEnv->declareVar(fn->params[i].name);
        funcEnv->setVar(fn->params[i].name, val);
    }

    try {
        execList(fn->body, funcEnv);
    } catch (ReturnSignal& r) {
        return r.val;
    }
    return Value::ofVoid();
}

Value Interpreter::eval(const ExprPtr& e, Environment::Ptr env) {
    if (!e) return Value::ofInt(0);
    checkSteps();

    if (auto* n = dynamic_cast<IntLitExpr*>   (e.get())) return Value::ofInt(n->value);
    if (auto* n = dynamic_cast<RealLitExpr*>  (e.get())) return Value::ofReal(n->value);
    if (auto* n = dynamic_cast<StringLitExpr*>(e.get())) return Value::ofString(n->value);
    if (auto* n = dynamic_cast<CharLitExpr*>  (e.get())) return Value::ofChar(n->value);
    if (auto* n = dynamic_cast<VarExpr*>      (e.get())) return env->getVar(n->name);
    if (auto* n = dynamic_cast<ArrayExpr*>    (e.get())) {
        long long idx = eval(n->index, env).toInt();
        return env->getArray(n->name, idx);
    }
    if (auto* n = dynamic_cast<BinOpExpr*>(e.get())) return evalBinOp(n, env);
    if (auto* n = dynamic_cast<UnaryExpr*>(e.get())) return evalUnary(n, env);
    if (auto* n = dynamic_cast<CallExpr*> (e.get())) return callFunc(n->func, n->args, env);

    return Value::ofInt(0);
}

Value Interpreter::evalUnary(const UnaryExpr* e, Environment::Ptr env) {
    Value v = eval(e->operand, env);
    switch (e->op) {
        case TokenType::MINUS:
            return v.type == Value::Type::REAL
                   ? Value::ofReal(-v.realVal)
                   : Value::ofInt(-v.toInt());
        case TokenType::NOT:     return Value::ofInt(!v.toBool() ? 1 : 0);
        case TokenType::BIT_NOT: return Value::ofInt(~v.toInt());
        default:                 return v;
    }
}

Value Interpreter::evalBinOp(const BinOpExpr* e, Environment::Ptr env) {
    Value l = eval(e->left,  env);
    Value r = eval(e->right, env);
    return applyBinOp(e->op, l, r);
}

Value Interpreter::applyBinOp(TokenType op, const Value& l, const Value& r) {
    bool useReal = (l.type == Value::Type::REAL || r.type == Value::Type::REAL);

    switch (op) {
        case TokenType::PLUS:
            if (l.type == Value::Type::STRING || r.type == Value::Type::STRING)
                return Value::ofString(l.toString() + r.toString());
            return useReal ? Value::ofReal(l.toReal() + r.toReal())
                           : Value::ofInt(l.toInt()  + r.toInt());
        case TokenType::MINUS:
            return useReal ? Value::ofReal(l.toReal() - r.toReal())
                           : Value::ofInt(l.toInt()  - r.toInt());
        case TokenType::MULTIPLY:
            return useReal ? Value::ofReal(l.toReal() * r.toReal())
                           : Value::ofInt(l.toInt()  * r.toInt());
        case TokenType::DIVIDE: {
            if (useReal) return Value::ofReal(r.toReal() != 0 ? l.toReal()/r.toReal() : 0);
            long long d = r.toInt();
            return Value::ofInt(d != 0 ? l.toInt() / d : 0);
        }
        case TokenType::MOD:
            return Value::ofInt(r.toInt() != 0 ? l.toInt() % r.toInt() : 0);

        case TokenType::EQUAL:
            return Value::ofInt(useReal ? l.toReal() == r.toReal()
                                        : l.toInt()  == r.toInt() ? 1 : 0);
        case TokenType::NOT_EQUAL:
            return Value::ofInt(useReal ? l.toReal() != r.toReal()
                                        : l.toInt()  != r.toInt() ? 1 : 0);
        case TokenType::GREATER:
            return Value::ofInt(useReal ? l.toReal() >  r.toReal()
                                        : l.toInt()  >  r.toInt() ? 1 : 0);
        case TokenType::LESS:
            return Value::ofInt(useReal ? l.toReal() <  r.toReal()
                                        : l.toInt()  <  r.toInt() ? 1 : 0);
        case TokenType::GREATER_EQUAL:
            return Value::ofInt(useReal ? l.toReal() >= r.toReal()
                                        : l.toInt()  >= r.toInt() ? 1 : 0);
        case TokenType::LESS_EQUAL:
            return Value::ofInt(useReal ? l.toReal() <= r.toReal()
                                        : l.toInt()  <= r.toInt() ? 1 : 0);

        case TokenType::AND: return Value::ofInt(l.toBool() && r.toBool() ? 1 : 0);
        case TokenType::OR:  return Value::ofInt(l.toBool() || r.toBool() ? 1 : 0);

        case TokenType::BIT_AND:    return Value::ofInt(l.toInt() &  r.toInt());
        case TokenType::BIT_OR:     return Value::ofInt(l.toInt() |  r.toInt());
        case TokenType::BIT_XOR:    return Value::ofInt(l.toInt() ^  r.toInt());
        case TokenType::SHIFT_LEFT: return Value::ofInt(l.toInt() << r.toInt());
        case TokenType::SHIFT_RIGHT:return Value::ofInt(l.toInt() >> r.toInt());

        default: return Value::ofInt(0);
    }
}
