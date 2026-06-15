.data
   fat : 0
   temp : 0
   i : 0
   j : 0
   num : 0
.text
_PRINCIPAL:
   LDI     1
   STO     fat
   LDI     0
   STO     temp
   LDI     0
   STO     i
   LDI     0
   STO     j
   LD      $in_port
   STO     num
   LDI     2
   STO     i
   LD      num
   STO     1000
   LDI     1
   STO     1001
   LD      i
PARA1:
   SUB     1000
   BGT     FIMPARA1
   LD      fat
   STO     temp
   LDI     1
   STO     j
   LD      i
   SUBI    1
   STO     1002
   LDI     1
   STO     1003
   LD      j
PARA2:
   SUB     1002
   BGT     FIMPARA2
   LD      fat
   ADD     temp
   STO     fat
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
   LD      fat
   STO     $out_port
   HLT     0
