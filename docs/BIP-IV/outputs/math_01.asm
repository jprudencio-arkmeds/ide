.data
	a_41 : 0,0,0
	b_63 : 0,0
	c_86 : "dentro"
	a_307 : 0,0,0
	b_325 : 0,0
	c_344 : "fora"

.text
MAIN:
	LDI 10
	STO 1000
	LDI 0
	STO $indr
	LD 1000
	STOV a_41
	LDI 20
	STO 1000
	LDI 1
	STO $indr
	LD 1000
	STOV a_41
	LDI 0
	STO $indr
	LDV a_41
	STO 1001
	LDI 1
	STO $indr
	LDV a_41
	ADD 1001
	STO 1000
	LDI 2
	STO $indr
	LD 1000
	STOV a_41
	LDI 1.5
	STO 1000
	LDI 0
	STO $indr
	LD 1000
	STOV b_63
	LDI 0
	STO $indr
	LDV b_63
	STO 1000
	LDI 1
	STO $indr
	LD 1000
	STOV b_63
	LDI 2
	STO $indr
	LDV a_41
	STO $out_port
	LDI 1
	STO $indr
	LDV b_63
	STO $out_port
	LD c_86
	STO $out_port
	LDI 1
	STO 1000
	LDI 0
	STO $indr
	LD 1000
	STOV a_307
	LDI 2
	STO 1000
	LDI 1
	STO $indr
	LD 1000
	STOV a_307
	LDI 0
	STO $indr
	LDV a_307
	STO 1000
	LDI 2
	STO $indr
	LD 1000
	STOV a_307
	LDI 3.5
	STO 1000
	LDI 0
	STO $indr
	LD 1000
	STOV b_325
	LDI 0
	STO $indr
	LDV b_325
	ADDI 1.0
	STO 1000
	LDI 1
	STO $indr
	LD 1000
	STOV b_325
	LDI 2
	STO $indr
	LDV a_307
	STO $out_port
	LDI 1
	STO $indr
	LDV b_325
	STO $out_port
	LD c_344
	STO $out_port
	HLT 0
