# Guia de Geração de Código para BIP IV

Este documento descreve como gerar código assembly para o processador BIP IV. O BIP IV suporta apenas o tipo **inteiro** (16 bits com sinal, –32768 a +32767).

---

## Estrutura de um Programa BIP IV

Um programa BIP IV é dividido em duas seções:

```asm
.data
   ; declarações de variáveis e vetores
.text
   ; instruções do programa
```

- **`.data`**: Seção de declaração de variáveis e vetores.
- **`.text`**: Seção de código (instruções executáveis).

---

## 1. Declarações de Variáveis e Vetores

### 1.1 Variáveis

Variáveis são declaradas na seção `.data` com a sintaxe:

```
nome : valor_inicial
```

**Exemplo:**

```asm
.data
   fat : 0
   temp : 0
   num : 0
```

Cada variável ocupa uma posição de memória e é inicializada com o valor fornecido.

### 1.2 Vetores

Vetores são declarados na seção `.data` com múltiplos valores separados por vírgula:

```
nome : val0,val1,val2,...,valN
```

**Exemplo:**

```asm
.data
   vetor : 0,0,0,0,0
```

Isso declara um vetor de 5 posições, todas inicializadas com 0. Os elementos são acessados via endereçamento indireto usando o registrador `$indr`.

---

## 2. Entrada de Dados

A entrada de dados é realizada através da porta de entrada `$in_port`.

### 2.1 Leitura para uma variável

Para ler um valor da entrada e armazená-lo em uma variável:

```asm
LD      $in_port    ; lê valor da porta de entrada para ACC
STO     variavel    ; armazena ACC na variável
```

**Exemplo** (leitura de um número do usuário):

```asm
LD      $in_port
STO     num
```

### 2.2 Leitura para uma posição de vetor

Para ler um valor da entrada e armazená-lo em uma posição específica de um vetor:

```asm
LDI     indice      ; carrega o índice desejado
STO     $indr       ; define o registrador de índice
LD      $in_port    ; lê valor da porta de entrada
STOV    vetor       ; armazena em vetor[indice]
```

**Exemplo** (leitura para `vetor[2]`):

```asm
LDI     2
STO     $indr
LD      $in_port
STOV    vetor
```

---

## 3. Saída de Dados

A saída de dados é realizada através da porta de saída `$out_port`.

### 3.1 Saída de um valor inteiro imediato

```asm
LDI     valor       ; carrega constante inteira no ACC
STO     $out_port   ; envia para a porta de saída
```

**Exemplo** (imprime o valor 42):

```asm
LDI     42
STO     $out_port
```

### 3.2 Saída de uma variável

```asm
LD      variavel    ; carrega o valor da variável no ACC
STO     $out_port   ; envia para a porta de saída
```

**Exemplo** (imprime o valor de `fat`):

```asm
LD      fat
STO     $out_port
```

### 3.3 Saída de uma posição de vetor

```asm
LDI     indice      ; carrega o índice
STO     $indr       ; define o registrador de índice
LDV     vetor       ; carrega vetor[indice] no ACC
STO     $out_port   ; envia para a porta de saída
```

**Exemplo** (imprime `vetor[3]`):

```asm
LDI     3
STO     $indr
LDV     vetor
STO     $out_port
```

---

## 4. Atribuições

### 4.1 Atribuição de constante a variável

Para atribuir um valor imediato (constante) a uma variável:

```asm
LDI     valor
STO     variavel
```

**Exemplo** (`fat = 1`):

```asm
LDI     1
STO     fat
```

### 4.2 Atribuição de variável a variável

Para copiar o valor de uma variável para outra:

```asm
LD      origem
STO     destino
```

**Exemplo** (`temp = fat`):

```asm
LD      fat
STO     temp
```

### 4.3 Atribuição de constante a posição de vetor

Para atribuir um valor imediato a uma posição de um vetor:

```asm
LDI     indice
STO     $indr
LDI     valor
STOV    vetor
```

