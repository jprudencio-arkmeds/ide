#include "Sintatico.h"

// ── Bootstrap ───────────────────────────────────────────────────────────────

void Sintatico::parse(Lexico* scanner, Semantico* /*semanticAnalyser*/) {
    m_tokens.clear();
    m_errors.clear();
    m_pos = 0;

    // Collect all tokens from the lexer.
    m_tokens = scanner->tokenize();

    m_ast = parseProgram();
}

// ── Helpers ──────────────────────────────────────────────────────────────────

const Token& Sintatico::eofToken() {
    static const Token eof(TokenType::END_OF_FILE, "", 0, 0);
    return eof;
}

const Token& Sintatico::current() const {
    if (m_pos >= m_tokens.size()) return eofToken();
    return m_tokens[m_pos];
}

const Token& Sintatico::peek(int offset) const {
    size_t idx = m_pos + static_cast<size_t>(offset);
    if (idx >= m_tokens.size()) return eofToken();
    return m_tokens[idx];
}

Token Sintatico::consume() {
    Token t = current();
    if (m_pos < m_tokens.size()) m_pos++;
    return t;
}

bool Sintatico::check(TokenType type) const { return current().type == type; }
bool Sintatico::isType()              const { return isTypeKeyword(current().type); }
bool Sintatico::atEnd()               const { return check(TokenType::END_OF_FILE); }

void Sintatico::addError(const QString& msg) {
    if (!m_errors.empty() &&
        m_errors.back().line == current().line &&
        m_errors.back().col  == current().col) return;
    m_errors.push_back({current().line, current().col, msg});
}

bool Sintatico::expect(TokenType type, const QString& ctx) {
    if (check(type)) { consume(); return true; }
    QString where = ctx.isEmpty() ? "" : " in " + ctx;
    QString found = current().value.isEmpty()
                        ? tokenTypeName(current().type)
                        : "'" + current().value + "'";
    addError(QString("Expected %1%2, found %3")
             .arg(tokenTypeName(type)).arg(where).arg(found));
    return false;
}

void Sintatico::synchronize() {
    while (!atEnd()) {
        if (check(TokenType::END_LINE)) { consume(); return; }
        if (check(TokenType::KW_END) || check(TokenType::KW_ELSE)) return;
        consume();
    }
}

// ── Program ──────────────────────────────────────────────────────────────────

std::shared_ptr<ProgramNode> Sintatico::parseProgram() {
    auto node  = std::make_shared<ProgramNode>();
    node->items = parseItemList();
    if (!atEnd())
        addError("Unexpected token '" + current().value + "' at end of program");
    return node;
}

std::vector<StmtPtr> Sintatico::parseItemList() {
    std::vector<StmtPtr> items;
    while (!atEnd()) {
        StmtPtr s = parseItem();
        if (s) items.push_back(s);
        else   break;
    }
    return items;
}

StmtPtr Sintatico::parseItem() {
    if (isType()) {
        Token typeToken = consume();
        if (!check(TokenType::ID)) {
            addError("Expected identifier after type '" + typeToken.value + "'");
            synchronize();
            return nullptr;
        }
        Token name = consume();
        return parseItemSuffix(name, typeToken.value);
    }
    if (!atEnd()) return parseStatement();
    return nullptr;
}

StmtPtr Sintatico::parseItemSuffix(const Token& name, const QString& typeName) {
    if (check(TokenType::LEFT_PAREN)) {
        consume();
        auto params = parseParamListOpt();
        expect(TokenType::RIGHT_PAREN, "function definition");
        expect(TokenType::KW_THEN,     "function definition");
        auto body = parseStmtList();
        expect(TokenType::KW_END,      "function definition");

        auto node     = std::make_shared<FuncDefNode>();
        node->retType = typeName;
        node->name    = name.value;
        node->params  = params;
        node->body    = body;
        return node;
    }

    VarEntry first;
    first.name = name.value;
    if (check(TokenType::LEFT_BRACKET)) {
        consume();
        if (check(TokenType::LIT_INT)) { first.arraySize = consume().value.toInt(); }
        else addError("Expected integer size in array declaration");
        expect(TokenType::RIGHT_BRACKET, "array declaration");
    }
    auto more = parseMoreDeclIds();
    expect(TokenType::END_LINE, "declaration");

    auto node = std::make_shared<VarDeclNode>();
    node->type = typeName;
    node->vars.push_back(first);
    for (auto& v : more) node->vars.push_back(v);
    return node;
}

