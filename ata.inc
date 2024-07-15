;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - ATA I/O routines			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022 Meido-Tek Productions					:
;								:
; Main file: latabios.asm					:
;---------------------------------------------------------------:

;---------------------------------------------------------------:
; =GetIO							:
;	Obtain the base address of the ATA channel selected	:
;	by ES:PORT_OFF.						:
;								:
; Assumed: DS=CS(ROM), ES:PORT_OFF=<drive numer>		:
;								:
; Returns:							:
;	DX	I/O address					:
;---------------------------------------------------------------:
GetIO:
	SUB	DH,DH			; Zero out upper byte of DX
	MOV	DL,ES:PORT_OFF		; Get current disk number
	AND	DL,06H			; Mask off master/slave bit
	PUSH	SI			; Load base I/O address
	MOV	SI,ATA_BASE
	ADD	SI,DX
	MOV	DX,[CS:SI]
	POP	SI
	RET

;---------------------------------------------------------------:
; =ResetATA							:
;	Resets the specified ATA bus by toggling the SRST bit.	:
;								:
; Arguments:							:
;	DX	Base I/O address of ATA channel			:
;								:
; Assumes: None							:
;								:
; Destroys: None						:
;---------------------------------------------------------------:
ResetATA:
.baseIO		EQU	-2
	PUSH	BP
	MOV	BP,SP
	SUB	SP,2
	PUSH	AX
	PUSH	DX
	PUSH	CX
;-----	Perform software reset
	AND	DH,0FH
	MOV	[BP+.baseIO],DX		; Store base address for later recalls
	ADD	DX,ATA_CT_DCTRL		; Adjust to control port
	MOV	AL,DC_SRST		; Set software reset bit
	OUT	DX,AL
	MOV	DX,[BP+.baseIO]		; Get alternate status register
	ADD	DX,ATA_CT_ASTAT
	MOV	CX,60			; Delay by reading port multiple times
.resetwait:
	IN	AL,DX
	LOOP	.resetwait
	MOV	DX,[BP+.baseIO]		; Get control port
	ADD	DX,ATA_CT_DCTRL
	SUB	AL,AL			; Clear SRST bit
	OUT	DX,AL
;-----	Post reset
%if 0
	MOV	AH,086H			; Delay 256 just in case
	MOV	CX,001H
	SUB	DX,DX
	INT	015H
%endif
	POP	CX
	POP	DX
	POP	AX
	ADD	SP,2
	POP	BP
	RET

;---------------------------------------------------------------:
; =SetCHS							:
;	Applies CHS parameters to the hard drive specified by	:
;	global variable ES:PORT_OFF and additionally negotiates	:
;	drive selection.					:
;								:
;	The arguments are arranged such that untranslated CHS	:
;	parameters from INT 13h calls can be rearranged	with	:
;	few instructions possible:				:
;								:
;	AH = Function number (unneeded)				:
;	AL = Number of sectors					:
;	CH = Low eight bits of cylinder				:
;	CL = Bits 0-5: Sector number				:
;	     Bits 6-7: Upper two bits of cylinder		:
;	DH = Head number					:
;								:
;	MOV	AH,CL	; Use AH as sector number		:
;	AND	AH,03FH	; Mask off upper bits for cylinder	:
;	SHR	CL,6	; Shift upper 2 bits of cylinder down	:
;	XCHG	CH,CL	; Exchange to correct byte order	:
;								:
; Arguments:							:
;	AL	Number of sectors (1-255, 0=256)		:
;	AH	Sector number (1-63)				:
;	CX	Cylinder number	(0-65535)			:
;	DH	Head number (0-15)				:
;								:
; Assumes: DS=CS(ROM), ES=BDA_SEG, ES:PORT_OFF=<drive number>	:
;								:
; Destroys: None						:
;								:
; Returns:							:
;	DX	Base I/O address of ATA channel			:
;	CF	Set on open-bus (ie. no disk)			:
;---------------------------------------------------------------:
SetCHS:
.head	EQU -1
.bplen	EQU 2
	PUSH	BP
	MOV	BP,SP
	SUB	SP,.bplen
	PUSH	AX
	MOV	[BP+.head],DH		; Save head number
	CALL	GetIO			; Get I/O address
	AND	DH,00FH			; Mask off IRQ number
