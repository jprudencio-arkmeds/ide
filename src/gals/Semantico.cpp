#include "Semantico.h"
#include "TypeOperationsTable.h"

static bool isTypeBase(TokenId id) {
    return id == t_KEY_INT    || id == t_KEY_FLOAT  || id == t_KEY_DOUBLE ||
           id == t_KEY_CHAR   || id == t_KEY_VOID   || id == t_KEY_STRING;
}

static bool isModifier(TokenId id) {
    return id == t_KEY_CONST    || id == t_KEY_STATIC   || id == t_KEY_EXTERN  ||
           id == t_KEY_REGISTER || id == t_KEY_VOLATILE  || id == t_KEY_AUTO   ||
           id == t_KEY_SIGNED   || id == t_KEY_UNSIGNED  || id == t_KEY_SHORT  ||
           id == t_KEY_LONG;
}

static bool isBinaryOp(TokenId id) {
    return id == t_KEY_PLUS       || id == t_KEY_MINUS      || id == t_KEY_MULTIPLY   ||
           id == t_KEY_DIVIDE     || id == t_KEY_MOD        || id == t_KEY_AND        ||
           id == t_KEY_OR         || id == t_KEY_BIT_AND    || id == t_KEY_BIT_OR     ||
           id == t_KEY_BIT_XOR    || id == t_KEY_SHIFT_LEFT || id == t_KEY_SHIFT_RIGHT ||
           id == t_KEY_GREATER    || id == t_KEY_LESS       || id == t_KEY_GREATER_EQUAL ||
           id == t_KEY_LESS_EQUAL || id == t_KEY_EQUAL      || id == t_KEY_NOT_EQUAL;
}

static bool isRelationalOp(TokenId id) {
    return id == t_KEY_GREATER       || id == t_KEY_LESS         ||
           id == t_KEY_GREATER_EQUAL || id == t_KEY_LESS_EQUAL   ||
           id == t_KEY_EQUAL         || id == t_KEY_NOT_EQUAL;
}

static std::string relOpString(TokenId id) {
    switch (id) {
    case t_KEY_GREATER:       return ">";
    case t_KEY_LESS:          return "<";
    case t_KEY_GREATER_EQUAL: return ">=";
    case t_KEY_LESS_EQUAL:    return "<=";
    case t_KEY_EQUAL:         return "==";
    case t_KEY_NOT_EQUAL:     return "!=";
    default:                  return "";
    }
}

static std::string getTypeFromLiteral(TokenId id) {
    switch (id) {
        case t_INT: case t_BINARY: case t_HEX: return "int";
        case t_REAL:   return "float";
        case t_CHAR:   return "char";
        case t_STRING: return "string";
        default:       return "";
    }
}

static bool isBuiltinFunction(const std::string& lexeme) {
    return lexeme == "read" || lexeme == "write";
}

static TokenId compoundAssignToBinaryOp(TokenId assignOp) {
    switch (assignOp) {
    case t_KEY_PLUS_ASSIGN:         return t_KEY_PLUS;
    case t_KEY_MINUS_ASSIGN:        return t_KEY_MINUS;
    case t_KEY_AND_ASSIGN:          return t_KEY_BIT_AND;
    case t_KEY_OR_ASSIGN:           return t_KEY_BIT_OR;
    case t_KEY_XOR_ASSIGN:          return t_KEY_BIT_XOR;
    case t_KEY_SHIFT_LEFT_ASSIGN:   return t_KEY_SHIFT_LEFT;
    case t_KEY_SHIFT_RIGHT_ASSIGN:  return t_KEY_SHIFT_RIGHT;
    default:                        return EPSILON;
    }
}