// ── Declarations ─────────────────────────────────────────────────────────────

std::vector<VarEntry> Sintatico::parseMoreDeclIds() {
    std::vector<VarEntry> vars;
    while (check(TokenType::SEPARATOR)) { consume(); vars.push_back(parseDeclVar()); }
    return vars;
}

VarEntry Sintatico::parseDeclVar() {
    VarEntry v;
    if (!check(TokenType::ID)) { addError("Expected identifier"); return v; }
    v.name = consume().value;
    if (check(TokenType::LEFT_BRACKET)) {
        consume();
        if (check(TokenType::LIT_INT)) v.arraySize = consume().value.toInt();
        else addError("Expected integer size in array declaration");
        expect(TokenType::RIGHT_BRACKET, "array declaration");
    }
    return v;
}

// ── Parameters ───────────────────────────────────────────────────────────────

std::vector<FuncParam> Sintatico::parseParamListOpt() {
    if (!check(TokenType::RIGHT_PAREN)) return parseParamList();
    return {};
}

std::vector<FuncParam> Sintatico::parseParamList() {
    std::vector<FuncParam> params;
    params.push_back(parseParam());
    while (check(TokenType::SEPARATOR)) { consume(); params.push_back(parseParam()); }
    return params;
}

FuncParam Sintatico::parseParam() {
    FuncParam p;
    if (!isType()) { addError("Expected type in parameter list"); return p; }
    p.type = consume().value;
    if (!check(TokenType::ID)) { addError("Expected identifier in parameter"); return p; }
    p.name = consume().value;
    if (check(TokenType::LEFT_BRACKET)) {
        consume();
        expect(TokenType::RIGHT_BRACKET, "array parameter");
        p.isArray = true;
    }
    return p;
}

// ── Statements ────────────────────────────────────────────────────────────────

std::vector<StmtPtr> Sintatico::parseStmtList() {
    std::vector<StmtPtr> stmts;
    while (!atEnd() && !check(TokenType::KW_END) && !check(TokenType::KW_ELSE)) {
        StmtPtr s = parseStatement();
        if (s) stmts.push_back(s);
        else   break;
    }
    return stmts;
}

StmtPtr Sintatico::parseStatement() {
    if (atEnd() || check(TokenType::KW_END) || check(TokenType::KW_ELSE))
        return nullptr;

    if (isType()) {
        Token typeToken = consume();
        if (!check(TokenType::ID)) {
            addError("Expected identifier after type '" + typeToken.value + "'");
            synchronize(); return nullptr;
        }
        Token name = consume();
        VarEntry first; first.name = name.value;
        if (check(TokenType::LEFT_BRACKET)) {
            consume();
            if (check(TokenType::LIT_INT)) first.arraySize = consume().value.toInt();
            else addError("Expected integer size");
            expect(TokenType::RIGHT_BRACKET, "array declaration");
        }
        auto more = parseMoreDeclIds();
        expect(TokenType::END_LINE, "declaration");

        auto node = std::make_shared<VarDeclNode>();
        node->type = typeToken.value;
        node->vars.push_back(first);
        for (auto& v : more) node->vars.push_back(v);
        return node;
    }

    if (check(TokenType::ID))          return parseAssignOrCall();
    if (check(TokenType::KW_IF))       return parseIfStmt();
    if (check(TokenType::KW_WHILE))    return parseWhileStmt();
    if (check(TokenType::KW_FOR))      return parseForStmt();
    if (check(TokenType::KW_DO))       return parseDoWhileStmt();
    if (check(TokenType::KW_READ))     return parseReadStmt();
    if (check(TokenType::KW_WRITE))    return parseWriteStmt();
    if (check(TokenType::KW_RETURN))   return parseReturnStmt();
    if (check(TokenType::KW_BREAK))    return parseBreakStmt();
    if (check(TokenType::KW_CONTINUE)) return parseContinueStmt();

    addError("Unexpected token '" + current().value + "'");
    synchronize();
    return nullptr;
}

