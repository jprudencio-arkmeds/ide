# Documentação — IDE Compilador

## Visão geral

IDE em **C++17 + Qt6**: edita fonte na **linguagem imperativa do projeto** (não é C), tokeniza (`Lexico`), parseia LL(1) (`Sintatico`), monta AST (`AST.h`) e executa (`Interpreter`).

```
texto → Lexico::tokenize() → Sintatico::parse() → ProgramNode → Interpreter::run()
```

Erros léxicos/sintáticos aparecem com linha e coluna; a saída do `write` vai para o painel da IDE.

---

## Onde está o quê

| Área | Caminho | Função |
|------|---------|--------|
| Interface | `src/ide/MainWindow.cpp` | F5: lexico → sintático → interpretador; abas de tokens, erros, saída. |
| Editor | `src/ide/CodeEditor.*` | `QPlainTextEdit` + numeração de linhas. |
| Léxico | `src/gals/Lexico.cpp`, `Constants.h` | Tokens; **não** reconhece `#`, nem `.` em identificadores (só em `12.34`). |
| Sintático | `src/gals/Sintatico.cpp` | Parser recursivo; constrói a AST. `Semantico.cpp` é *stub* (compatível com GALS, não analisa tipos). |
| AST | `src/gals/AST.h` | Nós de programa, funções, comandos e expressões. |
| Runtime | `src/interpreter/Interpreter.cpp`, `Value.h` | Ambiente encadeado; `break`/`continue`/`return` via exceções internas; limite de passos (`MAX_STEPS`). |
| Gramática legível | `gramatica_sintatica.txt` | Especificação detalhada da linguagem. |
| GALS (ferramenta externa) | `exp.gals` | Gramática só de expressão para simulador; `exp_simulador_entrada.txt` = uma linha de teste. |

Executável CMake: **`compilador`** (não `CompilerIDE`).

---

## Linguagem (resumo)

- Blocos: **`then` … `end`**, não `{` `}`.
- Igualdade em expressões: **`==`** (token `EQUAL`); **`=`** é só atribuição.
- `for (init; cond; post) then` — separadores são **`;`**, como em C.
- E/S: **`read(x);`**, **`write(expr);`** (vários argumentos separados por **`,`**).
- Tipos: `int`, `float`, `char`, `string`, `void`; vetores `int v[10];`, parâmetro vetor `int arr[]`.

Detalhes e exemplos: `gramatica_sintatica.txt` e `exemplo.txt`.

---

## Precedência de expressões (no `Sintatico`)

Do mais fraco ao mais forte: **`||`** → **`&&`** → **`==` `!=`** → **`|`** → **`^`** → **`&`** → **`<` `>` `<=` `>=`** → **`<<` `>>`** → **`+` `-`** → **`*` `/` `%`** → unários **`!` `-` `~`** → primários.

Assim `write(a & b);` e misturas `<<` / `!=` / `||` analisam corretamente (o léxico já emitia `&`, etc.; o parser precisa consumi-los nessa ordem).

---

## Compilar e rodar

```bash
cd build && cmake .. && cmake --build .
./compilador
```

Requer **Qt6** (`Widgets`) e CMake ≥ 3.16.

---

## Dúvidas frequentes (código)

**Por que `#include` ou `stdio.h` dão erro léxico?**  
O analisador não implementa pré-processador C; `#` e `.` fora de literal real são `UNKNOWN`.

**Onde mudam palavras-chave e nomes de tokens?**  
`Lexico::keywordType` + enum `TokenType` em `Constants.h` / `Constants.cpp` (`tokenTypeName`).

**O que é `exp.gals` em relação ao projeto?**  
Arquivo para o **simulador GALS** (SLR). O compilador da IDE usa **`Sintatico.cpp`** manual. Mantém `==` em `<op_rel>` alinhado ao léxico; entrada de teste: `exp_simulador_entrada.txt`.

**Por que `AST.h` está em `gals/` e não em `interpreter/`?**  
Histórico GALS: parser e AST ficam juntos; o interpretador só consome `AST.h`.

**Como o `return` sai da função?**  
`ReturnSignal` capturado em `Interpreter::callFunc` após `execList` no corpo da função.

**`read` aceita o quê?**  
Linha de texto da UI; tenta inteiro, senão real, senão guarda como string (`execRead`).

**Passagem de vetor para função altera o vetor global?**  
O interpretador copia valores na chamada; mutar parâmetro `arr[]` **não** modela referência ao vetor global (limitação atual do runtime).

**Onde ajustar mensagens de erro sintático?**  
`Sintatico::expect`, `Sintatico::addError` e textos em `parse*` em `Sintatico.cpp`.

---

## Estrutura de pastas (real)

```
src/
  main.cpp
  ide/           MainWindow, CodeEditor
  gals/          Lexico, Sintatico, Semantico (stub), AST, Token, Constants, erros
  interpreter/   Interpreter, Value, Environment
gramatica_sintatica.txt
exemplo.txt
exp.gals
exp_simulador_entrada.txt
```