;-----	Set drive/head register
	PUSH	AX
	ADD	DX,ATA_DHEAD		; Adjust to drive/head register
	MOV	AL,[BP+.head]		; Get head value
	AND	AL,0FH			; Mask off upper 4-bits just in case
	OR	AL,0A0H			; Set unused bits of register
	MOV	AH,ES:PORT_OFF		; Get current disk number
	AND	AH,01H			; Mask out channel number
%ifndef TARGET_XT
	SHL	AH,4			; Shift lone bit to drive select
%else
	SHL	AH,1
	SHL	AH,1
	SHL	AH,1
	SHL	AH,1
%endif	; TARGET_XT
	OR	AL,AH
	OUT	DX,AL			; Set new drive/head select value
	OUT	DX,AL			; Set new drive/head select value
	MOV	AH,AL			; Save register value
;-----	Drive select delay
	ADD	DX,ATA_STAT-ATA_DHEAD
	PUSH	CX
	MOV	CX,16			; Loop 16 times
.dseldelay:
	IN	AL,DX			; Read alternate status register
	LOOP	.dseldelay		; ... as a crude delay
	POP	CX
;-----	Set sector count, number and cylinders
	ADD	DX,ATA_FEAT-ATA_STAT
	MOV	AL,0FFH			; Set WPCOMP (FFH for none)
	OUT	DX,AL
	POP	AX			; Set sector count register
	INC	DX
	OUT	DX,AL
	INC	DX			; Set sector number register
	MOV	AL,AH
	OUT	DX,AL			
	INC	DX			; Set cylinder low
	MOV	AL,CL
	OUT	DX,AL
	INC	DX			; Set cylinder high
	MOV	AL,CH
	OUT	DX,AL
	SUB	DX,ATA_CYLH
	POP	AX
	ADD	SP,.bplen
	POP	BP
	RET

;---------------------------------------------------------------:
; =SendCMD							:
;	Send a command to the ATA device selected by		:
;	ES:PORT_OFF. For master/slave drives to be selected	:
;	properly, SetCHS must be called prior			:
;								:
; Arguments:							:
;	AL	Command						:
;								:
; Assumes: ES=BDA_SEG, ES:PORT_OFF=<drive number>		:
;								:
; Destroys: None						:
;								:
; Returns:							:
;	DX	I/O port of ATA channel			:
;---------------------------------------------------------------:
SendCMD:
	PUSH	AX
;-----	Get base address of ATA channel
	SUB	DH,DH
	MOV	DL,[ES:PORT_OFF]	; Get port offset
	AND	DL,06H			; Mask off master/slave bit
;-----	Set flag on ATA channel
	PUSH	CX
	SHR	DL,1
	MOV	CL,DL
%ifndef DONT_USE_IRQ
	MOV	AL,1			; Set IRQ pending bit
	SHL	AL,CL
	OR	ES:ATA_STATUS,AL
%endif	; DONT_USE_IRQ
	POP	CX
;-----	Issue command
	CALL	GetIO			; Get IO port
	AND	DH,00FH			; Mask off IRQ number
	ADD	DX,ATA_CMD		; Adjust address to command register
	POP	AX
	OUT	DX,AL			; Write command
	SUB	DX,ATA_CMD
	RET

;---------------------------------------------------------------:
; =ReadBLK							:
;	Read a data block from the hard drive via PIO.		:
;								:
; Arguments:							:
;	DX	Base I/O address (ie. 01F0H)			:
;	ES:DI	Pointer to store read block (DI updated)	:
;								:
; Destroys: AX,CX						:
;---------------------------------------------------------------:
ReadBLK:
	CLD
	ADD	DX,ATA_STAT		; Adjust to status register
	IN	AL,DX
.waitdrq:				; Wait for DRQ to become set
	IN	AL,DX
	TEST	AL,STAT_DRQ
	JZ	.waitdrq
	SUB	DX,ATA_STAT		; Adjust to data port
	MOV	CX,0100H		; 256 words, 512 bytes
