;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - Main assembler file			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022 Meido-Tek Productions					:
;								:
; Assembler: NASM 2.11						:
;								:
; To assemble, invoke NASM in the command-line as follows:	:
;								:
;   nasm -f bin -o latabios.bin latabios.asm			:
;								:
; Then, run the output file through romfixup:			:
;								:
;   romfixup latabios.bin					:
;								:
; The romfixup tool fixes up the ROM image to produce a valid	:
; checksum, otherwise the system BIOS will reject it as a	:
; faulty ROM.							:
;---------------------------------------------------------------:

%define		VERSION '1.00'

;-----	User configurable constants  ---------------------------

; Uncomment to include a bootloader for
; older systems to boot from hard disk
;
;%define BOOTLOADER

; Uncomment to produce a 'sensible' build of
; L-ATA that only supports one ATA channel
; with two drives, but may fix compatibility
; issues on some OEM systems
;
;%define SENSIBLE

; Uncomment to produce an unbranded and
; de-attributed ROM build which may come out
; more popular when pre-built binaries are
; passed around with this set, because
; Internet culture
;
;%define DEBRAND

; Cylinder count threshold for
; when to apply sector translation
;
TRNS_CYL	EQU 1152

; Time out values in seconds
;
DETECT_TIMEOUT	EQU 2			; Drive detection timeout
DISK_TIMEOUT	EQU 10			; Standard disk timeout
POST_DELAY	EQU 2			; Post-screen delay

;---------------------------------------------------------------:
; 8259 constants						:
;---------------------------------------------------------------:

INTA00		EQU 020H		; Master 8259 control
INTA01		EQU 021H		; Master 8259 data
INTB00		EQU 0A0H		; Slave 8259 control
INTB01		EQU 0A1H		; Slave 8259 data

EOI		EQU 020H		; 8259 End-of-Interrupt command

	%INCLUDE 'disk.inc'		; Include ATA interface definitions

;---------------------------------------------------------------:
; Interrupt vectors						:
;---------------------------------------------------------------:

DISK_VEC	EQU 013H*4		; BIOS disk functions vector
BOOT_VEC	EQU 019H*4		; Bootstrap loader vector
FLOPPY_VEC	EQU 040H*4		; Original BIOS disk functions vector

;---------------------------------------------------------------:
; Disk BIOS data structures					:
;---------------------------------------------------------------:

STRUC		diskparm		; Disk parameter table
.cyls		RESW 1			; Cylinders
.heads		RESB 1			; Heads per cylinder
.sects		RESB 1			; Sectors per track
.len		RESB 0			; Length of structure
ENDSTRUC	; 4 bytes

;---------------------------------------------------------------:
; L-ATA workspace at 0030:0000					:
;---------------------------------------------------------------:

%ifndef SENSIBLE
MAX_DRV		EQU 8			; Maximum number of drives
%else
MAX_DRV		EQU 2
%endif

WRK_SEG		EQU 030H		; Unused portion of BIOS stack area

SECTION		bsa nobits

%ifndef SENSIBLE
		ABSOLUTE 00H
%else
		ABSOLUTE 01E0H
%endif
WRK_START:

PARM_TBL	RESB diskparm.len*MAX_DRV	; BIOS reported parameters
DPRM_TBL	RESB diskparm.len*MAX_DRV	; Drive reported parameters
%ifndef SENSIBLE
DMAP_TBL	RESD MAX_DRV		; Drive map and relocation table
					;   bits 0-23	: Starting track
					;   bits 24-26	: Physical drive num.
					;   bit  27	: 1=Translated+trackoff
					;   bit  28	: 1=Use LBA addressing
					;   bits 29-31	: Reserved
PHYS_NUM	RESB 1			; Physical drive count (incl. checksum)
%else
DRV_FLAG	RESB MAX_DRV		; Drive flags
					;   bits 0-2	: Unused
					;   bit  3	: 1=Translated+trackoff
					;   bit  4	: 1=Use LBA addressing
					;   bits 5-7	: Reserved