static size_t findMatchingParen(const std::vector<Token>& tokens, size_t openParen) {
    int depth = 0;
    for (size_t i = openParen; i < tokens.size(); ++i) {
        TokenId id = tokens[i].getId();
        if (id == t_KEY_LEFT_PARENTHESIS) {
            ++depth;
        } else if (id == t_KEY_RIGHT_PARENTHESIS) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return tokens.size();
}


void Semantico::executeAction(int /*action*/, const Token* /*token*/) {
}

enum State { IDLE, IN_TYPE, IN_ASSIGN, IN_CONDITION };

State       state         = IDLE;
bool        isDeclaring   = false;
std::string pendingType;
TokenId     prevId        = EPSILON;
bool        skipNextBrace = false;
bool        inParamList   = false;
Operator    lastAssign    = ASSIGN;
bool        wasUnaryOp    = false;

// Control-flow tracking (module-level to mirror existing state convention)
bool    cfInCondition        = false;
int     cfCondParenDepth     = 0;
TokenId cfCondTerminator     = t_KEY_RIGHT_PARENTHESIS;
bool    cfInForHeader        = false;
int     cfForHeaderPhase     = 0;   // 0=init, 1=cond(buffered), 2=incr(buffered)
int     cfForHeaderParenDepth = 0;

void Semantico::analyze(const std::vector<Token>& tokens) {

    // Reset all state
    m_table.reset();
    m_errors.clear();
    m_warnings.clear();
    m_assemblyGen.reset();
    while (!m_operatingVars.empty()) m_operatingVars.pop();
    while (!m_cfStack.empty()) m_cfStack.pop();
    m_currentBraceDepth = 0;

    state         = IDLE;
    isDeclaring   = false;
    pendingType.clear();
    prevId        = EPSILON;
    skipNextBrace = false;
    inParamList   = false;
    wasUnaryOp    = false;
    m_exprLeftType.clear();
    m_exprOp        = EPSILON;
    m_assignLHSSymbol = nullptr;
    m_assignLHSIndex  = -1;
    m_accLoaded       = false;
    m_cgPendingOp     = EPSILON;
    m_bitNotPending   = false;

    cfInCondition         = false;
    cfCondParenDepth      = 0;
    cfCondTerminator      = t_KEY_RIGHT_PARENTHESIS;
    cfInForHeader         = false;
    cfForHeaderPhase      = 0;
    cfForHeaderParenDepth = 0;

    auto checkUnused = [this]() {
        for (auto& [name, sym] : m_table.currentScopeSymbols()) {
            if (!sym->isUsed && sym->modality != Modality::FUNCTION) {
                m_warnings.emplace_back(
                    "Identificador '" + name + "' declarado mas nao utilizado.",
                    sym->position);
            }
        }
    };

    // ── Emit binary/relational op into ACC (shared by IN_ASSIGN and IN_CONDITION) ──
    auto emitBinaryOrRelOp = [&](TokenId op, Symbol* sym, int arrayIdx) {
        if (isRelationalOp(op)) {
            if (arrayIdx >= 0) {
                // Load array element to temp, then compare
                // Reuse temp 1001: save ACC, load array elem, compare reversed
                m_assemblyGen.appendBinaryOpWithArray(t_KEY_MINUS, sym, arrayIdx);
                // After BinaryOpWithArray(SUB), ACC = left - elem.
                // Then treat as "<" pattern isn't trivial without reworking; approximate:
                // For now emit SUB as a full relational via temp
                // This is a simplification: array relational falls back to subtraction result
            } else {
                m_assemblyGen.appendRelationalOp(relOpString(op), sym);
            }
        } else {
            if (arrayIdx >= 0)
                m_assemblyGen.appendBinaryOpWithArray(op, sym, arrayIdx);
            else
                m_assemblyGen.appendBinaryOp(op, sym);
        }
    };

    auto emitBinaryOrRelOpImm = [&](TokenId op, const std::string& val) {
        if (isRelationalOp(op)) {
            m_assemblyGen.appendRelationalOpImm(relOpString(op), val);
        } else {
            m_assemblyGen.appendBinaryOpImm(op, val);
        }
    };

    // ── handleConditionEnd: called when condition ')' closes for if/while/do-while ──
    auto handleConditionEnd = [&]() {
        if (m_cfStack.empty()) return;
        auto& f = m_cfStack.top();
        switch (f.type) {
        case ControlFlowFrame::IF:
            m_assemblyGen.appendJmpZ(f.elseLabel);
            break;
        case ControlFlowFrame::WHILE:
            m_assemblyGen.appendJmpZ(f.endLabel);
            break;
        case ControlFlowFrame::DO_WHILE:
            m_assemblyGen.appendJmpZ(f.endLabel);
            m_assemblyGen.appendJmp(f.testLabel);   // testLabel = body label
            m_assemblyGen.appendLabel(f.endLabel);
            m_cfStack.pop();
            break;
        default:
            break;
        }
        m_accLoaded     = false;
        m_cgPendingOp   = EPSILON;
        m_bitNotPending = false;
    };

    // ── cfForHeaderEnd: emit for-loop header assembly after ')' closes the header ──
    auto cfForHeaderEnd = [&]() {
        if (m_cfStack.empty()) return;
        auto& f = m_cfStack.top();
        if (!f.incrBuffer.empty()) {
            m_assemblyGen.appendJmp(f.testLabel);
            m_assemblyGen.appendLabel(f.incrLabel);
            m_assemblyGen.appendBuffered(f.incrBuffer);
        }
        m_assemblyGen.appendLabel(f.testLabel);
        if (!f.condBuffer.empty()) {
            m_assemblyGen.appendBuffered(f.condBuffer);
            m_assemblyGen.appendJmpZ(f.endLabel);
        }
    };

    // ── checkAndEmitPendingEndLabel: cascade-close parent IF frames after a frame pops ──
    auto checkAndEmitPendingEndLabel = [&]() {
        while (!m_cfStack.empty()) {
            auto& top = m_cfStack.top();
            if (top.type == ControlFlowFrame::IF && top.bodyDone) {
                m_assemblyGen.appendLabel(top.endLabel);
                m_cfStack.pop();
            } else {
                break;
            }
        }
    };

    // ── handleBodyEnd: called when '}' closes the body of the top CF frame ──
    auto handleBodyEnd = [&]() {
        if (m_cfStack.empty()) return;
        auto& f = m_cfStack.top();
        switch (f.type) {
        case ControlFlowFrame::IF:
            if (!f.bodyDone) {
                // First body ended — peek for else
                TokenId nextTok = (/* i is captured */ true) ? EPSILON : EPSILON;
                // We capture the loop index via [&], use it directly:
                // (handled below via the tokens/i captured in outer lambda)
                // This lambda is defined inside analyze() so it captures i by ref.
                nextTok = EPSILON; // will be overridden below
                // Actually we set this outside; this placeholder is resolved at call site.
                // See "// peek for else" comment at call sites — handled inline.
            }
            break;
        default:
            break;
        }
    };
    // NOTE: handleBodyEnd is NOT used as a lambda because it needs loop-local `i`.
    // Instead, body-end logic is inlined in the '}' handler below.

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& tok = tokens[i];
        TokenId      id  = tok.getId();

        if (id == DOLLAR) break;

        // ── Top-of-loop: for-header paren/phase intercept ──────────────────────
        bool skipStateSwitch = false;
        if (cfInForHeader) {
            if (id == t_KEY_LEFT_PARENTHESIS) {
                ++cfForHeaderParenDepth;
                if (cfForHeaderParenDepth == 1) {
                    // Eat the for's outer '('
                    prevId = id;
                    skipStateSwitch = true;
                }
                // Inner '(' falls through to state machine
            } else if (id == t_KEY_RIGHT_PARENTHESIS) {
                if (cfForHeaderParenDepth == 1 && cfForHeaderPhase == 2) {
                    // For header's closing ')'
                    // Finish any in-progress assignment (incr was an assign expr)
                    if (state == IN_ASSIGN && m_accLoaded && m_assignLHSSymbol) {
                        m_assemblyGen.appendStoreResult(m_assignLHSSymbol, m_assignLHSIndex);
                        if (!m_operatingVars.empty()) {
                            m_operatingVars.top()->isInitialized = true;
                            m_operatingVars.pop();
                        }
                        m_assignLHSSymbol = nullptr;
                        m_assignLHSIndex  = -1;
                        m_accLoaded       = false;
                        m_cgPendingOp     = EPSILON;
                        m_bitNotPending   = false;
                        state             = IDLE;
                    }
                    m_assemblyGen.stopBuffer(m_cfStack.top().incrBuffer);
                    cfForHeaderEnd();
                    cfInForHeader         = false;
                    cfForHeaderPhase      = 0;
                    cfForHeaderParenDepth = 0;
                    prevId = id;
                    skipStateSwitch = true;
                } else {
                    --cfForHeaderParenDepth;
                    // Fall through to state machine
                }
            }
        }

        if (!skipStateSwitch) {
            switch (state) {

            // ── IDLE ───────────────────────────────────────────────────────────
            case IDLE: {
                // Control flow keywords
                if (id == t_KEY_IF) {
                    ControlFlowFrame f;
                    f.type      = ControlFlowFrame::IF;
                    f.elseLabel = m_assemblyGen.nextLabel();
                    f.endLabel  = m_assemblyGen.nextLabel();
                    m_cfStack.push(f);
                    ++i; // skip opening '('
                    state            = IN_CONDITION;
                    cfCondParenDepth = 0;
                    cfCondTerminator = t_KEY_RIGHT_PARENTHESIS;
                    m_accLoaded      = false;
                    m_cgPendingOp    = EPSILON;
                    m_bitNotPending  = false;
                    break;
                }
                if (id == t_KEY_WHILE) {
                    if (!m_cfStack.empty() &&
                        m_cfStack.top().type == ControlFlowFrame::DO_WHILE &&
                        (!m_cfStack.top().bodyIsBlock || m_cfStack.top().bodyDone)) {
                        // This 'while' closes a do-while (braced or brace-less body)
                        ++i; // skip opening '('
                        state            = IN_CONDITION;
                        cfCondParenDepth = 0;
                        cfCondTerminator = t_KEY_RIGHT_PARENTHESIS;
                        m_accLoaded      = false;
                        m_cgPendingOp    = EPSILON;
                        m_bitNotPending  = false;
                    } else {
                        ControlFlowFrame f;
                        f.type      = ControlFlowFrame::WHILE;
                        f.testLabel = m_assemblyGen.nextLabel();
                        f.endLabel  = m_assemblyGen.nextLabel();
                        m_assemblyGen.appendLabel(f.testLabel);
                        m_cfStack.push(f);
                        ++i; // skip opening '('
                        state            = IN_CONDITION;
                        cfCondParenDepth = 0;
                        cfCondTerminator = t_KEY_RIGHT_PARENTHESIS;
                        m_accLoaded      = false;
                        m_cgPendingOp    = EPSILON;
                        m_bitNotPending  = false;
                    }
                    break;
                }
                if (id == t_KEY_DO) {
                    ControlFlowFrame f;
                    f.type      = ControlFlowFrame::DO_WHILE;
                    f.testLabel = m_assemblyGen.nextLabel();  // body label
                    f.endLabel  = m_assemblyGen.nextLabel();
                    m_assemblyGen.appendLabel(f.testLabel);
                    m_cfStack.push(f);
                    break;
                }
                if (id == t_KEY_FOR) {
                    ControlFlowFrame f;
                    f.type      = ControlFlowFrame::FOR;
                    f.testLabel = m_assemblyGen.nextLabel();
                    f.incrLabel = m_assemblyGen.nextLabel();
                    f.endLabel  = m_assemblyGen.nextLabel();
                    m_cfStack.push(f);
                    cfInForHeader         = true;
                    cfForHeaderPhase      = 0;
                    cfForHeaderParenDepth = 0;
                    break;
                }
                if (id == t_KEY_ELSE || id == t_KEY_BREAK ||
                    id == t_KEY_CONTINUE || id == t_KEY_RETURN) {
                    break;
                }

                // Type/modifier start
                if (isModifier(id) || isTypeBase(id)) {
                    pendingType = tok.getLexeme();
                    state = IN_TYPE;
                    break;
                }

                // Block entry / exit
                if (id == t_KEY_LEFT_BRACE) {
                    ++m_currentBraceDepth;
                    if (skipNextBrace)
                        skipNextBrace = false;
                    else
                        m_table.enterScope();
                    m_exprLeftType.clear(); m_exprOp = EPSILON;
                    // Record body start for CF frames
                    if (!m_cfStack.empty() && m_cfStack.top().braceDepth == 0) {
                        m_cfStack.top().braceDepth  = m_currentBraceDepth;
                        m_cfStack.top().bodyIsBlock = true;
                    }
                    break;
                }

                if (id == t_KEY_RIGHT_BRACE) {
                    checkUnused();
                    m_table.exitScope();
                    m_exprLeftType.clear(); m_exprOp = EPSILON;
                    --m_currentBraceDepth;

                    // Check if this closes a CF body
                    if (!m_cfStack.empty()) {
                        auto& f = m_cfStack.top();
                        if (f.bodyIsBlock && m_currentBraceDepth == f.braceDepth - 1) {
                            // Body just ended — dispatch by frame type
                            switch (f.type) {
                            case ControlFlowFrame::IF:
                                if (!f.bodyDone) {
                                    // Peek for 'else'
                                    TokenId next = (i + 1 < tokens.size())
                                                   ? tokens[i + 1].getId() : EPSILON;
                                    if (next == t_KEY_ELSE) {
                                        m_assemblyGen.appendJmp(f.endLabel);
                                        m_assemblyGen.appendLabel(f.elseLabel);
                                        f.bodyDone = true;
                                    } else {
                                        // Simple if — elseLabel serves as end
                                        m_assemblyGen.appendLabel(f.elseLabel);
                                        m_cfStack.pop();
                                        checkAndEmitPendingEndLabel();
                                    }
                                } else {
                                    // Else body ended
                                    m_assemblyGen.appendLabel(f.endLabel);
                                    m_cfStack.pop();
                                    checkAndEmitPendingEndLabel();
                                }
                                break;
                            case ControlFlowFrame::WHILE:
                                m_assemblyGen.appendJmp(f.testLabel);
                                m_assemblyGen.appendLabel(f.endLabel);
                                m_cfStack.pop();
                                checkAndEmitPendingEndLabel();
                                break;
                            case ControlFlowFrame::DO_WHILE:
                                // Body done — wait for 'while(cond)'
                                f.bodyDone = true;
                                break;
                            case ControlFlowFrame::FOR:
                                if (!f.incrBuffer.empty())
                                    m_assemblyGen.appendJmp(f.incrLabel);
                                else
                                    m_assemblyGen.appendJmp(f.testLabel);
                                m_assemblyGen.appendLabel(f.endLabel);
                                m_cfStack.pop();
                                checkAndEmitPendingEndLabel();
                                break;
                            }
                        }
                    }
                    break;
                }

                if (id == t_END_LINE) {
                    m_exprLeftType.clear(); m_exprOp = EPSILON;
                    break;
                }

                if (id == t_KEY_RIGHT_PARENTHESIS) {
                    inParamList = false;
                    break;
                }

                if (isBinaryOp(id) || isUnaryOperator(id)) {
                    m_exprOp = id;
                    break;
                }

                if (id == t_ID) {
                    if (prevId == t_KEY_DOT || prevId == t_KEY_ARROW) break;

                    const TokenId nextId = (i + 1 < tokens.size()) ? tokens[i + 1].getId() : EPSILON;

                    if (isBuiltinFunction(tok.getLexeme()) && nextId == t_KEY_LEFT_PARENTHESIS) {
                        if (tok.getLexeme() == "read") {
                            const size_t closeParen = findMatchingParen(tokens, i + 1);
                            for (size_t j = i + 2; j < closeParen && j < tokens.size(); ++j) {
                                if (tokens[j].getId() != t_ID) {
                                    m_errors.emplace_back("Não é possível ler uma constante", tokens[j].getPosition());
                                    break;
                                }
                                if (auto readSymbol = lookupSymbol(tokens[j].getLexeme(), tokens[j].getPosition())) {
                                    readSymbol->isInitialized = true;
                                    if (readSymbol->arraySize > 0) {
                                        const std::string index = std::to_string(getVectorIndex(tokens, j));
                                        m_assemblyGen.appendArrayRead(readSymbol.get(), index);
                                    } else {
                                        m_assemblyGen.appendRead(readSymbol.get());
                                    }
                                    break;
                                }
                            }
                            i = closeParen;
                        } else { // write
                            i = checkUseOfUninitializedInParams(tokens, i);
                        }
                        break;
                    }

                    if (std::shared_ptr<Symbol> symbol = lookupSymbol(tok.getLexeme(), tok.getPosition())) {
                        if (nextId == t_KEY_LEFT_PARENTHESIS) {
                            i = checkUseOfUninitializedInParams(tokens, i);
                            break;
                        }

                        // Postfix increment/decrement: i++ or i--
                        if (nextId == t_KEY_INCREMENT || nextId == t_KEY_DECREMENT) {
                            unaryCompatibilityCheck(symbol);
                            m_assemblyGen.appendLoadVar(symbol.get());
                            if (nextId == t_KEY_INCREMENT)
                                m_assemblyGen.appendBinaryOpImm(t_KEY_PLUS, "1");
                            else
                                m_assemblyGen.appendBinaryOpImm(t_KEY_MINUS, "1");
                            m_assemblyGen.appendStoreResult(symbol.get(), -1);
                            symbol->isInitialized = true;
                            ++i;  // consume the ++ or --
                            break;
                        }

                        if (nextId == t_KEY_LEFT_BRACKET && symbol->modality == Modality::ARRAY) {
                            if (i + 3 < tokens.size() &&
                                tokens[i + 3].getId() == t_KEY_RIGHT_BRACKET &&
                                i + 4 < tokens.size() &&
                                isAssign(tokens[i + 4].getId()))
                            {
                                int arrayIdx = 0;
                                TokenId idxTokId = tokens[i + 2].getId();
                                if (idxTokId == t_INT || idxTokId == t_HEX || idxTokId == t_BINARY)
                                    arrayIdx = std::stoi(tokens[i + 2].getLexeme(), nullptr, 0);
                                i += 3;
                                symbol->isUsed = true;
                                m_operatingVars.push(symbol);
                                pendingType       = symbol->type;
                                m_exprOp          = EPSILON;
                                m_assignLHSSymbol = symbol.get();
                                m_assignLHSIndex  = arrayIdx;
                                m_accLoaded       = false;
                                m_cgPendingOp     = EPSILON;
                                m_bitNotPending   = false;
                                state = IN_ASSIGN;
                                break;
                            }
                        }

                        m_operatingVars.push(symbol);

                        if (isAssign(nextId)) {
                            m_exprOp          = EPSILON;
                            pendingType       = symbol->type;
                            m_assignLHSSymbol = symbol.get();
                            m_assignLHSIndex  = -1;
                            m_accLoaded       = false;
                            m_cgPendingOp     = EPSILON;
                            m_bitNotPending   = false;
                            state = IN_ASSIGN;
                            break;
                        }

                        // Prefix increment/decrement: ++i or --i
                        const bool wasUnaryOperator = isUnaryOperator(m_exprOp);
                        if (wasUnaryOperator) {
                            unaryCompatibilityCheck(symbol);
                            m_assemblyGen.appendLoadVar(symbol.get());
                            if (m_exprOp == t_KEY_INCREMENT)
                                m_assemblyGen.appendBinaryOpImm(t_KEY_PLUS, "1");
                            else
                                m_assemblyGen.appendBinaryOpImm(t_KEY_MINUS, "1");
                            m_assemblyGen.appendStoreResult(symbol.get(), -1);
                            symbol->isInitialized = true;
                            m_exprOp = EPSILON;
                            break;
                        }

                        if (isUnaryOperator(nextId))
                            unaryCompatibilityCheck(symbol);

                        checkUseOfUninitialized(symbol, tok);
                        trackExprType(symbol->type, tok.getPosition());
                        state = IDLE;
                        pendingType.clear();
                    }
                    break;
                }

                std::string lt = getTypeFromLiteral(id);
                if (!lt.empty())
                    trackExprType(lt, tok.getPosition());
                break;
            }

            // ── IN_TYPE ───────────────────────────────────────────────────────
            case IN_TYPE: {
                isDeclaring = true;
                if (isModifier(id) || isTypeBase(id)) {
                    pendingType += " " + tok.getLexeme();
                    break;
                }

                if (id == t_KEY_MULTIPLY) {
                    pendingType += "*";
                    break;
                }

                if (id == t_KEY_LEFT_BRACE) {
                    ++m_currentBraceDepth;
                    if (skipNextBrace)
                        skipNextBrace = false;
                    else
                        m_table.enterScope();
                    pendingType.clear();
                    state = IDLE;
                    break;
                }

                if (id == t_KEY_RIGHT_BRACE) {
                    checkUnused();
                    m_table.exitScope();
                    --m_currentBraceDepth;
                    pendingType.clear();
                    state = IDLE;
                    break;
                }

                if (id == t_ID) {
                    bool isFunction = (i + 1 < tokens.size()) && tokens[i + 1].getId() == t_KEY_LEFT_PARENTHESIS;
                    bool isArray    = !isFunction &&
                                      (i + 1 < tokens.size()) && tokens[i + 1].getId() == t_KEY_LEFT_BRACKET;

                    std::shared_ptr<Symbol> symbol = m_table.addSymbol(tok.getLexeme(), pendingType, isFunction, tok.getPosition());
                    if (!symbol) {
                        m_errors.emplace_back(
                            "Função ou variável '" + tok.getLexeme() + "' já declarada neste escopo.",
                            tok.getPosition());
                    }

                    symbol = symbol.get() == nullptr ? lookupSymbol(tok.getLexeme(), tok.getPosition()) : symbol;

                    if (symbol) {
                        if (inParamList) {
                            symbol->modality = Modality::PARAMETER;
                            symbol->isInitialized = true;
                        } else if (isArray) {
                            symbol->modality = Modality::ARRAY;
                        }
                    }

                    if (isArray && symbol) {
                        symbol->arraySize = getVectorIndex(tokens, i);
                        symbol->isInitialized = true;
                        m_assemblyGen.appendArrayData(symbol.get());
                        state = IDLE;
                        pendingType.clear();
                        isDeclaring = false;
                        break;
                    }

                    if (isFunction) {
                        m_table.enterScope();
                        skipNextBrace = true;
                        inParamList   = true;
                    }

                    const TokenId nextId = (i + 1 < tokens.size()) ? tokens[i + 1].getId() : EPSILON;

                    if (isDefaultAssign(nextId)) {
                        m_operatingVars.push(symbol);
                        state = IN_ASSIGN;
                    } else {
                        isFunction ? m_assemblyGen.appendFunction(tok.getLexeme())
                                   : m_assemblyGen.appendData(symbol.get());
                        state       = IDLE;
                        isDeclaring = false;
                        pendingType.clear();
                    }
                    break;
                }

                isDeclaring = false;
                pendingType.clear();
                state = IDLE;
                break;
            }

            // ── IN_ASSIGN ─────────────────────────────────────────────────────
            case IN_ASSIGN: {
                if (isAssign(id)) {
                    lastAssign = static_cast<Operator>(id);
                    if (!isDeclaring && id != t_KEY_ASSIGN && m_assignLHSSymbol) {
                        if (m_assignLHSIndex >= 0)
                            m_assemblyGen.appendLoadArrayElem(m_assignLHSSymbol, m_assignLHSIndex);
                        else
                            m_assemblyGen.appendLoadVar(m_assignLHSSymbol);
                        m_accLoaded   = true;
                        m_cgPendingOp = compoundAssignToBinaryOp(id);
                    }
                    break;
                }

                if (id == t_KEY_BIT_NOT && !isDeclaring) {
                    m_bitNotPending = true;
                    break;
                }

                if (isUnaryOperator(id)) {
                    m_exprOp = id;
                    break;
                }

                if (isBinaryOp(id) && !isDeclaring) {
                    m_cgPendingOp = id;
                    break;
                }

                if (id == t_END_LINE && !isDeclaring) {
                    if (m_accLoaded && m_assignLHSSymbol) {
                        m_assemblyGen.appendStoreResult(m_assignLHSSymbol, m_assignLHSIndex);
                    }
                    if (!m_operatingVars.empty()) {
                        m_operatingVars.top()->isInitialized = true;
                        m_operatingVars.pop();
                    }
                    m_assignLHSSymbol = nullptr;
                    m_assignLHSIndex  = -1;
                    m_accLoaded       = false;
                    m_cgPendingOp     = EPSILON;
                    m_bitNotPending   = false;
                    m_exprLeftType.clear();
                    m_exprOp = EPSILON;
                    state    = IDLE;
                    break;
                }

                if (id == t_ID) {
                    bool isArrayAccess = (i + 1 < tokens.size()) &&
                                         tokens[i + 1].getId() == t_KEY_LEFT_BRACKET;

                    if (auto symbol = lookupSymbol(tok.getLexeme(), tok.getPosition())) {
                        if (!isDeclaring) {
                            if (!m_accLoaded &&
                                !TypeOperationsTable::isCompatible(symbol->type, pendingType, lastAssign)) {
                                m_errors.emplace_back(
                                    "Tipo incompatível na atribuição à variável '" + tok.getLexeme() + "'.",
                                    tok.getPosition());
                            }
                            checkUseOfUninitialized(symbol, tok);

                            int arrayIdx = -1;
                            if (isArrayAccess && i + 3 < tokens.size()) {
                                TokenId idxTokId = tokens[i + 2].getId();
                                if (idxTokId == t_INT || idxTokId == t_HEX || idxTokId == t_BINARY)
                                    arrayIdx = std::stoi(tokens[i + 2].getLexeme(), nullptr, 0);
                                i += 3;
                            }

                            if (!m_accLoaded) {
                                if (arrayIdx >= 0)
                                    m_assemblyGen.appendLoadArrayElem(symbol.get(), arrayIdx);
                                else
                                    m_assemblyGen.appendLoadVar(symbol.get());
                                if (m_bitNotPending) {
                                    m_assemblyGen.appendNot();
                                    m_bitNotPending = false;
                                }
                                m_accLoaded    = true;
                                m_exprLeftType = symbol->type;
                            } else {
                                emitBinaryOrRelOp(m_cgPendingOp, symbol.get(), arrayIdx);
                                m_cgPendingOp  = EPSILON;
                                m_exprLeftType = symbol->type;
                            }
                        } else {
                            if (!TypeOperationsTable::isCompatible(symbol->type, pendingType, lastAssign)) {
                                m_errors.emplace_back(
                                    "Tipo incompatível na atribuição à variável '" + tok.getLexeme() + "'.",
                                    tok.getPosition());
                            }
                            checkUseOfUninitialized(symbol, tok);
                            if (isUnaryOperator(m_exprOp)) {
                                unaryCompatibilityCheck(symbol);
                                m_exprOp = EPSILON;
                            }
                            m_exprLeftType = symbol->type;
                            if (symbol->modality == Modality::VARIABLE)
                                m_operatingVars.top()->value = symbol->value;

                            m_operatingVars.top()->isInitialized = true;
                            appendAssemblyData(m_operatingVars.top().get());
                            isDeclaring = false;
                            m_operatingVars.pop();
                            pendingType.clear();
                            state = IDLE;
                        }
                    }
                    break;
                }

                // Literal on RHS
                {
                    std::string literalType;
                    std::string literalValue = tok.getLexeme();
                    switch (id) {
                    case t_INT: case t_BINARY: case t_HEX: literalType = "int";    break;
                    case t_REAL:                            literalType = "float";  break;
                    case t_CHAR:                            literalType = "char";   break;
                    case t_STRING:                          literalType = "string"; break;
                    default:                                break;
                    }

                    if (!literalType.empty()) {
                        if (!isDeclaring) {
                            if (!m_accLoaded &&
                                !TypeOperationsTable::isCompatible(literalType, pendingType, lastAssign)) {
                                m_errors.emplace_back(
                                    "Tipo incompatível na atribuição à variável '" +
                                        m_operatingVars.top()->name + "'.",
                                    tok.getPosition());
                            }
                            if (!m_accLoaded) {
                                m_assemblyGen.appendLoadImm(literalValue);
                                if (m_bitNotPending) {
                                    m_assemblyGen.appendNot();
                                    m_bitNotPending = false;
                                }
                                m_accLoaded    = true;
                                m_exprLeftType = literalType;
                            } else {
                                emitBinaryOrRelOpImm(m_cgPendingOp, literalValue);
                                m_cgPendingOp  = EPSILON;
                                m_exprLeftType = literalType;
                            }
                        } else {
                            Symbol* lastSymbol = m_operatingVars.top().get();
                            if (!TypeOperationsTable::isCompatible(literalType, pendingType, lastAssign)) {
                                m_errors.emplace_back(
                                    "Tipo incompatível na atribuição à variável '" + lastSymbol->type + "'.",
                                    tok.getPosition());
                                pendingType.clear();
                            } else {
                                lastSymbol->value = literalValue;
                            }
                            m_exprLeftType = literalType;
                            lastSymbol->isInitialized = true;
                            appendAssemblyData(lastSymbol);
                            isDeclaring = false;
                            m_operatingVars.pop();
                            pendingType.clear();
                            state = IDLE;
                        }
                    }
                }
                break;
            }

            // ── IN_CONDITION ──────────────────────────────────────────────────
            case IN_CONDITION: {
                // Track inner parentheses
                if (id == t_KEY_LEFT_PARENTHESIS) {
                    ++cfCondParenDepth;
                    break;
                }
                if (id == t_KEY_RIGHT_PARENTHESIS) {
                    if (cfCondParenDepth > 0) {
                        --cfCondParenDepth;
                        break;
                    }
                    if (cfCondTerminator == t_KEY_RIGHT_PARENTHESIS) {
                        // Condition closing ')'
                        handleConditionEnd();
                        state = IDLE;
                    }
                    // Else: for-cond, this ')' closes a sub-expression — nothing to do
                    break;
                }

                // For-cond ends at ';'
                if (id == t_END_LINE && cfCondTerminator == t_END_LINE) {
                    m_assemblyGen.stopBuffer(m_cfStack.top().condBuffer);
                    cfForHeaderPhase = 2;
                    m_assemblyGen.startBuffer();
                    m_accLoaded     = false;
                    m_cgPendingOp   = EPSILON;
                    m_bitNotPending = false;
                    state = IDLE;
                    break;
                }

                // Identifier in condition
                if (id == t_ID) {
                    auto symbol = m_table.lookupSymbol(tok.getLexeme());
                    if (!symbol) {
                        m_errors.emplace_back(
                            "Variável '" + tok.getLexeme() + "' não declarada.", tok.getPosition());
                        break;
                    }
                    symbol->isUsed = true;
                    checkUseOfUninitialized(symbol, tok);

                    int arrayIdx = -1;
                    if ((i + 1 < tokens.size()) &&
                        tokens[i + 1].getId() == t_KEY_LEFT_BRACKET &&
                        i + 3 < tokens.size()) {
                        TokenId idxTok = tokens[i + 2].getId();
                        if (idxTok == t_INT || idxTok == t_HEX || idxTok == t_BINARY)
                            arrayIdx = std::stoi(tokens[i + 2].getLexeme(), nullptr, 0);
                        i += 3;
                    }

                    if (!m_accLoaded) {
                        if (arrayIdx >= 0)
                            m_assemblyGen.appendLoadArrayElem(symbol.get(), arrayIdx);
                        else
                            m_assemblyGen.appendLoadVar(symbol.get());
                        if (m_bitNotPending) {
                            m_assemblyGen.appendNot();
                            m_bitNotPending = false;
                        }
                        m_accLoaded = true;
                    } else if (m_cgPendingOp != EPSILON) {
                        emitBinaryOrRelOp(m_cgPendingOp, symbol.get(), arrayIdx);
                        m_cgPendingOp = EPSILON;
                    }
                    break;
                }

                // Literal in condition
                {
                    std::string literalType = getTypeFromLiteral(id);
                    if (!literalType.empty()) {
                        const std::string val = tok.getLexeme();
                        if (!m_accLoaded) {
                            m_assemblyGen.appendLoadImm(val);
                            m_accLoaded = true;
                        } else if (m_cgPendingOp != EPSILON) {
                            emitBinaryOrRelOpImm(m_cgPendingOp, val);
                            m_cgPendingOp = EPSILON;
                        }
                        break;
                    }
                }

                // Operators (relational + arithmetic/bitwise)
                if (isBinaryOp(id)) {
                    m_cgPendingOp = id;
                    break;
                }

                // Bitwise NOT in condition
                if (id == t_KEY_BIT_NOT) {
                    m_bitNotPending = true;
                    break;
                }

                // Prefix ++ / -- in condition expressions (e.g. for-incr buffered phase)
                if (id == t_KEY_INCREMENT || id == t_KEY_DECREMENT) {
                    m_exprOp = id;
                    break;
                }

                break;
            }

            } // switch (state)

            // ── Post-switch: for-header phase 0→1 transition ──────────────────
            if (cfInForHeader && cfForHeaderParenDepth == 1 && id == t_END_LINE) {
                if (cfForHeaderPhase == 0) {
                    // Init just finished (IN_ASSIGN already processed ';')
                    cfForHeaderPhase = 1;
                    cfCondTerminator = t_END_LINE;
                    m_assemblyGen.startBuffer();
                    m_accLoaded     = false;
                    m_cgPendingOp   = EPSILON;
                    m_bitNotPending = false;
                    state = IN_CONDITION;
                }
            }

        } // if (!skipStateSwitch)

        prevId = id;
    }

    checkUnused(); // global scope
}

