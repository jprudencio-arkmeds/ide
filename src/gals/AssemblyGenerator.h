#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H

#include "gals_export.h"
#include "SymbolTable.h"
#include "Constants.h"
#include <string>

class _GALS_CLASS AssemblyGenerator
{
public:
	AssemblyGenerator() = default;
	virtual ~AssemblyGenerator() = default;

	std::string getAssembly() const { return m_assemblyData + "\n" + m_assemblyText + "\tHLT 0\n"; }

	void reset() {
		m_assemblyData = ".data\n";
		m_assemblyText = ".text\n";
		m_labelCounter = 0;
		m_buffering    = false;
		m_buffer.clear();
	}

	void appendData(Symbol* symbol);
	void appendArrayData(Symbol* symbol);

	void appendFunction(std::string name);
	void appendRead(Symbol* symbol);
	void appendArrayRead(Symbol* symbol, std::string index);
	void appendWrite(Symbol* symbol);
	void appendArrayWrite(Symbol* symbol, std::string index);
	void appendImmediateWrite(std::string value);

	void appendLoadVar(Symbol* sym);
	void appendLoadImm(const std::string& value);
	void appendLoadArrayElem(Symbol* sym, int index);

	void appendStoreResult(Symbol* dst, int dstIndex);

	void appendBinaryOp(TokenId op, Symbol* src);
	void appendBinaryOpImm(TokenId op, const std::string& value);
	void appendBinaryOpWithArray(TokenId op, Symbol* src, int index);

	void appendNot();

	// Control flow: labels and jumps
	std::string nextLabel();
	void appendLabel(const std::string& label);
	void appendJmp(const std::string& label);
	void appendJmpZ(const std::string& label);
	void appendJmpN(const std::string& label);

	// Relational comparison: ACC OP rhs → 0 or 1 in ACC
	void appendRelationalOp(const std::string& op, Symbol* rhs);
	void appendRelationalOpImm(const std::string& op, const std::string& val);

	// Buffering (for for-loop condition and increment deferral)
	void startBuffer();
	void stopBuffer(std::string& out);
	void appendBuffered(const std::string& code);

private:
	void emitText(const std::string& s);
	void appendIndexAccess(std::string index);

	std::string dataName(const Symbol* symbol) const {
		return symbol->name + "_" + std::to_string(symbol->position);
	}

	std::string m_assemblyData = ".data\n";
	std::string m_assemblyText = ".text\n";
	int         m_labelCounter = 0;
	bool        m_buffering    = false;
	std::string m_buffer;
};

#endif