%endif

WRK_END:

ATA_STATUS	RESB 1			; ATA internal status byte
					;   bits 0-3	: IRQ pending bit
					;   bit  4	: Corrected data
					;   bits 5-7	: Reserved

CKSUM		RESB 1			; Checksum of parameter tables

WRK_LEN		EQU WRK_END-WRK_START

;---------------------------------------------------------------:
; BIOS Data Area at 0030:0100 (equivalent to 0040:0000)		:
;---------------------------------------------------------------:

		ABSOLUTE 016CH
TIMER_LOW	RESW 1			; Timer low word
		ABSOLUTE 0172H
RESET_FLAG	RESW 1			; Keyboard reset underway if 1234H
		ABSOLUTE 0174H		; Disk controller variables
DISK_STATUS	RESB 1			; Fixed disk last status byte
DISK_NUM	RESB 1			; Number of fixed disks
CONTROL_BYTE	RESB 1			; Last control byte
PORT_OFF	RESB 1			; Used as selected drive number 

SECT_BUFF	EQU 0200H		; Sector buffer area (for detection)

;---------------------------------------------------------------:
; Status code definitions					:
;---------------------------------------------------------------:

SENSE_FAIL	EQU 0FFH
WRITE_FAULT	EQU 0CCH
UNDEF_ERR	EQU 0BBH
TIME_OUT	EQU 080H
BAD_SEEK	EQU 040H
BAD_CNTLR	EQU 020H
DATA_CORRECTED	EQU 011H
BAD_ECC		EQU 010H
BAD_TRACK	EQU 00BH
DMA_BOUNDARY	EQU 009H
INIT_FAIL	EQU 007H
BAD_RESET	EQU 005H
RECORD_NOT_FND	EQU 004H
BAD_ADDR_MARK	EQU 002H
BAD_CMD		EQU 001H

;---------------------------------------------------------------:
; Start of ROM Program Code					:
;---------------------------------------------------------------:

SECTION text

	CPU	286			; 80286 instructions

	ORG	0H			; Set starting offset to zero
		
	DB	055H			; Magic bytes of BIOS header
	DB	0AAH
	DB	8			; Specify 4K ROM (4096/512 = 8)

	JMP	ROM_SETUP		; Skips the non-executable text below

BANNER:
%ifndef DEBRAND
	DB	'L-'
%endif
	DB	'ATA '
%ifndef DEBRAND
	DB	'Fixed Disk Support '
%endif
	DB	'BIOS'
%ifndef DEBRAND
	DB	' by John "Lameguy" Wilbert Villamor',0AH,0DH
	DB	'2022 Meido-Tek Productions ('
%ifndef VERSION
	DB	__?DATE?__,' ',__?TIME?__
%else
	DB	VERSION
%endif
%ifdef SENSIBLE
	DB	'-S'
%endif
	DB	')',0AH,0DH
	DB	'Released under MPLv2'
%ifdef BOOTLOADER
	DB	0AH,0DH
	DB	'Bootloader Enabled'
%endif
%endif
	DB	0AH,0DH,00H

	ALIGN	2

;---------------------------------------------------------------:
; =ROM_SETUP							:
;	Standard entry-point of ROM program. BIOS executes this	:
;	function during POST.					:
;---------------------------------------------------------------:
ROM_SETUP:
	MOV	AX,CS			; Use code segment as data segment
	MOV	DS,AX
	MOV	SI,BANNER		; Print ROM banner
	CALL	PrintStr
;-----	Print ROM segment
	SUB	BX,BX			; Print opening parenthesis
	MOV	SI,strROMSEG
	CALL	PrintStr
	MOV	AX,CS			; Print the ROM's code segment
	MOV	CL,4
	CALL	PrintHex
	MOV	SI,strCRLF		; Produce whitespace between banner and
	CALL	PrintStr		; ... messages that would follow
	CALL	PrintStr
