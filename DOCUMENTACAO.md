# Documentação — Compiler IDE

## 1. Visão Geral

Este projeto é uma **IDE de compilador** desenvolvida em C++ com Qt. Ela permite escrever programas em uma linguagem imperativa customizada, executar análise léxica, análise sintática e interpretação diretamente na interface gráfica.

O fluxo completo é:

```
Código-fonte (texto)
    → Léxico (tokenização)
    → Sintático (parser LL(1), geração de AST)
    → Interpretador (execução via árvore sintática)
    → Saída na interface
```

A IDE exibe os tokens reconhecidos, erros léxicos e sintáticos com linha e coluna, e a saída do programa em abas separadas.

---

## 2. Arquitetura

O projeto é dividido em quatro camadas:

### 2.1 IDE (Qt) — `src/ide/`

Interface gráfica construída com Qt Widgets. Composta por:

- **`MainWindow`** — janela principal. Gerencia menus (Arquivo, Executar), o editor de código, o painel de tokens, as abas de compilação e saída, e a barra de status. Orquestra o pipeline: chama o Lexer, Parser e Interpreter em sequência ao pressionar F5.
- **`CodeEditor`** — editor de texto baseado em `QPlainTextEdit` com numeração de linhas (via `LineNumberArea`), destaque da linha atual e fonte monoespaçada (Fira Code).

### 2.2 Léxico — `src/lexer/`

- **`Token`** (`Token.h`, `Token.cpp`) — define o enum `TokenType` com todos os tipos de tokens (palavras-chave, literais, operadores, delimitadores) e a struct `Token` (tipo, valor, linha, coluna).
- **`Lexer`** (`Lexer.h`, `Lexer.cpp`) — analisador léxico que percorre o código-fonte caractere a caractere e produz uma lista de tokens. Suporta comentários de linha (`//`) e bloco (`/* */`), literais inteiros decimais, binários (`0b...`) e hexadecimais (`0x...`), literais reais, caracteres e strings.

### 2.3 Sintático — `src/parser/`

- **`Parser`** (`Parser.h`, `Parser.cpp`) — parser LL(1) por descida recursiva. Valida a sintaxe e constrói simultaneamente a Árvore Sintática Abstrata (AST). Em caso de erro, registra a linha, coluna e mensagem descritiva, e tenta se recuperar via sincronização (avança até encontrar `;` ou `end`/`else`).

### 2.4 Interpretador — `src/interpreter/`

- **`AST.h`** — define todos os nós da AST: declarações, definições de funções, comandos (`if`, `while`, `for`, `do-while`, `read`, `write`, `return`, `break`, `continue`) e expressões (literais, variáveis, arrays, chamadas, operações binárias e unárias).
- **`Value.h`** — define o tipo de valor em tempo de execução (`Value`), com suporte a `INT`, `REAL`, `CHAR`, `STRING` e `VOID`. Inclui conversões automáticas entre tipos.
- **`Interpreter`** (`Interpreter.h`, `Interpreter.cpp`) — interpretador de árvore (*tree-walking interpreter*). Usa `Environment` (ambientes encadeados) para gerenciar escopos de variáveis. Implementa um limite de 500.000 passos para detectar loops infinitos. Sinais de controle de fluxo (`break`, `continue`, `return`) são propagados via exceções C++.

---

## 3. Linguagem Suportada

### 3.1 Palavras-chave

| Palavra-chave | Uso |
|---|---|
| `int`, `float`, `char`, `string`, `void` | Tipos de dados |
| `if`, `then`, `else`, `end` | Condicional |
| `while`, `for`, `do` | Laços |
| `return`, `break`, `continue` | Controle de fluxo |
| `read`, `write` | Entrada e saída |

### 3.2 Tipos de dados

- `int` — inteiro de 64 bits (suporta literais decimais, binários `0b101` e hexadecimais `0xFF`)
- `float` — ponto flutuante de precisão dupla
- `char` — caractere único (literal: `'A'`)
- `string` — cadeia de caracteres (literal: `"texto"`)
- `void` — tipo de retorno para procedimentos sem valor