%ifndef TARGET_XT
	REP	INSW			; Fetch the block
%else
;-----	Two 8-bit reads from 16-bit expander
.blkloop:
	IN	AL,DX			; Read lo-byte- asserts ATA /RD signal
	MOV	AH,AL
	ADD	DX,08H
	IN	AL,DX			; Then hi-byte
	XCHG	AH,AL
	STOSW
	ADD	DX,-08H
	DEC	CX
	JNZ	.blkloop
%endif	; TARGET_XT
	RET

;---------------------------------------------------------------:
; =WriteBLK							:
;	Write a data block to the hard drive via PIO.		:
;								:
; Arguments:							:
;	DX	Base I/O address (ie. 01F0H)			:
;	DS:SI	Source pointer for data block (SI updated)	:
;								:
; Destroys: AX,CX						:
;---------------------------------------------------------------:
WriteBLK:
	CLD
	ADD	DX,ATA_STAT		; Adjust returned port to status reg.
.drqwait:				; Wait for data request
	IN	AL,DX
	TEST	AL,STAT_DRQ
	JZ	.drqwait
	SUB	DX,ATA_STAT		; Adjust back to data port
	MOV	CX,0100H		; 256 words, 512 bytes
%ifndef TARGET_XT
	REP	OUTSW			; Write the block
%else
;-----	Two 8-bit writes to 16-bit expander
.blkloop:
	LODSW
	XCHG	AH,AL			; Write hi-byte first
	ADD	DX,08H
	OUT	DX,AL
	MOV	AL,AH			; Then lo-byte- asserts ATA /WR signal
	ADD	DX,-08H
	OUT	DX,AL
	DEC	CX
	JNZ	.blkloop
%endif	; TARGET_XT
	RET

;---------------------------------------------------------------:
; =WaitRDY							:
;	Wait for a drive on channel to become ready.		:
;								:
; Arguments:							:
;	BX	Timeout (seconds)				:
;								:
; Returns:							:
;	Sets ES:DISK_STATUS to TIME_OUT when timeout surpassed.	:
;---------------------------------------------------------------:
WaitRDY:
	PUSH	AX
;-----	Post a device busy
	PUSH	BX
	SUB	BH,BH			; Standard timeout
	MOV	BL,010H
	CLC				; Device busy interrupt
	MOV	AX,09000H
	INT	015H
	STI				; Just in case IRQs got cleared by OS
	POP	BX
;-----	Calculate timeout value
	PUSH	CX
	MOV	AX,18
	MUL	BX
	MOV	BX,AX
	MOV	CX,DX
	ADD	BX,ES:TIMER_LOW	
	ADC	CX,ES:TIMER_LOW+2
;-----	Get status register
	CALL	GetIO
	AND	DH,0FH
	ADD	DX,ATA_CT_ASTAT
.bsywait:
	CMP	ES:TIMER_LOW+2,CX
	JB	.cont
	CMP	ES:TIMER_LOW,BX
	JB	.cont
	MOV	BYTE [ES:DISK_STATUS],TIME_OUT	; Set timeout condition
	JMP	.done
.cont:
	IN	AL,DX
	AND	AL,STAT_BUSY
	JNZ	.bsywait
.done:
	SUB	DX,ATA_CT_ASTAT
	POP	CX
	POP	AX
	RET

;---------------------------------------------------------------:
; =WaitATA							:
;	Wait for disk interrupt to occur. Sets DISK_STATUS	:
;	to TIME_OUT if no IRQ occurred during the last disk	:
;	operation. Normally used for device detection.		:
;								:
;	The hard drive is not automatically acknowledged by	:
;	this function when an interrupt occurs.			:
;								:
; Arguments:							:
;	BL	Timeout value.					:
;								:
; Assumes: ES=CS,ES=BDA_SEG					:
;								:
; Destroys: BX							:
;								:
; Returns:							:
;	DX		Data port of selected drive		:
;	ES:DISK_STATUS	Updated to TIME_OUT on IRQ timeout	:
;---------------------------------------------------------------:
WaitATA:
	PUSH	AX
	PUSH	CX