;-----  Initial preparations
	MOV	BX,WRK_SEG		; Set ES to point at work segment
	MOV	ES,BX
	CALL	CalcCSums		; Check if a previous instance has
	CMP	AL,ES:CKSUM		; been installed (in case of mirroring)
	JNE	.noprev
	MOV	SI,strPrev
	CALL	PrintStr
	JMP	.done
.noprev:
;-----	Check if drive count is zero
	MOV	AL,[ES:DISK_NUM]	; Fixed disk count
	AND	AL,AL
	JZ	.nodiskbios
	MOV	SI,strBIOSpresent
	CALL	PrintStr
	JMP	.done			; Exit right away
.nodiskbios:
;-----	Clear some variables
	SUB	AL,AL			; Clear some internal variables
	MOV	ES:DISK_STATUS,AL	; Disk status
%ifndef SENSIBLE
	MOV	ES:PHYS_NUM,AL		; Physical drive count
%endif
;-----	Scan, initialize and prepare drives
	CALL	ScanDrives
	MOV	SI,strScanDone
	CALL	PrintStr
;-----	Hook INT 13H vector
	MOV	AL,ES:DISK_NUM		; Check number of drives tallied
	AND	AL,AL			; Skip vector hook if no drives found
	JZ	.nodrives		
	PUSH	ES
	SUB	BX,BX
	MOV	ES,BX
	MOV	AX,ES:DISK_VEC		; Copy original disk vector
	MOV	ES:FLOPPY_VEC,AX	; ... to INT 40h
	MOV	AX,ES:DISK_VEC+2
	MOV	ES:FLOPPY_VEC+2,AX
	MOV	AX,DISK_IO		; Set own disk function
	MOV	ES:DISK_VEC,AX
	MOV	AX,CS
	MOV	ES:DISK_VEC+2,AX
;----	Hook bootstrap loader
%ifdef BOOTLOADER
	MOV	AX,BOOTSTRAP
	MOV	ES:BOOT_VEC,AX
	MOV	AX,CS
	MOV	ES:BOOT_VEC+2,AX
%endif
	POP	ES
;-----	Calculate and store checksum of parameter tables
	CALL	CalcCSums
	MOV	ES:CKSUM,AL
	JMP	.done
.nodrives:
	MOV	SI,strNoVect
	CALL	PrintStr
.done:
	MOV	SI,strCRLF		; Whitespace
	CALL	PrintStr
	SUB	DX,DX			; Wait for user to see the POST result
	MOV	CX,ES:TIMER_LOW	
	ADD	CX,18*POST_DELAY
	ADC	DX,0
.waitloop:
	CMP	ES:TIMER_LOW,CX
	JB	.waitloop
	CMP	ES:TIMER_LOW+2,DX
	JB	.waitloop
	RETF				; Far return back to BIOS

;---------------------------------------------------------------:
; =ScanDrives							:
;	Scans all ATA channels for hard drives present and	:
;	prepares valid drives for use. PHYS_NUM and DPRM_TBL	:
;	are updated and filled with parameters of drives found	:
;	respectively.						:
;								:
; Assumes: DS=CS, ES=BDA_SEG					:
;								:
; Destroys: AX,BX,CX,DX,SI,DI					:
;---------------------------------------------------------------:
ScanDrives:
.oldhandler	EQU	-4
.baseIO		EQU	-6
.drvfound	EQU	-7
.oldmask	EQU	-8
.bplen		EQU	8
	MOV	BP,SP
	SUB	SP,.bplen
	SUB	AL,AL			; Set port offset zero
	MOV	[ES:PORT_OFF],AL
.scanloop:				; * Start of scanning loop
	CALL	GetIO			; Get port address
	PUSH	DX			; Save value for later
	AND	DH,0FH			; Mask off IRQ number
	ADD	DX,ATA_STAT		; Adjust to status port
	IN	AL,DX			; Read port
	CMP	AL,0FFH			; Is it floating bus (all bits set)?
	POP	DX			; Restore value
	JE	.skipchan		; Skip channel if so
