#include "AssemblyGenerator.h"
#include <cmath>

static std::string toIntLiteral(const std::string& value) {
    if (value.find('.') != std::string::npos) {
        return std::to_string(static_cast<int>(std::stof(value)));
    }
    return value;
}

void AssemblyGenerator::emitText(const std::string& s) {
    if (m_buffering) m_buffer += s;
    else             m_assemblyText += s;
}

void AssemblyGenerator::appendData(Symbol* symbol) {
    std::string val = symbol->value.empty() ? "0" : symbol->value;
    if (!val.empty() && val.front() == '"') val = "0";
    m_assemblyData += "\t" + dataName(symbol) + " : " + val + "\n";
}

void AssemblyGenerator::appendArrayData(Symbol* symbol) {
    std::string values = "0";
    for (size_t i = 1; i < symbol->arraySize; ++i) {
        values += ",0";
    }
    m_assemblyData += "\t" + dataName(symbol) + " : " + values + "\n";
}

void AssemblyGenerator::appendFunction(std::string name) {
    for (auto& c : name) c = std::toupper(c);
    emitText(name + ":\n");
}

void AssemblyGenerator::appendRead(Symbol* symbol) {
    emitText("\tLD $in_port\n");
    emitText("\tSTO " + dataName(symbol) + "\n");
}

void AssemblyGenerator::appendArrayRead(Symbol* symbol, std::string index) {
    appendIndexAccess(index);
    emitText("\tLD $in_port\n");
    emitText("\tSTOV " + dataName(symbol) + "\n");
}

void AssemblyGenerator::appendIndexAccess(std::string index) {
    emitText("\tLDI " + index + "\n");
    emitText("\tSTO $indr\n");
}

void AssemblyGenerator::appendWrite(Symbol* symbol) {
    // BIP IV não suporta strings
    if (symbol->type == "string") return;
    emitText("\tLD " + dataName(symbol) + "\n");
    emitText("\tSTO $out_port\n");
}

void AssemblyGenerator::appendImmediateWrite(std::string value) {
    emitText("\tLDI " + value + "\n");
    emitText("\tSTO $out_port\n");
}

void AssemblyGenerator::appendArrayWrite(Symbol* symbol, std::string index) {
    appendIndexAccess(index);
    emitText("\tLDV " + dataName(symbol) + "\n");
    emitText("\tSTO $out_port\n");
}

void AssemblyGenerator::appendLoadVar(Symbol* sym) {
    emitText("\tLD " + dataName(sym) + "\n");
}

void AssemblyGenerator::appendLoadImm(const std::string& value) {
    emitText("\tLDI " + value + "\n");
}

void AssemblyGenerator::appendLoadArrayElem(Symbol* sym, int index) {
    emitText("\tLDI " + std::to_string(index) + "\n");
    emitText("\tSTO $indr\n");
    emitText("\tLDV " + dataName(sym) + "\n");
}

void AssemblyGenerator::appendStoreResult(Symbol* dst, int dstIndex) {
    if (dstIndex < 0) {
        emitText("\tSTO " + dataName(dst) + "\n");
    } else {
        emitText("\tSTO 1000\n");
        emitText("\tLDI " + std::to_string(dstIndex) + "\n");
        emitText("\tSTO $indr\n");
        emitText("\tLD 1000\n");
        emitText("\tSTOV " + dataName(dst) + "\n");
    }
}

static std::string binaryInstr(TokenId op) {
    switch (op) {
    case t_KEY_PLUS:    return "ADD";
    case t_KEY_MINUS:   return "SUB";
    case t_KEY_BIT_AND: return "AND";
    case t_KEY_BIT_OR:  return "OR";
    case t_KEY_BIT_XOR: return "XOR";
    default:            return "";
    }
}

static std::string binaryImmInstr(TokenId op) {
    switch (op) {
    case t_KEY_PLUS:          return "ADDI";
    case t_KEY_MINUS:         return "SUBI";
    case t_KEY_BIT_AND:       return "ANDI";
    case t_KEY_BIT_OR:        return "ORI";
    case t_KEY_BIT_XOR:       return "XORI";
    case t_KEY_SHIFT_LEFT:    return "SLL";
    case t_KEY_SHIFT_RIGHT:   return "SRL";
    default:                  return "";
    }
}