### 3.3 Operadores

| Categoria | Operadores |
|---|---|
| Aritméticos | `+`, `-`, `*`, `/`, `%` |
| Relacionais | `==`, `!=`, `>`, `<`, `>=`, `<=` |
| Lógicos | `&&`, `\|\|`, `!` |
| Bit a bit | `&`, `\|`, `^`, `~`, `<<`, `>>` |
| Atribuição | `=` |

### 3.4 Estruturas de controle

```
if (condição) then
    ...
else
    ...
end

while (condição) then
    ...
end

for (i = 0; i < n; i = i + 1) then
    ...
end

do
    ...
while (condição);
```

### 3.5 Funções e Procedimentos

```
int nomeFuncao(int a, int b) then
    return a + b;
end

void procedimento(int arr[], int tamanho) then
    ...
end
```

### 3.6 Entrada e Saída

```
read(x);
read(arr[i]);
write(x);
write("mensagem", x);
```

---

## 4. Gramática Sintática

O parser implementa a seguinte gramática LL(1) (resumo das principais regras):

```
<program>       ::= <item_list>

<item_list>     ::= <item> <item_list> | ε

<item>          ::= <type> ID <item_suffix> | <statement>

<item_suffix>   ::= '(' <param_list_opt> ')' then <stmt_list> end
                  | <var_id_suffix> ';'

<type>          ::= int | float | char | string | void

<var_id_suffix> ::= '[' INT ']' <more_decl_ids> | <more_decl_ids>

<more_decl_ids> ::= ',' <decl_var> <more_decl_ids> | ε

<decl_var>      ::= ID '[' INT ']' | ID

<param_list_opt> ::= <param_list> | ε

<param_list>    ::= <param> ',' <param_list> | <param>

<param>         ::= <type> ID '[' ']' | <type> ID

<stmt_list>     ::= <statement> <stmt_list> | ε

<statement>     ::= <type> ID <var_id_suffix> ';'
                  | <assign_or_call>
                  | <if_stmt>
                  | <while_stmt>
                  | <for_stmt>
                  | <dowhile_stmt>
                  | <read_stmt>
                  | <write_stmt>
                  | <return_stmt>
                  | <break_stmt>
                  | <continue_stmt>

<assign_or_call> ::= ID <assign_or_call_tail> ';'

<assign_or_call_tail> ::= '=' <expr>
                        | '[' <expr> ']' '=' <expr>
                        | '(' <arg_list_opt> ')'

<if_stmt>       ::= if '(' <expr> ')' then <stmt_list> <else_part> end

<else_part>     ::= else <stmt_list> | ε

<while_stmt>    ::= while '(' <expr> ')' then <stmt_list> end

<for_stmt>      ::= for '(' <for_init> ';' <expr> ';' <for_post> ')' then <stmt_list> end

<for_init>      ::= ID '=' <expr> | ε

<for_post>      ::= ID '=' <expr> | ε

<dowhile_stmt>  ::= do <stmt_list> while '(' <expr> ')' ';'

<read_stmt>     ::= read '(' <read_arg> ')' ';'

<read_arg>      ::= ID '[' <expr> ']' | ID

<write_stmt>    ::= write '(' <write_arg_list> ')' ';'

<return_stmt>   ::= return <expr> ';' | return ';'

<break_stmt>    ::= break ';'

<continue_stmt> ::= continue ';'

<arg_list_opt>  ::= <arg_list> | ε

<arg_list>      ::= <expr> ',' <arg_list> | <expr>

-- Expressões (precedência crescente):
<expr>          ::= <expr_and> <expr_or_tail>
<expr_and>      ::= <expr_not> <expr_and_tail>
<expr_not>      ::= '!' <expr_not> | <expr_rel>
<expr_rel>      ::= <expr_add> <rel_tail>
<expr_add>      ::= <expr_mul> <expr_add_tail>
<expr_mul>      ::= <expr_unary> <expr_mul_tail>
<expr_unary>    ::= '-' <expr_unary> | '~' <expr_unary> | <expr_primary>
<expr_primary>  ::= '(' <expr> ')'
                  | ID <id_expr_tail>
                  | INT | REAL | BINARY | HEX | CHAR_LIT | STRING_LIT

<id_expr_tail>  ::= '(' <arg_list_opt> ')' | '[' <expr> ']' | ε
```