StmtPtr Sintatico::parseAssignOrCall() {
    Token id = consume();
    return parseAssignOrCallTail(id);
}

StmtPtr Sintatico::parseAssignOrCallTail(const Token& id) {
    if (check(TokenType::ASSIGN)) {
        consume();
        ExprPtr val = parseExpr();
        expect(TokenType::END_LINE, "assignment");
        auto n = std::make_shared<AssignNode>();
        n->var = id.value; n->value = val;
        return n;
    }
    if (check(TokenType::LEFT_BRACKET)) {
        consume();
        ExprPtr idx = parseExpr();
        expect(TokenType::RIGHT_BRACKET, "array index");
        expect(TokenType::ASSIGN,        "array assignment");
        ExprPtr val = parseExpr();
        expect(TokenType::END_LINE,      "array assignment");
        auto n = std::make_shared<AssignNode>();
        n->var = id.value; n->index = idx; n->value = val;
        return n;
    }
    if (check(TokenType::LEFT_PAREN)) {
        consume();
        auto args = parseArgListOpt();
        expect(TokenType::RIGHT_PAREN, "function call");
        expect(TokenType::END_LINE,    "function call");
        auto n = std::make_shared<CallStmtNode>();
        n->func = id.value; n->args = args;
        return n;
    }
    addError("Expected '=', '[' or '(' after '" + id.value + "'");
    synchronize();
    return nullptr;
}

StmtPtr Sintatico::parseIfStmt() {
    consume();
    expect(TokenType::LEFT_PAREN,  "if condition");
    ExprPtr cond = parseExpr();
    expect(TokenType::RIGHT_PAREN, "if condition");
    expect(TokenType::KW_THEN,     "if statement");
    auto thenBody = parseStmtList();
    auto elseBody = parseElsePart();
    expect(TokenType::KW_END, "if statement");

    auto n = std::make_shared<IfNode>();
    n->cond = cond; n->thenBody = thenBody; n->elseBody = elseBody;
    return n;
}

std::vector<StmtPtr> Sintatico::parseElsePart() {
    if (check(TokenType::KW_ELSE)) { consume(); return parseStmtList(); }
    return {};
}

StmtPtr Sintatico::parseWhileStmt() {
    consume();
    expect(TokenType::LEFT_PAREN,  "while condition");
    ExprPtr cond = parseExpr();
    expect(TokenType::RIGHT_PAREN, "while condition");
    expect(TokenType::KW_THEN,     "while statement");
    auto body = parseStmtList();
    expect(TokenType::KW_END, "while statement");

    auto n = std::make_shared<WhileNode>();
    n->cond = cond; n->body = body;
    return n;
}

StmtPtr Sintatico::parseForStmt() {
    consume();
    expect(TokenType::LEFT_PAREN, "for statement");
    auto [initVar, initVal] = parseForInit();
    expect(TokenType::END_LINE, "for initializer");
    ExprPtr cond = parseExpr();
    expect(TokenType::END_LINE, "for condition");
    auto [postVar, postVal] = parseForPost();
    expect(TokenType::RIGHT_PAREN, "for statement");
    expect(TokenType::KW_THEN,     "for statement");
    auto body = parseStmtList();
    expect(TokenType::KW_END, "for statement");

    auto n = std::make_shared<ForNode>();
    n->initVar = initVar; n->initVal = initVal;
    n->cond    = cond;
    n->postVar = postVar; n->postVal = postVal;
    n->body    = body;
    return n;
}

std::pair<QString,ExprPtr> Sintatico::parseForInit() {
    if (check(TokenType::ID)) {
        Token id = consume();
        expect(TokenType::ASSIGN, "for initializer");
        return {id.value, parseExpr()};
    }
    return {"", nullptr};
}

std::pair<QString,ExprPtr> Sintatico::parseForPost() {
    if (check(TokenType::ID)) {
        Token id = consume();
        expect(TokenType::ASSIGN, "for post-operation");
        return {id.value, parseExpr()};
    }
    return {"", nullptr};
}