**Exemplo** (`vetor[0] = 5`):

```asm
LDI     0
STO     $indr
LDI     5
STOV    vetor
```

> **Nota:** Como `LDI` carrega o valor no ACC e `STO $indr` usa o ACC, é necessário separar a carga do índice e a carga do valor. Uma forma alternativa usando endereços temporários:

```asm
LDI     0           ; índice
STO     1000        ; salva índice em temporário
LDI     5           ; valor
STO     1001        ; salva valor em temporário
LD      1000        ; recupera índice
STO     $indr       ; define índice
LD      1001        ; recupera valor
STOV    vetor       ; vetor[0] = 5
```

### 4.4 Atribuição de variável a posição de vetor

```asm
LDI     indice
STO     $indr
LD      variavel
STOV    vetor
```

**Exemplo** (`vetor[2] = aux`):

```asm
LDI     2
STO     $indr
LD      aux
STOV    vetor
```

### 4.5 Atribuição de posição de vetor a variável

```asm
LDI     indice
STO     $indr
LDV     vetor
STO     variavel
```

**Exemplo** (`aux = vetor[3]`):

```asm
LDI     3
STO     $indr
LDV     vetor
STO     aux
```

### 4.6 Atribuição entre posições de vetor

Para copiar `vetor[i]` para `vetor[j]`, é necessário usar uma variável ou endereço temporário:

```asm
LD      i
STO     $indr
LDV     vetor       ; ACC = vetor[i]
STO     1005        ; salva em temporário
LD      j
STO     $indr
LD      1005        ; recupera valor
STOV    vetor       ; vetor[j] = vetor[i]
```

---

## 5. Operações Aritméticas e Bit a Bit

O BIP IV utiliza o acumulador (ACC) como operando implícito em todas as operações. O resultado sempre fica em ACC.

### 5.1 Soma

#### Soma com variável (`ACC = ACC + Memory[var]`)

```asm
LD      a           ; ACC = a
ADD     b           ; ACC = a + b
STO     resultado   ; resultado = a + b
```

#### Soma com constante (`ACC = ACC + imediato`)

```asm
LD      a           ; ACC = a
ADDI    5           ; ACC = a + 5
STO     resultado   ; resultado = a + 5
```

#### Soma com posição de vetor

Como `ADD` usa endereçamento direto, para somar com um elemento de vetor é necessário carregar o elemento antes:

```asm
; resultado = a + vetor[i]
LDI     2
STO     $indr
LDV     vetor       ; ACC = vetor[2]
ADD     a           ; ACC = vetor[2] + a
STO     resultado
```

### 5.2 Subtração

#### Subtração com variável (`ACC = ACC - Memory[var]`)

```asm
LD      a           ; ACC = a
SUB     b           ; ACC = a - b
STO     resultado   ; resultado = a - b
```

#### Subtração com constante (`ACC = ACC - imediato`)

```asm
LD      a           ; ACC = a
SUBI    3           ; ACC = a - 3
STO     resultado   ; resultado = a - 3
```

#### Subtração com posição de vetor

```asm
; resultado = vetor[i] - vetor[j]
LD      i
STO     $indr
LDV     vetor       ; ACC = vetor[i]
STO     1004        ; temporário = vetor[i]
LD      j
STO     $indr
LDV     vetor       ; ACC = vetor[j]
STO     1005        ; temporário = vetor[j]
LD      1004        ; ACC = vetor[i]
SUB     1005        ; ACC = vetor[i] - vetor[j]
STO     resultado
```

### 5.3 Operações Bit a Bit

#### AND com variável

```asm
LD      a
AND     b           ; ACC = a AND b
STO     resultado
```

#### AND com constante (imediato)

```asm
LD      a
ANDI    0xFF        ; ACC = a AND 0xFF
STO     resultado
```

#### OR com variável

```asm
LD      a
OR      b           ; ACC = a OR b
STO     resultado
```

