;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - Utility functions			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022 Meido-Tek Productions					:
;								:
; Main file: latabios.asm					:
;---------------------------------------------------------------:

;---------------------------------------------------------------:
; =DivLong16							:
;	Performs 32-bit division of two register pairs with a	:
; 16-bit divisor.						:
;								:
; Arguments:							:
;	DX:AX	32-bit dividend					:
;	BX	16-bit divisor					:
;								:
; Returns:							:
;	DX:AX	32-bit quotient					:
;	BX	16-bit remainder				:
;---------------------------------------------------------------:
DivLong16.temp	EQU -2
DivLong16:
	PUSH	BP			; Save base pointer and set it to
	MOV	BP,SP			; ... the current stack frame
	SUB	SP,2			; Allocate space for temporaries
	PUSH	AX			; Save LSB for later
	MOV	AX,DX			; Divide MSB of dividend
	SUB	DX,DX			; Zero out DX
	DIV	BX			; Divide
	MOV	[BP+DivLong16.temp],AX	; Save MSB result of quotient
	POP	AX			; Pop the LSB of dividend
	DIV	BX			; ... and divide against divisor
	MOV	BX,DX			; Put remainder to DX
	MOV	DX,[BP+DivLong16.temp]	; Get MSB of dividend from temp
	ADD	SP,2			; Free stack and restore base pointer
	POP	BP
	RET

;---------------------------------------------------------------:
; =PrintStr							:
;	Prints a zero-terminated string.			:
;								:
;	DS:SI	Zero-terminated string to print			:
;---------------------------------------------------------------:
PrintStr:
	PUSH	AX
	PUSH	BX
	PUSH	SI
	CLD
	SUB	BX,BX
	MOV	AH,0EH
.printLoop:
	LODSB
	OR	AL,AL
	JZ	.printExit
	INT	010H
	JMP	.printLoop
.printExit:
	POP	SI
	POP	BX
	POP	AX
	RET

;---------------------------------------------------------------:
; =PrintLong							:
;	Prints a 32-bit value in a pair of 16-bit registers.	:
;								:
; Arguments:							:
;	DX:AX	32-bit value					:
;---------------------------------------------------------------:
PrintLong:
	PUSH	BP
	MOV	BP,SP
	SUB	SP,12			; Allocate 12 character string buffer
	PUSH	ES
	PUSH	DI
	PUSH	AX
	PUSH	DX
	PUSH	BX
	PUSH	CX
	MOV	DI,BP			; Set ES:DI pointer to buffer in BP
	DEC	DI
	MOV	BX,SS
	MOV	ES,BX
	STD				; Make STOSB decrement pointer
	PUSH	AX			; Write terminating byte
	SUB	AL,AL
	STOSB
	MOV	AL,'0'			; Store a zero without decrementing
	MOV	[ES:DI],AL		; ... in case the value is zero
	POP	AX
	SUB	CX,CX			; Flag to indicate if input was non-zero
.strloop:
	AND	DX,DX			; Test if value has become zero
	JNZ	.getdigit		; ... and exit the loop otherwise
	AND	AX,AX
	JZ	.doprint
.getdigit:
	SUB	BH,BH
	MOV	BL,10			; Divide value by 10 to 'shift' the
	CALL	DivLong16		; ... decimal numbers left and obtain
	PUSH	AX			; ... the last digit in the remainder
	MOV	AL,BL			; Grab remainder
	ADD	AL,'0'			; Turn single base 10 value to ASCII
	STOSB				; ... and store to string buffer
	POP	AX
	MOV	CL,1
	JMP	.strloop
.doprint:
	ADD	DI,CX
	JNZ	.skipinc
	INC	DI
.skipinc:
	XCHG	SI,DI			; Swap pointers around for printing
	MOV	BX,DS
	MOV	AX,ES
	MOV	DS,AX
	CALL	PrintStr		; Print the generated string of numbers
	MOV	DS,BX
	XCHG	SI,DI
.end:
	POP	CX
	POP	BX
	POP	DX
	POP	AX
	POP	DI
	POP	ES
	ADD	SP,12
	POP	BP
	RET

;---------------------------------------------------------------:
; =PrintNum							:
;	Prints a 16-bit value.					:
;								:
; Arguments:							:
;	AX	16-bit value to print				:
;	CX	Number of space characters			:
;---------------------------------------------------------------:
PrintNum.val	EQU	-2
PrintNum.space	EQU	-4
PrintNum:
	PUSH	BP
	MOV	BP,SP
	SUB     SP,4
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	MOV	[BP+.val],AX
	AND	CX,CX
	JZ	.nopredec
	DEC	CX
.nopredec:
	MOV	[BP+.space],CX
 	SUB	CX,CX
	MOV	BX,10
.countdigits:
	SUB	DX,DX
	DIV	BX
	AND	AX,AX			; Keep dividing until result is zero
	JZ	.countdone
	INC	CL			; Count the iteration
	JMP	.countdigits