;-----	Print channel scanning message
	MOV	SI,strScanMsg		; Print scanning message
	CALL	PrintStr
	MOV	AX,DX
	AND	AH,0FH
	MOV	CL,3			; Display port address
	CALL	PrintHex
	MOV	SI,strIRQ		; Display IRQ
	CALL	PrintStr
	MOV	AL,DH			; Obtain the IRQ value
	SHR	AX,4
	AND	AL,0FH
	SUB	CX,CX
	CALL	PrintNum		; Print IRQ
	MOV	SI,strCRLF		; New line
	CALL	PrintStr
	MOV	[BP+.baseIO],DX		; Store I/O port
	AND	DH,0FH			; Clear IRQ number
	CALL	ResetATA		; Reset the ATA bus
;-----	Hook interrupt handler	
	MOV	DX,[BP+.baseIO]		; Load I/O port
	SUB	AH,AH			; Clear upper byte
	MOV	AL,DH			; Obtain IRQ number
	SHR	AX,4			; Adjust for the IRQ number
	SUB	AX,08H			; Adjust value to base 0 for slave 8259
	ADD	AX,070H			; Increment to slave 8259 vectors 70-77
	SHL	AX,2			; Multiply value by 4
	MOV	DI,AX			; Use result as pointer on DI
	MOV	BX,ES			; Save ES
	SUB	AX,AX			; Set ES to segment 0
	MOV	ES,AX
	MOV	AX,[ES:DI]		; Get original interrupt vector
	MOV	DX,[ES:DI+2]		; High word of vector
	MOV	[BP+.oldhandler],AX	; Save the old vector value
	MOV	[BP+.oldhandler+2],DX
	MOV	AX,IntHandler		; Set new handler
	MOV	DX,CS			; Copy code segment
	MOV	[ES:DI],AX		; Low word of new vector
	MOV	[ES:DI+2],DX		; High word of new vector
	MOV	ES,BX			; Restore ES
;-----	Enable mask bit on slave 8259
	MOV	DX,[BP+.baseIO]		; Load base address
	MOV	AL,DH			; Get IRQ number
	SHR	AL,4
	SUB	AL,08H			; Subtract to base 0 from 8 to 15
	MOV	CL,AL			; Copy value
	MOV	AH,1			; Load bit
	SHL	AH,CL			; Shift bit left by adjusted IRQ number
	XOR	AH,0FFH			; Invert bits
	IN	AL,INTB01		; Read 8259 mask port
	MOV	[BP+.oldmask],AL	; Save old mask
	AND	AL,AH			; Clear bit with mask pattern
	OUT	INTB01,AL		; Set new mask to 8259
;-----	Scan channel for master/slave drives
	SUB	CL,CL			; Zero-out drive found count
	MOV	[BP+.drvfound],CL
.probeloop:
	MOV	AL,1			; Dummy block parameters
	MOV	AH,AL
	SUB	DH,DH
	SUB	CX,CX
	CALL	SetCHS			; Apply parameters
	MOV	AL,CMD_IDENTIFY		; Issue IDENTIFY command
	CALL	SendCMD
;-----	Wait for IRQ response from command
	MOV	BYTE ES:DISK_STATUS,0	; Clear disk status
	SUB	BH,BH
	MOV	BL,DETECT_TIMEOUT	; Use shorter detection timeout
	CALL	WaitINT
	MOV	AL,ES:DISK_STATUS	; Check if timeout occurred
	AND	AL,AL
	JNZ	.nodrive		; Skip further detection if failed
