#include "Compilation.h"

#include <iostream>

int main(int argc, char* argv[]) {
    std::string path;
    if (argc != 2) {
      std::cout << "Insert path: ";
      std::getline(std::cin, path);
    }
    else {
        path = argv[1];
    }

    const CompileResult result = compileFile(path);

    for (const auto& message : result.messages) {
        std::cout << message.text << "\n";
    }

    if (result.success) {
        std::cout << "Compilation finished successfully." << "\n";
        return 0;
    }

    return 1;
}