.countdone:
	MOV	AL,DL
	ADD	AL,'0'
	PUSH	BX
	SUB	BX,BX
	MOV	AH,0EH
	INT	010H
	POP	BX
	AND	CL,CL
	JZ	.done
	MOV	CH,CL
.numloop:
	MOV	AX,[BP+PrintNum.val]
.numdivloop:
	SUB	DX,DX
	DIV	BX
	DEC	CL
	JNZ	.numdivloop
	PUSH	BX
	SUB	BX,BX
	MOV	AH,0EH
	MOV	AL,DL
	ADD	AL,'0'
	INT	010H
	POP	BX
;-----	Print space padding
	MOV	AL,[BP+.space]
	AND	AL,AL
	JZ	.nospacedec
	DEC	BYTE [BP+.space]
.nospacedec:
	DEC	CH
	MOV	CL,CH
	JNZ	.numloop
.done:
	MOV	CX,[BP+.space]
	AND	CX,CX
	JZ	.nospaces
	MOV	AH,0EH
	MOV	AL,' '
	SUB	BX,BX
.spaceloop:
	INT	010H
	LOOP	.spaceloop
.nospaces:
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	ADD	SP,4
	POP	BP
	RET

;---------------------------------------------------------------:
; =PrintHex							:
;								:
;	AX	16-bit word to print				:
;	CL	Number of digits to print			:
;---------------------------------------------------------------:
PrintHex:
	PUSH	BX
	PUSH	CX
	SUB	BX,BX
.loopRep:
	PUSH	AX
	MOV	CH,CL
	DEC	CH
	JZ	.skipLoop
.divLoop:
	SHR	AX,4
	DEC	CH
	JNZ	.divLoop
.skipLoop:
	AND	AX,00FH
	CMP	AX,10
	JGE	.printAF
	ADD	AL,'0'
	MOV	AH,0EH
	INT	010H
	JMP	.loopCont
.printAF:
	SUB	AL,10
	ADD	AL,'A'
	MOV	AH,0EH
	INT	010H
.loopCont:
	POP	AX
	DEC	CL
	JNZ	.loopRep
	POP	CX
	POP	BX
	RET

;---------------------------------------------------------------:
; =RegDump							:
;	Prints out the values of all register values.		:
;	Used to provide basic debugging.			:
;---------------------------------------------------------------:
%if 0
RegDump:
	PUSH	DS
	PUSH	AX
	PUSH	SI
	PUSH	CX
	PUSH	DS
	PUSH	SI
	PUSH	CX
	MOV	CX,CS
	MOV	DS,CX
	MOV	CX,4
	MOV	SI,strAX		; AX
	CALL	PrintStr
	CALL	PrintHex
	MOV	SI,strBX		; BX
	CALL	PrintStr
	MOV	AX,BX
	CALL	PrintHex
	MOV	SI,strCX		; CX
	CALL	PrintStr
	POP	AX
	CALL	PrintHex
	MOV	SI,strDX		; DX
	CALL	PrintStr
	MOV	AX,DX
	CALL	PrintHex
	MOV	SI,strCRLF		; Newline
	CALL	PrintStr
	MOV	SI,strSI		; SI
	CALL	PrintStr
	POP	AX
	CALL	PrintHex
	MOV	SI,strDI		; DI
	CALL	PrintStr
	MOV	AX,DI
	CALL	PrintHex
	MOV	SI,strSP		; SP
	CALL	PrintStr
	MOV	AX,SP
	CALL	PrintHex
	MOV	SI,strBP		; BP
	CALL	PrintStr
	MOV	AX,BP
	CALL	PrintHex
	MOV	SI,strCRLF		; Newline
	CALL	PrintStr
	MOV	SI,strCS		; CS
	CALL	PrintStr
	MOV	AX,CS
	CALL	PrintHex
	MOV	SI,strDS		; DS
	CALL	PrintStr
	POP	AX
	CALL	PrintHex
	MOV	SI,strES		; ES
	CALL	PrintStr
	MOV	AX,ES
	CALL	PrintHex
	MOV	SI,strSS		; SS
	CALL	PrintStr
	MOV	AX,SS
	CALL	PrintHex
	MOV	SI,strCRLF		; Newline
	CALL	PrintStr
	POP	CX
	POP	SI
	POP	AX
	POP	DS
	RET

;-----	Register dump strings

strAX:		DB 'AX=',00H
strBX:		DB ' BX=',00H
strCX:		DB ' CX=',00H
strDX:		DB ' DX=',00H
strSI:		DB 'SI=',00H
strDI:		DB ' DI=',00H
strSP:		DB ' SP=',00H
strBP:		DB ' BP=',00H
strCS:		DB 'CS=',00H
strDS:		DB ' DS=',00H
strES:		DB ' ES=',00H
strSS:		DB ' SS=',00H

%endif
