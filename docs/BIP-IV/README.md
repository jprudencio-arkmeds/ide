# Processador BIP IV

## Visão Geral

O BIP IV é uma extensão do BIP III e possui as mesmas características arquiteturais. Uma das mudanças realizadas na extensão do BIP III para o BIP IV foi a antecipação de classes de instruções, de manipulação de vetores e suporte a sub-rotinas, presentes no µBIP.

---

## Resumo da Arquitetura

| Característica | Descrição |
|---|---|
| **Tamanho da palavra de dados** | 16 bits |
| **Tipos de dados** | Inteiro de 16 bits com sinal –32768 a +32767 |
| **Tamanho da palavra de instrução** | 16 bits |

### Formato de Instrução

```
| 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|--- Cód. Operação ---|------------------- Operando -------------------------|
```

- **Bits 15–11:** Código de Operação (5 bits)
- **Bits 10–0:** Operando (11 bits)

### Modos de Endereçamento

| Modo | Descrição |
|---|---|
| **Direto** | O operando é um endereço da memória |
| **Imediato** | O operando é uma constante |
| **Indireto** | O campo Operando é um endereço base de um vetor que é somado ao INDR para o cálculo de um endereço efetivo da memória de dados |

### Registradores

| Registrador | Descrição |
|---|---|
| **ACC** | Acumulador |
| **PC** | Contador de programa |
| **STATUS** | Registrador de Status |
| **INDR** | Registrador de índice |
| **SP** | Apontador do topo da pilha |

### Classes de Instrução

| Classe | Instruções |
|---|---|
| **Armazenamento** | STO |
| **Carga** | LD e LDI |
| **Aritmética** | ADD, ADDI, SUB e SUBI |
| **Lógica booleana** | AND, OR, XOR, ANDI, ORI, XORI e NOT |
| **Controle** | HLT |
| **Desvio** | BEQ, BNE, BGT, BGE, BLT, BLE e JMP |
| **Deslocamento Lógico** | SLL e SRL |
| **Manipulação de vetor** | LDV e STOV |
| **Suporte a procedimentos** | RETURN e CALL |

---

## Conjunto de Instruções

| Opcode | Instrução | Operação | Atualização do PC |
|--------|-----------|----------|-------------------|
| `00000` | **HLT** | Desabilita atualização do PC | PC ← PC |
| `00001` | **STO** operand | Memory[operand] ← ACC | PC ← PC + 1 |
| `00010` | **LD** operand | ACC ← Memory[operand] | PC ← PC + 1 |
| `00011` | **LDI** operand | ACC ← operand | PC ← PC + 1 |
| `00100` | **ADD** operand | ACC ← ACC + Memory[operand] | PC ← PC + 1 |
| `00101` | **ADDI** operand | ACC ← ACC + operand | PC ← PC + 1 |
| `00110` | **SUB** operand | ACC ← ACC - Memory[operand] | PC ← PC + 1 |
| `00111` | **SUBI** operand | ACC ← ACC - operand | PC ← PC + 1 |
| `01000` | **BEQ** operand | Se (STATUS.Z=1) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01001` | **BNE** operand | Se (STATUS.Z=0) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01010` | **BGT** operand | Se (STATUS.Z=0) e (STATUS.N=0) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01011` | **BGE** operand | Se (STATUS.N=0) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01100` | **BLT** operand | Se (STATUS.N=1) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01101` | **BLE** operand | Se (STATUS.Z=1) ou (STATUS.N=1) então PC ← endereço; Senão PC ← PC + 1 | Condicional |
| `01110` | **JMP** operand | — | PC ← endereço |
| `01111` | **NOT** | ACC ← NOT(ACC) | PC ← PC + 1 |
| `10000` | **AND** operand | ACC ← ACC AND Memory[operand] | PC ← PC + 1 |
| `10001` | **ANDI** operand | ACC ← ACC AND operand | PC ← PC + 1 |
| `10010` | **OR** operand | ACC ← ACC OR Memory[operand] | PC ← PC + 1 |
| `10011` | **ORI** operand | ACC ← ACC OR operand | PC ← PC + 1 |
| `10100` | **XOR** operand | ACC ← ACC XOR Memory[operand] | PC ← PC + 1 |
| `10101` | **XORI** operand | ACC ← ACC XOR operand | PC ← PC + 1 |
| `10110` | **SLL** operand | ACC ← ACC << operand | PC ← PC + 1 |
| `10111` | **SRL** operand | ACC ← ACC >> operand | PC ← PC + 1 |
| `11000` | **STOV** operand | Memory[operand + INDR] ← ACC | PC ← PC + 1 |
| `11001` | **LDV** operand | ACC ← Memory[operand + INDR] | PC ← PC + 1 |
| `11010` | **RETURN** | — | PC ← ToS |
| `11011` | — | Não utilizada | — |
| `11100` | **CALL** operand | ToS ← PC + 1 | PC ← operand |

---

## Detalhes das Instruções

### Instruções de Armazenamento e Carga

- **STO** (Store): Armazena o valor do acumulador na posição de memória indicada pelo operando.
- **LD** (Load): Carrega no acumulador o valor armazenado na posição de memória indicada pelo operando.
- **LDI** (Load Immediate): Carrega no acumulador o valor imediato do operando.

### Instruções Aritméticas

- **ADD**: Soma o conteúdo do acumulador com o valor da memória no endereço do operando.
- **ADDI**: Soma o conteúdo do acumulador com o valor imediato do operando.
- **SUB**: Subtrai do acumulador o valor da memória no endereço do operando.
- **SUBI**: Subtrai do acumulador o valor imediato do operando.

### Instruções de Desvio

- **BEQ** (Branch if Equal): Desvia se o flag Z (zero) estiver ativo.
- **BNE** (Branch if Not Equal): Desvia se o flag Z não estiver ativo.
- **BGT** (Branch if Greater Than): Desvia se Z=0 e N=0.
- **BGE** (Branch if Greater or Equal): Desvia se N=0.
- **BLT** (Branch if Less Than): Desvia se N=1.
- **BLE** (Branch if Less or Equal): Desvia se Z=1 ou N=1.
- **JMP** (Jump): Desvio incondicional.

### Instruções Lógicas

- **NOT**: Complemento bit a bit do acumulador.
- **AND / ANDI**: Operação AND entre ACC e memória/imediato.
- **OR / ORI**: Operação OR entre ACC e memória/imediato.
- **XOR / XORI**: Operação XOR entre ACC e memória/imediato.

### Instruções de Deslocamento

- **SLL** (Shift Left Logical): Deslocamento lógico à esquerda.
- **SRL** (Shift Right Logical): Deslocamento lógico à direita.

### Instruções de Manipulação de Vetor

- **STOV** (Store Vector): Armazena ACC na posição de memória calculada por operando + INDR.
- **LDV** (Load Vector): Carrega em ACC o valor da posição de memória calculada por operando + INDR.

### Instruções de Suporte a Procedimentos

- **CALL**: Empilha o endereço de retorno (PC+1) no topo da pilha (ToS) e desvia para o endereço do operando.
- **RETURN**: Desvia para o endereço armazenado no topo da pilha (ToS).

### Controle

- **HLT** (Halt): Para a execução do processador, desabilitando a atualização do PC.

---

## Registrador STATUS

O registrador STATUS contém flags atualizados pelas operações aritméticas e lógicas:

| Flag | Descrição |
|------|-----------|
| **Z** (Zero) | Ativo (1) quando o resultado da operação é zero |
| **N** (Negativo) | Ativo (1) quando o resultado da operação é negativo |