StmtPtr Sintatico::parseDoWhileStmt() {
    consume();
    std::vector<StmtPtr> body;
    while (!atEnd() && !check(TokenType::KW_END) &&
           !check(TokenType::KW_ELSE) && !check(TokenType::KW_WHILE)) {
        StmtPtr s = parseStatement();
        if (s) body.push_back(s);
        else   break;
    }
    expect(TokenType::KW_WHILE,    "do-while");
    expect(TokenType::LEFT_PAREN,  "do-while condition");
    ExprPtr cond = parseExpr();
    expect(TokenType::RIGHT_PAREN, "do-while condition");
    expect(TokenType::END_LINE,    "do-while statement");

    auto n = std::make_shared<DoWhileNode>();
    n->body = body; n->cond = cond;
    return n;
}

StmtPtr Sintatico::parseReadStmt() {
    consume();
    expect(TokenType::LEFT_PAREN, "read");
    if (!check(TokenType::ID)) { addError("Expected identifier in read"); synchronize(); return nullptr; }
    Token id = consume();
    ExprPtr idx;
    if (check(TokenType::LEFT_BRACKET)) {
        consume(); idx = parseExpr();
        expect(TokenType::RIGHT_BRACKET, "read array index");
    }
    expect(TokenType::RIGHT_PAREN, "read");
    expect(TokenType::END_LINE,    "read");
    auto n = std::make_shared<ReadNode>();
    n->var = id.value; n->index = idx;
    return n;
}

StmtPtr Sintatico::parseWriteStmt() {
    consume();
    expect(TokenType::LEFT_PAREN, "write");
    auto args = parseWriteArgList();
    expect(TokenType::RIGHT_PAREN, "write");
    expect(TokenType::END_LINE,    "write");
    auto n = std::make_shared<WriteNode>();
    n->args = args;
    return n;
}

std::vector<ExprPtr> Sintatico::parseWriteArgList() {
    if (check(TokenType::RIGHT_PAREN)) return {};
    return parseArgList();
}

StmtPtr Sintatico::parseReturnStmt() {
    consume();
    ExprPtr val;
    if (!check(TokenType::END_LINE)) val = parseExpr();
    expect(TokenType::END_LINE, "return");
    auto n = std::make_shared<ReturnNode>(); n->value = val; return n;
}

StmtPtr Sintatico::parseBreakStmt() {
    consume(); expect(TokenType::END_LINE, "break");
    return std::make_shared<BreakNode>();
}

StmtPtr Sintatico::parseContinueStmt() {
    consume(); expect(TokenType::END_LINE, "continue");
    return std::make_shared<ContinueNode>();
}

// ── Arguments ─────────────────────────────────────────────────────────────────

std::vector<ExprPtr> Sintatico::parseArgListOpt() {
    if (check(TokenType::RIGHT_PAREN)) return {};
    return parseArgList();
}

std::vector<ExprPtr> Sintatico::parseArgList() {
    std::vector<ExprPtr> args;
    args.push_back(parseExpr());
    while (check(TokenType::SEPARATOR)) { consume(); args.push_back(parseExpr()); }
    return args;
}

// ── Expressions ───────────────────────────────────────────────────────────────