;-----	Obtain channel number
	MOV	AL,[ES:PORT_OFF]	; Get current drive
	AND	AL,06H
	SHR	AL,1
	MOV	CL,AL
	MOV	AH,1			; Set pending bit
	SHL	AH,CL
;-----	Get timer counter addresses
	PUSH	AX
	MOV	AX,18			; Roughly 18.2Hz (timer rate)
	MUL	BX
	MOV	CX,DX			; Move upper word to DX
	ADD	AX,ES:TIMER_LOW		; Load BDA counter
	ADC	CX,ES:TIMER_LOW+2
	MOV	BX,AX
	MOV	AX,ES:TIMER_LOW
	MOV	DX,ES:TIMER_LOW+2
	POP	AX
;-----	Wait until drive responds
	CALL	GetIO			; Load I/O port
	AND	DH,0FH			; Mask out IRQ bits
	ADD	DX,ATA_CT_ASTAT		; Adjust to alternate status register
.wait:
%ifndef DONT_USE_IRQ
;-----	IRQ based wait 
	TEST	ES:ATA_STATUS,AH	; Has interrupt occurred?
	JZ	.waitdone
	CMP	ES:TIMER_LOW,BX		; Compare ticks to test timeout
	JB	.wait
	CMP	ES:TIMER_LOW+2,CX
	JB	.wait
%else
;-----	Polling based wait
	IN	AL,DX			; Read alternate status port
	TEST	AL,STAT_BUSY		; Check BSY=0
	JZ	.waitrdy		; ... then advance to data ready wait
	CMP	ES:TIMER_LOW,BX		; Compare ticks to test timeout
	JB	.wait
	CMP	ES:TIMER_LOW+2,CX
	JB	.wait
.waitrdy:
	AND	AL,STAT_ERR		; Check error condition
	JNZ	.waitdone		; Skip DRDY on error
.wait2:
	IN	AL,DX			; Read alternate status port
	TEST	AL,STAT_DRDY		; Check DRDY=1
	JNZ	.waitdone
	CMP	ES:TIMER_LOW,BX		; Compare ticks to test timeout
	JB	.wait2
	CMP	ES:TIMER_LOW+2,CX
	JB	.wait2
%endif
	MOV	BYTE ES:DISK_STATUS,TIME_OUT	; Set timeout condition
.waitdone:
	SUB	DX,ATA_CT_ASTAT		; Revert back to base address
	POP	CX
	POP	AX
	RET

;---------------------------------------------------------------:
; =IntHandler (AT-only)							:
;	Hard disk interrupt handler. The hard disk posts a	:
;	hardware interrupt when a disk operation has completed.	:
;								:
;	The handler additionally posts a busy done call on	:
;	INT 15h for multi-tasking operating systems.		:
;---------------------------------------------------------------:
%ifndef DONT_USE_IRQ
IntHandler:
	PUSH	AX
	PUSH	DS
	PUSH	ES
;-----	Setup segments
	MOV	AX,CS			; Setup ROM data segment
	MOV	DS,AX
	MOV	AX,WRK_SEG		; Setup BDA segment
	MOV	ES,AX
;-----	Turn off IRQ mask of drive
	CLI
	SUB	AH,AH			; Get drive number of current operation
	MOV	AL,[ES:PORT_OFF]
	AND	AL,06H			; Mask off master/slave bit
;-----	Clear IRQ pending bit in status
	PUSH	AX
	PUSH	CX
	SHR	AL,1
	MOV	CL,AL
	MOV	AL,1
	SHL	AL,CL
	XOR	AL,0FFH
	AND	ES:ATA_STATUS,AL
	POP	CX
	POP	AX
;-----	Issue EOI
	MOV	AL,EOI
%ifndef TARGET_XT
	OUT	INTB00,AL		; Slave 8259
%endif	; TARGET_XT
	OUT	INTA00,AL		; Then master 8259
;-----	Post interrupt then return
	POP	ES
	POP	DS
	STI
	MOV	AX,09100H		; Device post interrupt
	INT	015H
	POP	AX
	IRET
%endif	; DONT_USE_IRQ
