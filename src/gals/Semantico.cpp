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

enum State { IDLE, IN_TYPE, IN_ASSIGN };

State       state         = IDLE;
bool        isDeclaring   = false;
std::string pendingType;
TokenId     prevId        = EPSILON;
bool        skipNextBrace = false;
bool        inParamList   = false;   // true enquanto estamos dentro de '(' params ')'
Operator    lastAssign    = ASSIGN;
bool        wasUnaryOp    = false;

void Semantico::analyze(const std::vector<Token>& tokens) {

    // Reinicia todo o estado para permitir múltiplas compilações consecutivas
    m_table.reset();
    m_errors.clear();
    m_warnings.clear();
    m_assemblyGen.reset();
    while (!m_operatingVars.empty()) m_operatingVars.pop();
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

    // Emite aviso para identificadores declarados mas não usados no escopo atual.
    // Funções ficam de fora: podem fazer parte de uma API não chamada internamente.
    auto checkUnused = [this]() {
        for (auto& [name, sym] : m_table.currentScopeSymbols()) {
            if (!sym->isUsed && sym->modality != Modality::FUNCTION) {
                m_warnings.emplace_back(
                    "Identificador '" + name + "' declarado mas nao utilizado.",
                    sym->position);
            }
        }
    };

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& tok = tokens[i];
        TokenId      id  = tok.getId();

        if (id == DOLLAR) break;

        switch (state) {

        case IDLE: {
            if (isModifier(id) || isTypeBase(id)) {
                pendingType = tok.getLexeme();
                state = IN_TYPE;
                break;
            }

            if (id == t_KEY_LEFT_BRACE) {
                if (skipNextBrace)
                    skipNextBrace = false;
                else
                    m_table.enterScope();
                m_exprLeftType.clear(); m_exprOp = EPSILON;

                break;
            }

            if (id == t_KEY_RIGHT_BRACE) {
                checkUnused();
                m_table.exitScope();
                m_exprLeftType.clear(); m_exprOp = EPSILON;
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
                // Acesso a membro (obj.campo / obj->campo) não é uso de variável.
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
                                }
                                else {
                                    m_assemblyGen.appendRead(readSymbol.get());
                                }
                                break;
                            }
                        }
                        i = closeParen;
                    }
                    else { // write
                        i = checkUseOfUninitializedInParams(tokens, i);
                    }
                    break;
                }

                if (std::shared_ptr<Symbol> symbol = lookupSymbol(tok.getLexeme(), tok.getPosition())) {

                    if (nextId == t_KEY_LEFT_PARENTHESIS) {
                        i = checkUseOfUninitializedInParams(tokens, i);
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
                            // Avança i para ']'; o for vai para '='
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

                    if (isUnaryOperator(nextId))
                        unaryCompatibilityCheck(symbol);

                    const bool wasUnaryOperator = isUnaryOperator(m_exprOp);
                    if (wasUnaryOperator)
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

                // Ajusta a modalidade conforme o contexto da declaração
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
                    pendingType.clear(); // 
                    isDeclaring = false;
                    break;
                }

                if (isFunction) {
                    m_table.enterScope();
                    skipNextBrace = true;
                    inParamList   = true;  // os próximos identificadores até ')' são parâmetros
                }

                const TokenId nextId = (i + 1 < tokens.size()) ? tokens[i + 1].getId() : EPSILON;

                if (isDefaultAssign(nextId)) {
                    m_operatingVars.push(symbol);
                    state = IN_ASSIGN;
                }
                else {
                    isFunction ? m_assemblyGen.appendFunction(tok.getLexeme()) : m_assemblyGen.appendData(symbol.get());
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
                            if (arrayIdx >= 0)
                                m_assemblyGen.appendBinaryOpWithArray(m_cgPendingOp, symbol.get(), arrayIdx);
                            else
                                m_assemblyGen.appendBinaryOp(m_cgPendingOp, symbol.get());
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

            // --- Literal no RHS ---
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
                            m_assemblyGen.appendBinaryOpImm(m_cgPendingOp, literalValue);
                            m_cgPendingOp  = EPSILON;
                            m_exprLeftType = literalType;
                        }
                    } else {
                        // Declaração: armazena valor para seção .data
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
        }

        prevId = id;
    }

    checkUnused(); // verifica escopo global ao fim da análise
}

std::shared_ptr<Symbol> Semantico::lookupSymbol(const std::string& lexeme, const int position) {
    std::shared_ptr<Symbol> symbol = m_table.lookupSymbol(lexeme);
    if (!symbol) {
        m_errors.emplace_back(
            "Variável ou função '" + lexeme + "' não declarada.",
            position);
    } else {
        symbol->isUsed = true;  // marca como utilizado
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
                }
                else {
                    m_assemblyGen.appendWrite(writeSymbol.get());
                }
            }
        }
        else if (tokens[j].getId() == t_INT) {
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
    // Parse [tamanho]
    index++; // '['
    index++; // tamanho
    int indexValue = 0;
    if (tokens[index].getId() == t_INT || tokens[index].getId() == t_HEX || tokens[index].getId() == t_BINARY) {
        indexValue = std::stoi(tokens[index].getLexeme(), nullptr, 0);
    }
    index++; // ']'
    if (index < tokens.size() && tokens[index].getId() == t_KEY_RIGHT_BRACKET) {
        index++; // skip ']'
    }

    return indexValue;
}