;-----	Fetch and parse information from hard drive
	MOV	DI,SECT_BUFF		; Set DI to sector buffer
	MOV	CX,0100H		; 256 words, 512 bytes
	REP	INSW			; Fetch the block
	MOV	DI,SECT_BUFF
	CALL	ParseDrive		; Parse the data returned
	JC	.nodrive
	INC	BYTE [BP+.drvfound]	; Increment drives found
.nodrive:
	TEST	BYTE [ES:PORT_OFF],01H	; Check if finished probing slave
	JNZ	.probedone		; Otherwise probing is done
	INC	BYTE [ES:PORT_OFF]	; Advance to next drive
	JMP	.probeloop
.probedone:
	MOV	AL,[BP+.drvfound]	; Any valid drives found from probing?
	AND	AL,AL
	JNZ	.skipchan		; Proceed to next drive
;-----	Restore old vector and IRQ mask if no drive found
	MOV	AL,[BP+.oldmask]	; Set old IRQ mask
	OUT	INTB01,AL
	MOV	AX,[BP+.baseIO]		; Get base I/O address
	MOV	AL,AH			; Obtain IRQ number
	SHR	AL,4
	SUB	AL,8			; Adjust for base zero
	SUB	AH,AH
	ADD	AX,070H			; Increment to slave 8259 vectors 70-77
	SHL	AX,2			; Multiply by 4
	MOV	DI,AX			; Set as DI pointer
	MOV	BX,ES			; Save old ES
	SUB	AX,AX			; Set ES to segment 0
	MOV	ES,AX
	MOV	AX,[BP+.oldhandler]	; Get old vector
	MOV	DX,[BP+.oldhandler+2]
	MOV	[ES:DI],AX		; Apply to vector table
	MOV	[ES:DI+2],DX
	MOV	ES,BX			; Restore old ES
.skipchan:
	MOV	AL,[ES:PORT_OFF]
%ifndef SENSIBLE
	AND	AL,0FEH			; Mask off first bit
	ADD	AL,2			; Advance to next channel
	CMP	AL,8			; Continue loop if not all checked yet
	MOV	[ES:PORT_OFF],AL
	JB	.scanloop		; * End of scanning loop
%endif
	ADD	SP,.bplen
	RET

;---------------------------------------------------------------:
; =ParseDrive							:
;	Sub-function of ScanDrives, parses the data block	:
;	returned by a IDENTIFY command.				:
;---------------------------------------------------------------:
ParseDrive:
.cyltmp		EQU -2
.headtmp	EQU -3
.bplen		EQU 4
	PUSH	BP
	MOV	BP,SP
	SUB	SP,.bplen
;-----	Skip non fixed disk ATA drives
	MOV	AX,[ES:DI]
	TEST	AX,040H
	JZ	.nothdd
;-----	Print master/slave prefix
	MOV	SI,strM			; Load pointer to master prefix string
	TEST	BYTE ES:PORT_OFF,1
	JZ	.ismaster
	MOV	SI,strS			; Load pointer to slave prefix string
.ismaster:
	CALL	PrintStr
;-----	Swap characters in model string
	MOV	AX,ES
	MOV	BX,DS			; Preserve old DS in BX
	MOV	DS,AX			; Set ES to DS
	MOV	CX,20			; 20 words, 40 characters
	PUSH	DI			; Save DI
	MOV	DI,(SECT_BUFF+drvinfo.model)	; Load pointer adjusted
.swaploop:				; ... to model string
	MOV	AX,[ES:DI]		; Load word
	XCHG	AH,AL			; Swap bytes
	STOSW				; Store word with address increment
	LOOP	.swaploop		; Continue looping
	POP	DI
;-----	Print model string
	MOV	SI,(SECT_BUFF+drvinfo.model)	; Load adjusted pointer
	MOV	BYTE [DS:SI+36],000H	; Set zero terminator
	CALL	PrintStr		; Print model string
	MOV	DS,BX			; Restore DS
