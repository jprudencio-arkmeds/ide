#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H

#include "gals_export.h"
#include "SymbolTable.h"
#include <string>

class _GALS_CLASS AssemblyGenerator
{
public:
	AssemblyGenerator() = default;
	virtual ~AssemblyGenerator() = default;

	std::string getAssembly() const { return m_assemblyData + "\n" + m_assemblyText; }

	void reset() {
		m_assemblyData = ".data\n";
		m_assemblyText = ".text\n";
	}

	void appendData(Symbol* symbol);

	void appendArrayData(Symbol* symbol);

	void appendFunction(std::string name);

	void appendRead(Symbol* symbol);

	void appendArrayRead(Symbol* symbol, std::string index);

	void appendWrite(Symbol* symbol);

	void appendArrayWrite(Symbol* symbol, std::string index);

	void appendImmediateWrite(std::string value);

private:

	void appendIndexAccess(std::string index);

	std::string dataName(const Symbol* symbol) const {
		return symbol->name + "_" + std::to_string(symbol->position);
	}

	std::string m_assemblyData = ".data\n";
	std::string m_assemblyText = ".text\n";
};

#endif