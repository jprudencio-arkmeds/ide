.data
   vetor : 0,0,0,0,0
   i : 0
   j : 0
   aux : 0
.text
_PRINCIPAL:
   LDI     0
   STO     1000
   LDI     5
   STO     1001
   LD      1000
   STO     $indr
   LD      1001
   STOV    vetor
   LDI     1
   STO     1000
   LDI     3
   STO     1001
   LD      1000
   STO     $indr
   LD      1001
   STOV    vetor
   LDI     2
   STO     1000
   LDI     4
   STO     1001
   LD      1000
   STO     $indr
   LD      1001
   STOV    vetor
   LDI     3
   STO     1000
   LDI     2
   STO     1001
   LD      1000
   STO     $indr
   LD      1001
   STOV    vetor
   LDI     4
   STO     1000
   LDI     1
   STO     1001
   LD      1000
   STO     $indr
   LD      1001
   STOV    vetor
   LDI     0
   STO     i
   LDI     4
   STO     1000
   LDI     1
   STO     1001
   LD      i
PARA1:
   SUB     1000
   BGT     FIMPARA1
   LD      i
   ADDI    1
   STO     j
   LDI     4
   STO     1002
   LDI     1
   STO     1003
   LD      j
PARA2:
   SUB     1002
   BGT     FIMPARA2
   LD      i
   STO     $indr
   LDV     vetor
   STO     1004
   LD      j
   STO     $indr
   LDV     vetor
   STO     1005
   LD      1004
   SUB     1005
   BLE     FIMSE1
   LD      i
   STO     $indr
   LDV     vetor
   STO     aux
   LD      i
   STO     1004
   LD      j
   STO     $indr
   LDV     vetor
   STO     1005
   LD      1004
   STO     $indr
   LD      1005
   STOV    vetor
   LD      j
   STO     1004
   LD      aux
   STO     1005
   LD      1004
   STO     $indr
   LD      1005
   STOV    vetor
FIMSE1:
   LD      j
   ADD     1003
   STO     j
   JMP     PARA2
FIMPARA2:
   LD      i
   ADD     1001
   STO     i
   JMP     PARA1
FIMPARA1:
   HLT     0
