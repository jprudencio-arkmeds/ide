.data
   multiplica_i : 0
   multiplica_result : 0
   multiplica_a : 0
   multiplica_c : 0
   j : 0
   k : 0
.text
   JMP     _PRINCIPAL
_MULTIPLICA:
   LDI     0
   STO     MULTIPLICA_result
   LDI     1
   STO     MULTIPLICA_i
   LD      MULTIPLICA_c
   STO     1000
   LDI     1
   STO     1001
   LD      MULTIPLICA_i
PARA1:
   SUB     1000
   BGT     FIMPARA1
   LD      MULTIPLICA_result
   ADD     MULTIPLICA_a
   STO     MULTIPLICA_result
   LD      MULTIPLICA_i
   ADD     1001
   STO     MULTIPLICA_i
   JMP     PARA1
FIMPARA1:
   LD      MULTIPLICA_result
   RETURN  0
_PRINCIPAL:
   LDI     3
   STO     k
   LDI     2
   STO     j
   LD      k
   STO     MULTIPLICA_a
   LD      j
   STO     MULTIPLICA_c
   CALL    _MULTIPLICA
   STO     k
   LD      k
   STO     $out_port
   HLT     0
