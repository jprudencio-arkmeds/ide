# C Compiler IDE

A lightweight IDE for the C programming language with integrated lexical and syntactic analysis.

## Features

- **Code Editor** with syntax highlighting
- **Lexical Analysis** — tokenizes C source code
- **Syntactic Analysis** — parses token streams and reports syntax errors
- **Semantic Analysis** — basic semantic checks
- **Example Programs** included to get started quickly

## Tech Stack

- **C++** with **Qt** for the GUI
- **GALS** (Gerador de Analisadores Léxicos e Sintáticos) for generating the lexer and parser
- **CMake** build system

## Project Structure

```
src/
├── main.cpp              # Application entry point
├── gals/                 # Generated lexer, parser, and semantic analyzer
│   ├── Lexico.cpp/h      # Lexical analyzer
│   ├── Sintatico.cpp/h   # Syntactic analyzer
│   ├── Semantico.cpp/h   # Semantic analyzer
│   ├── Constants.cpp/h   # Token and grammar constants
│   └── Token.h           # Token definition
├── ide/
│   ├── MainWindow.cpp/h  # Main application window
│   └── CodeEditor.cpp/h  # Code editor widget
example/                  # Example C programs
```

## Building

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, or Clang)
- CMake 3.16+
- Qt 5 or 6

### Build Steps

```bash
make build        # Build solution
make debug        # Build + Debug executable
make release      # Build + Release executable
```

The compiled binary will be located at `bin/[Debug|Release]/compilador.exe`.

## Usage

1. Run `compilador.exe`
2. Write or open a C source file in the editor
3. Use the IDE controls to run lexical and syntactic analysis
4. View errors and tokens in the output panel

## Examples

Sample programs are available in the `example/` directory:

- `helloworld.txt` — A simple Hello World program
- `all.txt` — A program demonstrating various language features

## License

This project is for educational purposes.