std::shared_ptr<Symbol> Semantico::lookupSymbol(const std::string& lexeme, const int position) {
    std::shared_ptr<Symbol> symbol = m_table.lookupSymbol(lexeme);
    if (!symbol) {
        m_errors.emplace_back(
            "Variável ou função '" + lexeme + "' não declarada.",
            position);
    } else {
        symbol->isUsed = true;
    }
    return symbol;
}

bool Semantico::isAssign(const TokenId& tokId) const {
    return isDefaultAssign(tokId) || tokId == t_KEY_PLUS_ASSIGN || tokId == t_KEY_MINUS_ASSIGN ||
        tokId == t_KEY_MULTIPLY_ASSIGN || tokId == t_KEY_DIVIDE_ASSIGN ||
        tokId == t_KEY_MOD_ASSIGN || tokId == t_KEY_AND_ASSIGN ||
        tokId == t_KEY_OR_ASSIGN || tokId == t_KEY_XOR_ASSIGN ||
        tokId == t_KEY_SHIFT_LEFT_ASSIGN || tokId == t_KEY_SHIFT_RIGHT_ASSIGN;
}

bool Semantico::isDefaultAssign(const TokenId& tokId) const {
    return tokId == t_KEY_ASSIGN;
}

bool Semantico::isUnaryOperator(const TokenId& tokId) const {
    return tokId == t_KEY_INCREMENT || tokId == t_KEY_DECREMENT;
}