;-----	Copy parameters to physical drive table
	SUB	AH,AH
	MOV	AL,ES:PORT_OFF		; Get drive number
	AND	AL,07H
	SHL	AX,2
	MOV	SI,DPRM_TBL		; Load pointer to original params table
	ADD	SI,AX			; Apply offset from number
	MOV	AX,[ES:DI+drvinfo.numcyls]	; Fill the parameter entry
	MOV	[ES:SI+diskparm.cyls],AX
	MOV	AX,[ES:DI+drvinfo.numheads]
	MOV	[ES:SI+diskparm.heads],AL
	MOV	AX,[ES:DI+drvinfo.numsects]
	MOV	[ES:SI+diskparm.sects],AL
;-----	Report capacity in MB
	PUSH	DX
;-----	Print capacity from LBA
	TEST	WORD [ES:DI+drvinfo.diskcaps],0200H
	JZ	.nolba
	MOV	AX,[ES:DI+drvinfo.nlbatotal]	; Load total sector count
	MOV	DX,[ES:DI+drvinfo.nlbatotal+2]
	JMP	.reportdone
.nolba:
;-----	Otherwise print capacity from parameters
	MOV	AX,[ES:DI+drvinfo.numsects]
	MUL	WORD [ES:DI+drvinfo.numheads]	; Multiply sects by heads
	MUL	WORD [ES:DI+drvinfo.numcyls]	; Then finally cylinders
.reportdone:
	CALL	PrintMB			; Report total sectors in MB
	POP	DX
;-----	Initialize drive parameters
	MOV	AL,[ES:DI+drvinfo.numsects]	; Number of sectors per track
	SUB	AH,AH	
	MOV	DH,[ES:DI+drvinfo.numheads]	; Heads per cylinder minus 1
	DEC	DH
	SUB	CX,CX
	CALL	SetCHS			; Set parameters
	MOV	AL,CMD_INIT		; Issue command
	CALL	SendCMD
	MOV	BL,DISK_TIMEOUT
	CALL	WaitINT			; Wait for interrupt
	ADD	DX,ATA_STATUS		; Quick acknowledge
	MOV	CX,15
	REP	IN AL,DX
	MOV	AL,[ES:DISK_STATUS]	; Check if timeout occured
	AND	AL,AL
	JZ	.initok
	MOV	SI,strTimeout		; Timeout occured, skip drive
	CALL	PrintStr
	JMP	.nothdd
.initok:
;-----	Is drive greater than 504MB?
	MOV	BYTE [BP+.headtmp],0	; Indicates that translation is not used
	MOV	AX,[ES:DI+drvinfo.numcyls]	; Load cylinders for checking 
	MOV	BX,[ES:DI+drvinfo.numheads]	; ... and load heads
	CMP	AX,TRNS_CYL
	JA	.headloop		; Proceed to translation calculation
	CMP	AX,1024			; Cap cylinder count to 1024
	JBE	.notrans
	MOV	AX,1024
	JMP	.notrans
;-----	Increment heads until cylinders is <=1024
.headloop:
	ADD	BX,[ES:DI+drvinfo.numheads]	; Increment heads
	CMP	BX,255			; Has surpassed head limit yet?
	JB	.noheadlim
	MOV	BX,255			; Cap at 255 then bail the loop
.noheadlim:
	MOV	AX,[ES:DI+drvinfo.numcyls]	; Compute total tracks
	MUL	WORD [ES:DI+drvinfo.numheads]
	PUSH	BX
	CALL	DivLong16		; Divide total with translated head
	POP	BX
	CMP	BX,255			; End translation compute if head limit
	JE	.transdone		; ... has been reached
	AND	DX,DX			; Test high word
	JNZ	.headloop
	CMP	AX,1024			; Is it 1024 cylinders or less now?
	JA	.headloop		; Continue loop if not yet
.transdone:
	CMP	AX,1024			; Cap cylinders if >1024 just in case
	JBE	.nocylcap
	MOV	AX,1024
