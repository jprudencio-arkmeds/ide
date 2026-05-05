#include "Compilation.h"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: compilador <source-file>\n";
        return 1;
    }

    const std::string path = argv[1];
    CompileResult result = compileFile(path);

    for (const auto& message : result.messages) {
        std::cout << message.text << "\n";
    }

    if (result.success) {
        std::cout << "Compilation finished successfully." << "\n";
        return 0;
    }

    return 1;
}