#### OR com constante (imediato)

```asm
LD      a
ORI     0x0F        ; ACC = a OR 0x0F
STO     resultado
```

#### XOR com variável

```asm
LD      a
XOR     b           ; ACC = a XOR b
STO     resultado
```

#### XOR com constante (imediato)

```asm
LD      a
XORI    0xFF        ; ACC = a XOR 0xFF
STO     resultado
```

#### NOT (complemento)

```asm
LD      a
NOT                 ; ACC = NOT(a)
STO     resultado
```

#### Deslocamento lógico à esquerda (SLL)

```asm
LD      a
SLL     2           ; ACC = a << 2 (multiplica por 4)
STO     resultado
```

#### Deslocamento lógico à direita (SRL)

```asm
LD      a
SRL     1           ; ACC = a >> 1 (divide por 2)
STO     resultado
```

### 5.4 Operações bit a bit com vetores

Para aplicar operações bit a bit em elementos de vetor, carregue-os para temporários primeiro:

```asm
; resultado = vetor[i] AND vetor[j]
LD      i
STO     $indr
LDV     vetor       ; ACC = vetor[i]
STO     1004        ; temporário
LD      j
STO     $indr
LDV     vetor       ; ACC = vetor[j]
STO     1005        ; temporário
LD      1004
AND     1005        ; ACC = vetor[i] AND vetor[j]
STO     resultado
```

---

## 6. Uso de Endereços Temporários

O BIP IV permite o uso de endereços numéricos (como `1000`, `1001`, etc.) como armazenamento temporário. Isso é útil quando operações complexas exigem preservar valores intermediários, já que o processador possui apenas um acumulador.

**Padrão comum:**

```asm
LDI     valor
STO     1000        ; salva em endereço temporário 1000
; ... outras operações ...
LD      1000        ; recupera o valor salvo
```

> **Convenção:** Endereços temporários geralmente começam em `1000` para não conflitar com variáveis declaradas na seção `.data`.

---

## 7. Exemplo Completo

Programa que lê dois números, calcula a soma e a diferença, e imprime os resultados:

```asm
.data
   a : 0
   b : 0
   soma : 0
   diff : 0
.text
_PRINCIPAL:
   ; Leitura de dados
   LD      $in_port
   STO     a
   LD      $in_port
   STO     b

   ; Soma: soma = a + b
   LD      a
   ADD     b
   STO     soma

   ; Subtração: diff = a - b
   LD      a
   SUB     b
   STO     diff

   ; Saída dos resultados
   LD      soma
   STO     $out_port
   LD      diff
   STO     $out_port

   HLT     0
```

---

## Resumo de Padrões

| Operação | Padrão |
|----------|--------|
| `var = constante` | `LDI constante` → `STO var` |
| `var = outra_var` | `LD outra_var` → `STO var` |
| `var = a + b` | `LD a` → `ADD b` → `STO var` |
| `var = a - b` | `LD a` → `SUB b` → `STO var` |
| `var = a + constante` | `LD a` → `ADDI constante` → `STO var` |
| `var = a - constante` | `LD a` → `SUBI constante` → `STO var` |
| `var = a AND b` | `LD a` → `AND b` → `STO var` |
| `var = a OR b` | `LD a` → `OR b` → `STO var` |
| `var = a XOR b` | `LD a` → `XOR b` → `STO var` |
| `var = NOT(a)` | `LD a` → `NOT` → `STO var` |
| `var = a << n` | `LD a` → `SLL n` → `STO var` |
| `var = a >> n` | `LD a` → `SRL n` → `STO var` |
| `vetor[i] = val` | `LDI i` → `STO $indr` → `LDI val` → `STOV vetor` |
| `var = vetor[i]` | `LDI i` → `STO $indr` → `LDV vetor` → `STO var` |
| `entrada → var` | `LD $in_port` → `STO var` |
| `var → saída` | `LD var` → `STO $out_port` |