.nocylcap:
	MOV	[BP+.cyltmp],AX		; Store results for safe keeping
	MOV	[BP+.headtmp],BL
;-----	Report translated parameters
	PUSH	SI
	MOV	SI,strTranslate
	CALL	PrintStr
	MOV	SI,strCyl		; Print cylinders
	CALL	PrintStr
	MOV	CX,4
	CALL	PrintNum
	MOV	SI,strHead		; Print heads
	CALL	PrintStr
	MOV	AX,BX
	MOV	CX,3
	CALL	PrintNum
	MOV	SI,strSect		; Print sectors
	CALL	PrintStr
	MOV	AX,[ES:DI+drvinfo.numsects]
	MOV	CX,2
	CALL	PrintNum
	MOV	SI,strDivider
	CALL	PrintStr
	POP	SI
;-----	Report useable size in megabytes
	SUB	BH,BH			; Load head count
	MOV	BL,[BP+.headtmp]
	MUL	BX
	MUL	WORD [BP+.cyltmp]
	CALL	PrintMB
;-----	End of translation routine
	MOV	AX,[BP+.cyltmp]
	MOV	BL,[BP+.headtmp]
	SUB	BH,BH
.notrans:
	; AX - cylinders
	; BX - heads (if >16 translation required)
;-----	Calculate offset for parameter table
	PUSH	AX
	SUB	AH,AH
	MOV	AL,[ES:DISK_NUM]	; Get drive number
	SHL	AX,2			; Multiply by 4
%ifdef SENSIBLE
	ADD	AX,PARM_TBL
%endif
	MOV	SI,AX
	POP	AX
;-----	Populate the parameter entry
	MOV	[ES:SI+diskparm.cyls],AX
	MOV	[ES:SI+diskparm.heads],BL
	MOV	AL,[ES:DI+drvinfo.numsects]
	MOV	[ES:SI+diskparm.sects],AL
;-----	Prepare the drive entry
%ifndef SENSIBLE
	ADD	SI,DMAP_TBL		; Simply increment offset to map table
	SUB	AX,AX			; Clear track offset just in case
	MOV	[ES:SI],AX
	MOV	[ES:SI+2],AX
	MOV	AL,[ES:PORT_OFF]	; Base value
	AND	AL,07H			; Mask-in the first three bits
	MOV	AH,[BP+.headtmp]	; Is .headtmp non-zero (translated)
	AND	AH,AH
	JZ	.notransm
	OR	AL,08H			; Set translated bit
.notransm:
	MOV	[ES:SI+3],AL		; Store map byte
%else
	PUSH	AX
	SUB	AH,AH
	MOV	AL,[ES:DISK_NUM]	; Get drive number
	MOV	SI,AX
	POP	AX
	ADD	SI,DRV_FLAG		; Increment offset to drive flags
	SUB	AL,AL
	MOV	AH,[BP+.headtmp]	; Is .headtmp non-zero (translated)
	AND	AH,AH
	JZ	.notransm
	OR	AL,08H			; Set translated bit
.notransm:
	MOV	[ES:SI],AL		; Store flag byte
%endif
;-----	Tally drives
	INC	BYTE [ES:DISK_NUM]	; Increment for phys. and virt. drives
%ifndef SENSIBLE
	INC	BYTE [ES:PHYS_NUM]	; Increment for physical drives
%endif
;-----	Print newline
	MOV	SI,strCRLF		; Print a newline
	CALL	PrintStr
	ADD	SP,.bplen
	POP	BP
	CLC
	RET
.nothdd:
	ADD	SP,.bplen
	POP	BP
	STC
	RET

;---------------------------------------------------------------:
; =PrintMB							:
;	Takes a long total sector count and prints the value in	:
;	megabytes.						:
;								:
; Arguments:							:
;	DX:AX	Total sector count				:
;								:
; Destroys: AX,BX,CX,DX,SI					:
;---------------------------------------------------------------:
PrintMB:
	MOV	BX,2048			; Divide into megabytes
	CALL	DivLong16
	CALL	PrintLong		; Print the capacity
