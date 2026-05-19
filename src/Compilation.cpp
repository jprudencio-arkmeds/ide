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

    std::string formatError(const std::string& where, const std::string& message, int position, const std::string& source) {
        int line, col;
        posToLineCol(source, position, line, col);
        return "[ERROR " + where + "] Line " + std::to_string(line) + ", Col " + std::to_string(col) + ": " + message;
    }
}

CompileResult compileSource(const std::string& source) {
    CompileResult result;
    if (source.empty()) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Warning, "No source code to analyze."});
        return result;
    }

    try {
        std::vector<Token> tokens;
        Lexico lexico(source.c_str());
        while (true) {
            Token* t = lexico.nextToken();
            if (t == nullptr || t->getId() == DOLLAR) {
                if (t) tokens.push_back(*t);
                delete t;
                break;
            }
            tokens.push_back(*t);
            delete t;
        }

        Lexico lexico2(source.c_str());
        Semantico semantico;
        Sintatico sintatico;
        sintatico.parse(&lexico2, &semantico);
        result.success = true;
        result.messages.push_back({CompileMessageKind::Success, "Program parsed successfully."});

        semantico.analyze(tokens);
        for (const auto& e : semantico.errors()) {
            result.success = false;
            result.messages.push_back({CompileMessageKind::Error,
                formatError("SEMANTIC", e.getMessage(), e.getPosition(), source)});
        }
        for (const auto& w : semantico.warnings()) {
            result.messages.push_back({CompileMessageKind::Warning,
                formatError("SEMANTIC WARNING", w.getMessage(), w.getPosition(), source)});
        }
    } catch (const SyntacticError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError("SYNTAX", e.getMessage(), e.getPosition(), source)});
    } catch (const LexicalError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError("LEXICAL", e.getMessage(), e.getPosition(), source)});
    } catch (const SemanticError& e) {
        result.success = false;
        result.messages.push_back({CompileMessageKind::Error, formatError("SEMANTIC", e.getMessage(), e.getPosition(), source)});
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