ExprPtr Sintatico::parseExpr()               { return parseExprOrTail(parseExprAnd()); }
ExprPtr Sintatico::parseExprOrTail(ExprPtr left) {
    while (check(TokenType::OR)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprAnd();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprAnd()              { return parseExprAndTail(parseExprEquality()); }
ExprPtr Sintatico::parseExprAndTail(ExprPtr left) {
    while (check(TokenType::AND)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprEquality();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprEquality() { return parseExprEqualityTail(parseExprBitOr()); }

ExprPtr Sintatico::parseExprEqualityTail(ExprPtr left) {
    while (check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprBitOr();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprBitOr() { return parseExprBitOrTail(parseExprBitXor()); }

ExprPtr Sintatico::parseExprBitOrTail(ExprPtr left) {
    while (check(TokenType::BIT_OR)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprBitXor();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprBitXor() { return parseExprBitXorTail(parseExprBitAnd()); }

ExprPtr Sintatico::parseExprBitXorTail(ExprPtr left) {
    while (check(TokenType::BIT_XOR)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprBitAnd();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprBitAnd() { return parseExprBitAndTail(parseExprRelStrict()); }

ExprPtr Sintatico::parseExprBitAndTail(ExprPtr left) {
    while (check(TokenType::BIT_AND)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprRelStrict();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprRelStrict() { return parseExprRelStrictTail(parseExprShift()); }

ExprPtr Sintatico::parseExprRelStrictTail(ExprPtr left) {
    while (check(TokenType::GREATER) || check(TokenType::LESS) ||
           check(TokenType::GREATER_EQUAL) || check(TokenType::LESS_EQUAL)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprShift();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprShift() { return parseExprShiftTail(parseExprAdd()); }

ExprPtr Sintatico::parseExprShiftTail(ExprPtr left) {
    while (check(TokenType::SHIFT_LEFT) || check(TokenType::SHIFT_RIGHT)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprAdd();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprAdd()              { return parseExprAddTail(parseExprMul()); }
ExprPtr Sintatico::parseExprAddTail(ExprPtr left) {
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprMul();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprMul()              { return parseExprMulTail(parseExprUnary()); }
ExprPtr Sintatico::parseExprMulTail(ExprPtr left) {
    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE) || check(TokenType::MOD)) {
        Token op = consume();
        auto n = std::make_shared<BinOpExpr>();
        n->op = op.type; n->left = left; n->right = parseExprUnary();
        left = n;
    }
    return left;
}

ExprPtr Sintatico::parseExprUnary() {
    if (check(TokenType::NOT) || check(TokenType::MINUS) || check(TokenType::BIT_NOT)) {
        Token op = consume();
        auto n = std::make_shared<UnaryExpr>();
        n->op = op.type; n->operand = parseExprUnary();
        return n;
    }
    return parseExprPrimary();
}

ExprPtr Sintatico::parseExprPrimary() {
    if (check(TokenType::LEFT_PAREN)) {
        consume();
        ExprPtr e = parseExpr();
        expect(TokenType::RIGHT_PAREN, "parenthesized expression");
        return e;
    }
    if (check(TokenType::ID)) {
        Token id = consume();
        return parseIdExprTail(id);
    }

    Token t = consume();
    if (t.type == TokenType::LIT_INT) {
        auto n = std::make_shared<IntLitExpr>();
        n->value = t.value.toLongLong();
        return n;
    }
    if (t.type == TokenType::LIT_BINARY) {
        auto n = std::make_shared<IntLitExpr>();
        n->value = t.value.mid(2).toLongLong(nullptr, 2);
        return n;
    }
    if (t.type == TokenType::LIT_HEX) {
        auto n = std::make_shared<IntLitExpr>();
        n->value = t.value.toLongLong(nullptr, 16);
        return n;
    }
    if (t.type == TokenType::LIT_REAL) {
        auto n = std::make_shared<RealLitExpr>();
        n->value = t.value.toDouble();
        return n;
    }
    if (t.type == TokenType::LIT_CHAR) {
        auto n = std::make_shared<CharLitExpr>();
        n->value = t.value.length() >= 2 ? t.value[1] : QChar(0);
        return n;
    }
    if (t.type == TokenType::LIT_STRING) {
        auto n = std::make_shared<StringLitExpr>();
        n->value = t.value.mid(1, t.value.length() - 2);
        return n;
    }

    addError("Expected expression, found '" + t.value + "'");
    return std::make_shared<IntLitExpr>();
}

ExprPtr Sintatico::parseIdExprTail(const Token& id) {
    if (check(TokenType::LEFT_PAREN)) {
        consume();
        auto args = parseArgListOpt();
        expect(TokenType::RIGHT_PAREN, "function call");
        auto n = std::make_shared<CallExpr>();
        n->func = id.value; n->args = args;
        return n;
    }
    if (check(TokenType::LEFT_BRACKET)) {
        consume();
        ExprPtr idx = parseExpr();
        expect(TokenType::RIGHT_BRACKET, "array index");
        auto n = std::make_shared<ArrayExpr>();
        n->name = id.value; n->index = idx;
        return n;
    }
    auto n = std::make_shared<VarExpr>();
    n->name = id.value;
    return n;
}