---

## 5. Como Compilar e Executar

### Pré-requisitos

- CMake >= 3.16
- Qt6 (módulos: Widgets)
- Compilador C++17 (GCC, Clang ou MSVC)

### Compilação

```bash
cd /home/arkmeds/compilador/build
cmake ..
make -j$(nproc)
```

### Execução

```bash
./CompilerIDE
```

Ao iniciar, a IDE carrega automaticamente o arquivo `~/compilador/exemplo.txt` no editor, se ele existir.

### Atalhos de teclado

| Ação | Atalho |
|---|---|
| Novo arquivo | Ctrl+N |
| Abrir arquivo | Ctrl+O |
| Salvar | Ctrl+S |
| Salvar como | Ctrl+Shift+S |
| Compilar e executar | F5 |
| Sair | Ctrl+Q |

---

## 6. Estrutura de Arquivos

```
compilador/
├── build/                        Diretório de build (CMake out-of-source)
├── src/
│   ├── main.cpp                  Ponto de entrada: inicializa Qt, aplica stylesheet, abre MainWindow
│   ├── ide/
│   │   ├── MainWindow.h          Declaração da janela principal
│   │   ├── MainWindow.cpp        Implementação: UI, menus, pipeline de compilação
│   │   ├── CodeEditor.h          Declaração do editor com numeração de linhas
│   │   └── CodeEditor.cpp        Implementação: gutter, highlight de linha atual
│   ├── lexer/
│   │   ├── Token.h               Enum TokenType e struct Token
│   │   ├── Token.cpp             Funções auxiliares: tokenTypeName, isTypeKeyword
│   │   ├── Lexer.h               Declaração do analisador léxico
│   │   └── Lexer.cpp             Implementação: tokenize, leitores de literais e comentários
│   ├── parser/
│   │   ├── Parser.h              Declaração do parser LL(1) e struct ParseError
│   │   └── Parser.cpp            Implementação: descida recursiva, construção da AST
│   └── interpreter/
│       ├── AST.h                 Todos os nós da AST (statements e expressões)
│       ├── Value.h               Tipo de valor em runtime com conversões
│       ├── Interpreter.h         Declaração do interpretador e da classe Environment
│       └── Interpreter.cpp       Implementação: execução de statements, avaliação de expressões
├── gramatica_sintatica.txt       Especificação formal da gramática e exemplos de programas válidos
└── DOCUMENTACAO.md               Este arquivo
```

---

## 7. Exemplos de Programas Válidos

### Exemplo 1 — Fatorial iterativo

```
int fat(int n) then
    int result;
    result = 1;
    for (i = 1; i < n + 1; i = i + 1) then
        result = result * i;
    end
    return result;
end

int n;
read(n);
write(fat(n));
```

### Exemplo 2 — Soma de vetor

```
int soma(int arr[], int tam) then
    int total;
    int i;
    total = 0;
    i = 0;
    while (i < tam) then
        total = total + arr[i];
        i = i + 1;
    end
    return total;
end

int v[5];
v[0] = 10;
v[1] = 20;
v[2] = 30;
v[3] = 40;
v[4] = 50;
write(soma(v, 5));
```

### Exemplo 3 — Verificação de paridade com do-while

```
int x;
do
    read(x);
    if (x % 2 == 0) then
        write("par");
    else
        write("impar");
    end
while (x != 0);
```

### Exemplo 4 — Operações bit a bit

```
int a;
int b;
a = 0b1010;
b = 0xFF;
write(a & b);
write(a | b);
write(a ^ b);
write(a << 2);
write(b >> 1);
```

### Exemplo 5 — Expressões lógicas e relacionais

```
int x;
int y;
x = 10;
y = 20;
if (x > 0 && y > 0) then
    write("ambos positivos");
end
if (!(x == y)) then
    write("x diferente de y");
end
```
