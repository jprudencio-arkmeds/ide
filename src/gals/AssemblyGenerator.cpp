#include "AssemblyGenerator.h"

void AssemblyGenerator::appendData(Symbol* symbol) {
  const std::string dataName = symbol->name + "_" + std::to_string(symbol->position);
  m_assemblyData += "\t" + dataName + ": " + symbol->value + "\n";
}

void AssemblyGenerator::appendFunction(std::string name) {
  for (auto& c : name) {
    c = std::toupper(c);
  }
  m_assemblyText += "\t" + name + ":\n";
}