void AssemblyGenerator::appendBinaryOp(TokenId op, Symbol* src) {
    const std::string instr = binaryInstr(op);
    if (!instr.empty())
        emitText("\t" + instr + " " + dataName(src) + "\n");
}

void AssemblyGenerator::appendBinaryOpImm(TokenId op, const std::string& value) {
    const std::string instr = binaryImmInstr(op);
    if (!instr.empty())
        emitText("\t" + instr + " " + value + "\n");
}

void AssemblyGenerator::appendBinaryOpWithArray(TokenId op, Symbol* src, int index) {
    if (op == t_KEY_MINUS) {
        emitText("\tSTO 1001\n");
        emitText("\tLDI " + std::to_string(index) + "\n");
        emitText("\tSTO $indr\n");
        emitText("\tLDV " + dataName(src) + "\n");
        emitText("\tSTO 1002\n");
        emitText("\tLD 1001\n");
        emitText("\tSUB 1002\n");
    } else {
        const std::string instr = binaryInstr(op);
        if (instr.empty()) return;
        emitText("\tSTO 1001\n");
        emitText("\tLDI " + std::to_string(index) + "\n");
        emitText("\tSTO $indr\n");
        emitText("\tLDV " + dataName(src) + "\n");
        emitText("\t" + instr + " 1001\n");
    }
}

void AssemblyGenerator::appendNot() {
    emitText("\tNOT\n");
}

// ── Control flow ──────────────────────────────────────────────────────────────

std::string AssemblyGenerator::nextLabel() {
    return "_CF" + std::to_string(m_labelCounter++);
}

void AssemblyGenerator::appendLabel(const std::string& label) {
    emitText(label + ":\n");
}

void AssemblyGenerator::appendJmp(const std::string& label) {
    emitText("\tJMP " + label + "\n");
}

void AssemblyGenerator::appendJmpZ(const std::string& label) {
    emitText("\tJMPZ " + label + "\n");
}

void AssemblyGenerator::appendJmpN(const std::string& label) {
    emitText("\tJMPN " + label + "\n");
}

// ACC holds left operand; after call ACC = (left OP rhs) ? 1 : 0
void AssemblyGenerator::appendRelationalOp(const std::string& op, Symbol* rhs) {
    const std::string lbl1 = nextLabel();
    const std::string lbl2 = nextLabel();
    const std::string r    = dataName(rhs);

    if (op == "<") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPN " + lbl1 + "\n");
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == ">") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPN " + lbl1 + "\n");  // negative  → a < b → false
        emitText("\tJMPZ " + lbl1 + "\n");  // zero      → a == b → false
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    } else if (op == "<=") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPN " + lbl1 + "\n");  // negative → a < b → true
        emitText("\tJMPZ " + lbl1 + "\n");  // zero     → a == b → true
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == ">=") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPN " + lbl1 + "\n");  // negative → a < b → false
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    } else if (op == "==") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");  // zero → equal → true
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == "!=") {
        emitText("\tSUB "  + r    + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");  // zero → equal → false
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    }
}

void AssemblyGenerator::appendRelationalOpImm(const std::string& op, const std::string& val) {
    const std::string lbl1 = nextLabel();
    const std::string lbl2 = nextLabel();

    if (op == "<") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPN " + lbl1 + "\n");
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == ">") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPN " + lbl1 + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    } else if (op == "<=") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPN " + lbl1 + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == ">=") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPN " + lbl1 + "\n");
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    } else if (op == "==") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");
        emitText("\tLDI 0\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 1\n");
        emitText(lbl2 + ":\n");
    } else if (op == "!=") {
        emitText("\tSUBI " + val  + "\n");
        emitText("\tJMPZ " + lbl1 + "\n");
        emitText("\tLDI 1\n");
        emitText("\tJMP "  + lbl2 + "\n");
        emitText(lbl1 + ":\n");
        emitText("\tLDI 0\n");
        emitText(lbl2 + ":\n");
    }
}

// ── Buffering ────────────────────────────────────────────────────────────────

void AssemblyGenerator::startBuffer() {
    m_buffering = true;
    m_buffer.clear();
}

void AssemblyGenerator::stopBuffer(std::string& out) {
    out = m_buffer;
    m_buffer.clear();
    m_buffering = false;
}

void AssemblyGenerator::appendBuffered(const std::string& code) {
    // Always writes to main text (never re-buffered)
    m_assemblyText += code;
}