;-----	Print the decimal point of capacity
	PUSH	AX			; Print decimal point
	PUSH	BX
	MOV	AX,00E00H|'.'
	SUB	BX,BX
	INT	010H
	POP	BX
	POP	AX
	MOV	AX,BX			; Multiply remainder by 9
	MOV	CX,9
	MUL	CX
	MOV	CX,2047			; Divide the result by 2047
	DIV	CX
	SUB	CX,CX			; Print resulting decimal point
	CALL	PrintNum
	MOV	SI,strMB
	CALL	PrintStr
	RET

;---------------------------------------------------------------:
; =CalcCSums							:
;	Calculates a checksum of the work area memory. Used to	:
;	test if the work area has been corrupted by a buggy	:
;	or badly written application, to prevent the potential	:
;	for data corruption.					:
;								:
; Assumes: ES=WRK_SEG						:
;								:
; Returns: AL = Checksum					:
;								:
; Destroys: AH,CX,SI						:
;---------------------------------------------------------------:
CalcCSums:
	CLD
	MOV	CX,WRK_LEN
	MOV	SI,PARM_TBL
	SUB	AH,AH
.cloop:
	MOV	AL,[ES:SI]
	INC	SI
	ADD	AH,AL
	LOOP	.cloop
	MOV	AL,AH
	XOR	AL,0FFH			; Make csum a complement
	RET

;---------------------------------------------------------------:
; Includes							:
;---------------------------------------------------------------:

	%INCLUDE 'bios.inc'		; BIOS interface handlers
	%INCLUDE 'ata.inc'		; ATA disk functions
	%INCLUDE 'util.inc'		; Utility functions

;---------------------------------------------------------------:
; Data area							:
;---------------------------------------------------------------:
	ALIGN 2
;-----	ATA channel base address table, the last half-word specifies IRQ
ATA_BASE	DW 0E1F0H		; Primary channel on IRQ 14
%ifndef SENSIBLE
		DW 0F170H		; Secondary channel on IRQ 15
		DW 0A168H		; Tertiary channel on IRQ 10
		DW 0B1E8H		; Quaternary channel on IRQ 11
%endif

;---------------------------------------------------------------:
; Strings area							:
;---------------------------------------------------------------:

%ifdef BOOTLOADER
strBootFail	DB 0AH,0DH
		DB 'No bootable disk found.',0AH,0DH
		DB 'Insert system disk then bash key when ready...',0AH,0DH,00H
%endif

strPrev		DB 'Previous instance detected - Skipping'
		DB 0AH,0DH,00H

strBIOSpresent	DB 'Fixed disk(s) set by system BIOS '
		DB 'or other adapter - Not installing'
		DB 0AH,0DH,00H

strROMSEG	DB 'ROM Segment:',00H

strScanMsg	DB 'Scanning ATA on port ',00H
strIRQ		DB ' IRQ ',00H
strNoDrives	DB 'No fixed disks found',0AH,0DH,00H
strNoVect	DB 0AH,0DH,'No drive(s) found - '
		DB 'Not installing',00H
strScanDone	DB 'Device scan complete',00H

strTimeout	DB 'Drive timeout - Ignored',0AH,0DH,00H

strM:		DB 'M:',00H
strS:		DB 'S:',00H

strTranslate	DB 0AH,0DH,'  Translated as ',00H

strCyl		DB 'C:',00H
strHead		DB ' H:',00H
strSect		DB ' S:',00H
strMB		DB 'MB',00H
strDivider	DB ' - ',00H

strTainted:	
%ifndef DEBRAND
		DB 'L-'
%endif
		DB 'ATA '
		DB 'INTERNAL TABLES TAINTED',0AH,0DH
		DB 'SYSTEM HALTED',00H

strCRLF		DB 0AH,0DH,00H
