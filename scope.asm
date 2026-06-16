.data
	a_209 : 5
	a_269 : 10
	a_450 : 1
	a_486 : 99
	v_669 : 0,0,0
	v_718 : 42
	a_912 : 1
	a_912 : 2
	a_1075 : 5
	a_1075 : 0,0,0

.text
MAINA:
	LD a_209
	STO $out_port
	LD a_269
	STO $out_port
MAINB:
	LD a_486
	STO $out_port
	LD a_450
	STO $out_port
MAINC:
	LDI 7
	STO 1000
	LDI 0
	STO $indr
	LD 1000
	STOV v_669
	LD v_718
	STO $out_port
	LDI 0
	STO $indr
	LDV v_669
	STO $out_port
MAIND:
MAINE:
	HLT 0