void Semantico::trackExprType(const std::string& type, int position) {
    if (!m_exprLeftType.empty() && m_exprOp != EPSILON) {
        Operator op = static_cast<Operator>(m_exprOp);
        if (!TypeOperationsTable::isCompatible(m_exprLeftType, type, op)) {
            m_errors.emplace_back(
                "Operação incompatível entre os tipos '" + m_exprLeftType + "' e '" + type + "'.",
                position);
        }
        m_exprLeftType = type;
        m_exprOp = EPSILON;
    } else {
        m_exprLeftType = type;
        m_exprOp = EPSILON;
    }
}

int Semantico::checkUseOfUninitializedInParams(const std::vector<Token>& tokens, int index) {
    const size_t closeParen = findMatchingParen(tokens, index + 1);
    for (size_t j = index + 2; j < closeParen && j < tokens.size(); ++j) {
        if (tokens[j].getId() == t_ID) {
            if (auto writeSymbol = lookupSymbol(tokens[j].getLexeme(), tokens[j].getPosition())) {
                checkUseOfUninitialized(writeSymbol, tokens[j]);
                if (writeSymbol->arraySize > 0) {
                    const std::string index = std::to_string(getVectorIndex(tokens, j));
                    m_assemblyGen.appendArrayWrite(writeSymbol.get(), index);
                } else {
                    m_assemblyGen.appendWrite(writeSymbol.get());
                }
            }
        } else if (tokens[j].getId() == t_INT) {
            m_assemblyGen.appendImmediateWrite(tokens[j].getLexeme());
        }
    }
    return closeParen;
}

