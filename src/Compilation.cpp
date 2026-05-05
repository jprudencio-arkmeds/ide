#include "Compilation.h"
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"
#include "gals/LexicalError.h"
#include "gals/SyntacticError.h"

#include <fstream>
#include <iterator>

namespace {

void posToLineCol(const std::string& src, int pos, int &line, int &col) {
    line = 1;
    col = 1;

    if (pos < 0) {
        return;
    }

    for (int i = 0; i < pos && i < static_cast<int>(src.size()); ++i) {
        if (src[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }
}

std::string formatError(const std::string& message, int position, const std::string& source) {
    int line, col;
    posToLineCol(source, position, line, col);
    return "[ERROR] Line " + std::to_string(line) + ", Col " + std::to_string(col) + ": " + message;
}

} // namespace

CompileResult compileSource(const std::string& source) {
    CompileResult result;
    if (source.empty()) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Warning, "No source code to analyze."});
        return result;
    }

    try {
        Lexico lexico(source.c_str());
        while (true) {
            Token* token = lexico.nextToken();
            if (token == nullptr || token->getId() == DOLLAR) {
                delete token;
                break;
            }
            delete token;
        }
    } catch (const LexicalError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError(e.getMessage(), e.getPosition(), source)});
        return result;
    }

    try {
        Lexico lexico(source.c_str());
        Semantico semantico;
        Sintatico sintatico;
        sintatico.parse(&lexico, &semantico);
        result.success = true;
        result.messages.push_back({CompileMessageKind::Success, "Program parsed successfully."});
    } catch (const SyntacticError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError(e.getMessage(), e.getPosition(), source)});
    } catch (const LexicalError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError(e.getMessage(), e.getPosition(), source)});
    }

    return result;
}

CompileResult compileFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return {false, {{CompileMessageKind::Error, "Unable to open source file: " + path}}};
    }

    std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return compileSource(source);
}
