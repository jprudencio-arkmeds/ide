#ifndef COMPILATION_H
#define COMPILATION_H

#include <string>
#include <vector>

enum class CompileMessageKind {
    Info,
    Success,
    Warning,
    Error
};

struct CompileMessage {
    CompileMessageKind kind;
    std::string text;
};

struct CompileResult {
    bool success;
    std::vector<CompileMessage> messages;
};

CompileResult compileSource(const std::string& source);
CompileResult compileFile(const std::string& path);

#endif // COMPILATION_H
