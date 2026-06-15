#include "AssemblyGenerator.h"

void AssemblyGenerator::appendData(Symbol* symbol) {
  m_assemblyData += "\t" + dataName(symbol) + " : " + symbol->value + "\n";
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
  m_assemblyText += "\t" + name + ":\n";
}

void AssemblyGenerator::appendRead(Symbol* symbol) {
  m_assemblyText += "\tLD $in_port\n";
  m_assemblyText += "\tSTO " + dataName(symbol) + "\n";
}

void AssemblyGenerator::appendWrite(Symbol* symbol) {
    m_assemblyText += "\tLD " + dataName(symbol) + "\n";
    m_assemblyText += "\tSTO $out_port\n";
}

void AssemblyGenerator::appendImmediateWrite(std::string value) {
    m_assemblyText += "\tLDI " + value + "\n";
    m_assemblyText += "\tSTO $out_port\n";
}