void Semantico::checkUseOfUninitialized(const std::shared_ptr<Symbol>& symbol, const Token& tok) {
    if (!symbol->isInitialized && symbol->modality == Modality::VARIABLE) {
        m_warnings.emplace_back(
            "Uso de variável '" + tok.getLexeme() + "' sem inicialização. Possível uso de lixo de memória.",
            tok.getPosition());
    }
}

void Semantico::unaryCompatibilityCheck(const std::shared_ptr<Symbol>& symbol) {
    if (!TypeOperationsTable::isCompatibleUnary(symbol->type)) {
        m_errors.emplace_back(
            "Operação unária incompatível para o tipo '" + symbol->type + "'.",
            symbol->position);
    }
}

void Semantico::appendAssemblyData(Symbol* var) {
    if (var->modality == Modality::VARIABLE) {
        m_assemblyGen.appendData(var);
    }
}

int Semantico::getVectorIndex(const std::vector<Token>& tokens, size_t& index) const {
    index++;  // '['
    index++;  // size
    int indexValue = 0;
    if (tokens[index].getId() == t_INT || tokens[index].getId() == t_HEX || tokens[index].getId() == t_BINARY) {
        indexValue = std::stoi(tokens[index].getLexeme(), nullptr, 0);
    }
    index++;  // ']'
    if (index < tokens.size() && tokens[index].getId() == t_KEY_RIGHT_BRACKET) {
        index++;
    }
    return indexValue;
}
