#include "AssemblyGenerator.h"

void AssemblyGenerator::appendData(Symbol* symbol) {
  const std::string val = symbol->value.empty() ? "0" : symbol->value;
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
  for (auto& c : name) {
    c = std::toupper(c);
  }
  m_assemblyText += name + ":\n";
}

void AssemblyGenerator::appendRead(Symbol* symbol) {
    m_assemblyText += "\tLD $in_port\n";
    m_assemblyText += "\tSTO " + dataName(symbol) + "\n";
}

void AssemblyGenerator::appendArrayRead(Symbol* symbol, std::string index) {
    appendIndexAccess(index);
    m_assemblyText += "\tLD $in_port\n";
    m_assemblyText += "\tSTOV " + dataName(symbol) + "\n";
}

void AssemblyGenerator::appendIndexAccess(std::string index) {
    m_assemblyText += "\tLDI " + index + "\n";
    m_assemblyText += "\tSTO $indr\n";
}

void AssemblyGenerator::appendWrite(Symbol* symbol) {
    m_assemblyText += "\tLD " + dataName(symbol) + "\n";
    m_assemblyText += "\tSTO $out_port\n";
}

void AssemblyGenerator::appendImmediateWrite(std::string value) {
    m_assemblyText += "\tLDI " + value + "\n";
    m_assemblyText += "\tSTO $out_port\n";
}

void AssemblyGenerator::appendArrayWrite(Symbol* symbol, std::string index) {
    appendIndexAccess(index);
    m_assemblyText += "\tLDV " + dataName(symbol) + "\n";
    m_assemblyText += "\tSTO $out_port\n";
}

void AssemblyGenerator::appendLoadVar(Symbol* sym) {
    m_assemblyText += "\tLD " + dataName(sym) + "\n";
}

void AssemblyGenerator::appendLoadImm(const std::string& value) {
    m_assemblyText += "\tLDI " + value + "\n";
}

void AssemblyGenerator::appendLoadArrayElem(Symbol* sym, int index) {
    m_assemblyText += "\tLDI " + std::to_string(index) + "\n";
    m_assemblyText += "\tSTO $indr\n";
    m_assemblyText += "\tLDV " + dataName(sym) + "\n";
}

// Armazena ACC no destino. Para vetores usa temporário 1000 para não perder ACC ao setar $indr.
void AssemblyGenerator::appendStoreResult(Symbol* dst, int dstIndex) {
    if (dstIndex < 0) {
        m_assemblyText += "\tSTO " + dataName(dst) + "\n";
    } else {
        m_assemblyText += "\tSTO 1000\n";
        m_assemblyText += "\tLDI " + std::to_string(dstIndex) + "\n";
        m_assemblyText += "\tSTO $indr\n";
        m_assemblyText += "\tLD 1000\n";
        m_assemblyText += "\tSTOV " + dataName(dst) + "\n";
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
        m_assemblyText += "\t" + instr + " " + dataName(src) + "\n";
}

void AssemblyGenerator::appendBinaryOpImm(TokenId op, const std::string& value) {
    const std::string instr = binaryImmInstr(op);
    if (!instr.empty())
        m_assemblyText += "\t" + instr + " " + value + "\n";
}

void AssemblyGenerator::appendBinaryOpWithArray(TokenId op, Symbol* src, int index) {
    if (op == t_KEY_MINUS) {
        m_assemblyText += "\tSTO 1001\n";
        m_assemblyText += "\tLDI " + std::to_string(index) + "\n";
        m_assemblyText += "\tSTO $indr\n";
        m_assemblyText += "\tLDV " + dataName(src) + "\n";
        m_assemblyText += "\tSTO 1002\n";
        m_assemblyText += "\tLD 1001\n";
        m_assemblyText += "\tSUB 1002\n";
    } else {
        const std::string instr = binaryInstr(op);
        if (instr.empty()) return;
        m_assemblyText += "\tSTO 1001\n";
        m_assemblyText += "\tLDI " + std::to_string(index) + "\n";
        m_assemblyText += "\tSTO $indr\n";
        m_assemblyText += "\tLDV " + dataName(src) + "\n";
        m_assemblyText += "\t" + instr + " 1001\n";
    }
}

void AssemblyGenerator::appendNot() {
    m_assemblyText += "\tNOT\n";
}