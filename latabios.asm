;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - Main assembler file			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022-24 Meido-Tek Productions					:
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
; The romfixup tool both pads and injects a valid checksum to   :
; the ROM image to- if not performed the system BIOS will       :
; reject it as a faulty ROM due to lacking a valid checksum.	:
;---------------------------------------------------------------:

%define		VERSION '1.01'

;-----	User configurable constants  ---------------------------

; Target for XT systems- 8088 compatible
; instructions are used and I/O adjusted for
; XT-adapted ATA interfaces such as XT-IDE.
;
;%define TARGET_XT


; Enable use of IRQs for XT target- not yet
; tested whatsoever due to emulators not
; implementing IRQ for XT-IDE.
;
; Disk interrupts are expected on IRQ 5- some
; XT-IDE adapters may omit jumper headers or
; entire circuitry for IRQs, however.
;
;%define XT_USE_IRQ


; Uncomment to include a bootloader for older
; systems to boot from hard disk. Enable this if
; hard disks can be seen by DOS from a floppy,
; but the system would not boot from it.
;
; This is always set for XT and 'sensible' builds.
;
;%define BOOTLOADER


; Uncomment to only support a single ATA channel
; with two drives, but may solve compatibility
; issues on some OEM systems.
;
; This is always set for XT builds.
;
;%define SINGLE_DRV


; Uncomment to disable sector translation- cylinder
; count is capped at 1024 for larger drives instead
; of employing translation.
;
; Required for operating systems that access drives
; directly, but are not aware of sector translation-
; would allow a system with fixed parameters to boot
; from a drive at least.
;
;%define NO_TRANSLATION


;-----

;
; XT configs, because nasm has crappy preprocessor
;
%ifdef TARGET_XT
%ifndef XT_USE_IRQ	; Disable use of IRQs for XT
%define DONT_USE_IRQ
%endif
%ifndef SINGLE_DRV	; Only allow single drive
%define SINGLE_DRV
%endif
%ifndef BOOTLOADER	; Always include a bootloader
%define BOOTLOADER
%endif
%endif	; TARGET_XT

;---------------------------------------------------------------:
; Global constants						:
;---------------------------------------------------------------:

; Cylinder count threshold for when sector
; translation should be applied- it may not
; be a good idea to translate a 520MB drive
; that was previously partitioned at 504MB
;
TRNS_CYL	EQU 1152

; Timeout values in seconds
;
DETECT_TIMEOUT	EQU 2			; Drive detection timeout
DISK_TIMEOUT	EQU 10			; Standard timeout (for BIOS access)
POST_DELAY	EQU 2			; POST-screen delay

;---------------------------------------------------------------:
; 8259 constants						:
;---------------------------------------------------------------:

INTA00		EQU 020H		; Master 8259 control
INTA01		EQU 021H		; Master 8259 data
INTB00		EQU 0A0H		; Slave 8259 control
INTB01		EQU 0A1H		; Slave 8259 data

EOI		EQU 020H		; 8259 End-of-Interrupt command

;---------------------------------------------------------------:
; Interrupt vectors						:
;---------------------------------------------------------------:

DISK_VEC	EQU 013H*4		; BIOS disk functions vector
BOOT_VEC	EQU 019H*4		; Bootstrap loader vector
FLOPPY_VEC	EQU 040H*4		; Original BIOS disk functions vector

;---------------------------------------------------------------:
; Include constants						:
;---------------------------------------------------------------:

	%INCLUDE 'disk.inc'		; Include ATA interface definitions

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

%ifndef SINGLE_DRV
MAX_DRV		EQU 8			; Maximum number of physical drives
%elifndef TARGET_XT
MAX_DRV		EQU 2
%else
MAX_DRV		EQU 2
%endif

WRK_SEG		EQU 030H		; Unused portion of BIOS stack area

SECTION		bsa nobits

%ifndef SINGLE_DRV
		ABSOLUTE 00H		; Place at start of BIOS stack area
%else
		ABSOLUTE 01E0H		; Place at very end of BIOS stack area
%endif

WRK_START:

PARM_TBL	RESB diskparm.len*MAX_DRV	; BIOS-reported parameter table
DPRM_TBL	RESB diskparm.len*MAX_DRV	; ATA-reported parameter table
%ifndef SINGLE_DRV
DMAP_TBL	RESD MAX_DRV		; Drive mapping and relocation table
					;   bits 0-23	: Starting track
					;   bits 24-26	: Physical drive num.
					;   bit  27	: 1=Translated+trackoff
					;   bit  28	: 1=Use LBA addressing
					;   bits 29-31	: Reserved
PHYS_NUM	RESB 1			; Physical drive count (incl. checksum)
%else
DRV_FLAG	RESB MAX_DRV		; Physical Drive flags
					;   bits 0-2	: Unused
					;   bit  3	: 1=Translated+trackoff
					;   bit  4	: 1=Use LBA addressing
					;   bits 5-7	: Unused
%endif

WRK_END:				; Work-area end label to determine size

ATA_STATUS	RESB 1			; ATA internal status byte
					;   bits 0-3	: IRQ pending bits
					;   bit  4	: Corrected read flag
					;   bits 5-7	: Unused

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
; INT 13h status code definitions				:
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

%ifndef TARGET_XT
	CPU	286			; 80286 instructions
%else
	CPU	8086			; 8088/86 instructions for XTs
%endif

	ORG	0H			; Set starting offset to zero
		
	DB	055H			; Magic bytes of option ROM header
	DB	0AAH
	DB	8			; Specify 4K ROM (4096/512 = 8)

	JMP	ROM_SETUP		; Skips non-executable text below

BANNER:
	DB	'L-ATA Fixed Disk Support BIOS'
	DB	'  by  John "Lameguy" Wilbert Villamor',0AH,0DH
	DB	'2022-24 Meido-Tek Productions ('
%ifndef VERSION
	DB	__?DATE?__,' ',__?TIME?__
%else
	DB	VERSION
%endif

%ifndef TARGET_XT
%ifdef SINGLE_DRV
	DB	'-S'			; Single, or 'sensisble' suffix
%endif
%else
	DB	'-XTI'			; XT-IDE compatible suffix
%endif
	DB	')',0AH,0DH
	DB	'Released under MPLv2'
%ifdef BOOTLOADER
	DB	0AH,0DH
	DB	'Bootloader Enabled'
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
	SUB	BX,BX			; Print opening parenthesis of ROM
	MOV	SI,strROMSEG		; ... segment message
	CALL	PrintStr
	MOV	AX,CS			; Print the ROM's code segment
	MOV	CL,4
	CALL	PrintHex
	MOV	SI,strCRLF		; Produce whitespace between banner and
	CALL	PrintStr		; drive messages that will follow
	CALL	PrintStr
%ifdef TARGET_XT
;-----	Set up hardware timer (not running by default on XTs)
	CLI				
	MOV	AL,036H
	OUT	043H,AL			; Set PIT timer interval to zero,
	SUB	AL,AL			; should yield the standard 18.2Hz rate
	OUT	040H,AL
	OUT	040H,AL
	IN	AL,INTA01		; Unmask IRQ0
	AND	AL,0FEH
	OUT	INTA01,AL
	STI
%endif	; TARGET_XT
;-----  Initial preparations
	MOV	BX,WRK_SEG		; Set ES to point at work segment
	MOV	ES,BX
	CALL	CalcCSums		; Check if a previous instance had
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
	MOV	SI,strBIOSpresent	; Non-zero- another BIOS has
	CALL	PrintStr		; ... defined drives!
	JMP	.done			; Exit right away
.nodiskbios:
;-----	Initialize some variables
	SUB	AL,AL
	MOV	ES:DISK_STATUS,AL	; Disk status
%ifndef SINGLE_DRV
	MOV	ES:PHYS_NUM,AL		; Physical drive count
%endif
;-----	Scan, initialize and prepare drives
	CALL	ScanDrives
	MOV	SI,strScanDone
	CALL	PrintStr
;-----	Hook disk BIOS to the INT 13H vector
	MOV	AL,ES:DISK_NUM		; Check number of drives tallied
	AND	AL,AL			; Skip vector hook if no drives found
	JZ	.nodrives		
	PUSH	ES
	SUB	BX,BX
	MOV	ES,BX
	MOV	AX,ES:DISK_VEC		; Copy original disk vector
	MOV	ES:FLOPPY_VEC,AX	; ... to INT 40h (for floppies)
	MOV	AX,ES:DISK_VEC+2
	MOV	ES:FLOPPY_VEC+2,AX
	MOV	AX,DISK_IO		; Set own disk function
	MOV	ES:DISK_VEC,AX
	MOV	AX,CS
	MOV	ES:DISK_VEC+2,AX
;----	Hook bootstrap loader if enabled
%ifdef BOOTLOADER
	MOV	AX,BOOTSTRAP
	MOV	ES:BOOT_VEC,AX
	MOV	AX,CS
	MOV	ES:BOOT_VEC+2,AX
%endif
	POP	ES
;-----	Calculate and store checksum of parameter tables
	CALL	CalcCSums
	MOV	ES:CKSUM,AL		; Store it at the workspace
	JMP	.done
.nodrives:
	MOV	SI,strNoVect		; Skipped hooking vector when no drives
	CALL	PrintStr		; ... are detected
.done:
	MOV	SI,strCRLF		; Whitespace
	CALL	PrintStr
	SUB	DX,DX			; Give time for user to see the
	MOV	CX,ES:TIMER_LOW		; ... initialization results
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
%ifndef DONT_USE_IRQ
	MOV	SI,strIRQ		; Display IRQ
	CALL	PrintStr
	MOV	AL,DH			; Obtain the IRQ value
%ifndef TARGET_XT
	SHR	AX,4
%else
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
%endif	; TARGET_XT
	AND	AL,0FH
	SUB	CX,CX
	CALL	PrintNum		; Print IRQ
%endif	; DONT_USE_IRQ
	MOV	SI,strCRLF		; New line
	CALL	PrintStr
	MOV	[BP+.baseIO],DX		; Store I/O port
	AND	DH,0FH			; Clear IRQ number
	CALL	ResetATA		; Reset the ATA bus
	SUB	DX,DX			; Wait two seconds after reset
	MOV	CX,ES:TIMER_LOW		; Give a second of delay after
	ADD	CX,18			; reset- required for CD-ROMs
	ADC	DX,0
.rstwait:
	CMP	ES:TIMER_LOW,CX
	JB	.rstwait
	CMP	ES:TIMER_LOW+2,DX
	JB	.rstwait
%ifndef DONT_USE_IRQ
;-----	Hook interrupt handler	
	CLI
	MOV	DX,[BP+.baseIO]		; Load I/O port
	SUB	AH,AH			; Clear upper byte
	MOV	AL,DH			; Obtain IRQ number
%ifndef TARGET_XT
	SHR	AX,4			; Adjust for the IRQ number
	SUB	AX,08H			; Adjust value to base 0
	ADD	AX,070H			; Increment to slave 8259 vectors at 70-77h (INT 8-15)
	SHL	AX,2			; Multiply value by 4
%else
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	AX,08H			; Increment IRQ to master 8259 vectors 8-15h (INT 0-7)
	SHL	AX,1			; Multiply value by 4
	SHL	AX,1
%endif	; TARGET_XT
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
;-----	Enable mask bit on slave 8259 (master for XT)
	MOV	DX,[BP+.baseIO]		; Load base address
	MOV	AL,DH			; Get IRQ number from address
%ifndef TARGET_XT
	SHR	AL,4
	SUB	AL,08H			; Subtract to base 0 from 8 to 15
%else
	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
%endif	; TARGET_XT
	MOV	CL,AL			; Copy value
	MOV	AH,1			; Load bit
	SHL	AH,CL			; Shift bit left by adjusted IRQ number
	XOR	AH,0FFH			; Invert bits
%ifndef TARGET_XT
	IN	AL,INTB01		; Read slave 8259 mask port
%else
	IN	AL,INTA01		; Read master 8259 mask port
%endif	; TARGET_XT
	MOV	[BP+.oldmask],AL	; Save old mask
	AND	AL,AH			; Clear bit with mask pattern
%ifndef TARGET_XT
	OUT	INTB01,AL		; Set new mask to slave 8259
%else
	OUT	INTA01,AL		; Set new mask to master 8259
%endif	; TARGET_XT
	STI
%endif	; DONT_USE_IRQ
;-----	Scan channel for master/slave drives
	SUB	CL,CL			; Zero-out drive found count
	MOV	[BP+.drvfound],CL
.probeloop:
;-----
	MOV	AL,1			; Construct dummy parameters-
	MOV	AH,AL			; ... only need to select drive
	SUB	DH,DH
	SUB	CX,CX
	CALL	SetCHS			; Apply parameters
	MOV	AL,CMD_IDENTIFY		; Issue IDENTIFY command
	CALL	SendCMD
	SUB	BH,BH			; Wait for disk response
	MOV	BL,DETECT_TIMEOUT
	CALL	WaitATA
	MOV	AL,[ES:DISK_STATUS]	; Check if timeout occured
	AND	AL,AL
	JNZ	.nodrive
	ADD	DX,ATA_STAT		; Read status and acknowledge ATA IRQ
	IN	AL,DX
	AND	AL,STAT_ERR		; Was there errror on IDENTIFY? Normally
	JNZ	.nodrive		; set for CD-ROMs as they speak ATAPI
	SUB	DX,ATA_STAT
;-----	Fetch and parse information from hard drive
	MOV	DI,SECT_BUFF		; Set DI to sector buffer
	MOV	CX,0100H		; 256 words, 512 bytes
	CLD
%ifndef TARGET_XT
	REP	INSW			; Fetch the block (AT)
%else
.blkloop:				; Read from 16-bit expander (XT)
	IN	AL,DX			; Read low-byte
	MOV	AH,AL
	ADD	DX,08H			; Adjust to hi-byte port and read it
	IN	AL,DX
	XCHG	AH,AL
	STOSW
	ADD	DX,-08H			; Revert to lo-byte for next iteration
	DEC	CX
	JNZ	.blkloop
%endif
	MOV	DI,SECT_BUFF
	CALL	ParseDrive		; Parse the data returned
	JC	.nodrive
	INC	BYTE [BP+.drvfound]	; Increment the drives found
.nodrive:
	TEST	BYTE [ES:PORT_OFF],01H	; Check if slave drive is being probed
	JNZ	.probedone		; If so, channel is done for probing
	INC	BYTE [ES:PORT_OFF]	; Otherwise advance to slave drive
	JMP	.probeloop
.probedone:
	MOV	AL,[BP+.drvfound]	; Any valid drives found from probing?
	AND	AL,AL
	JNZ	.skipchan		; Proceed to next ATA channel
%ifndef DONT_USE_IRQ
;-----	Restore old vector and IRQ mask if no drive found
	MOV	AL,[BP+.oldmask]	; Set old IRQ mask
%ifndef TARGET_XT
	OUT	INTB01,AL
%else
	OUT	INTA01,AL
%endif	; TARGET_XT
	MOV	AX,[BP+.baseIO]		; Get base I/O address
	MOV	AL,AH			; Obtain IRQ number
%ifndef TARGET_XT
	SHR	AL,4
%else
	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
	SHR	AL,1
%endif	; TARGET_XT
	SUB	AL,8			; Adjust for base zero
	SUB	AH,AH
%ifndef TARGET_XT
	ADD	AX,070H			; Increment to slave 8259 vectors 70-77
	SHL	AX,2			; Multiply by 4
%else
	ADD	AX,008H			; Increment to master 8259 vectors 8-15
	SHL	AX,1			; Multiply by 4
	SHL	AX,1
%endif	; TARGET_XT
	MOV	DI,AX			; Set as DI pointer
	MOV	BX,ES			; Save old ES
	SUB	AX,AX			; Set ES to segment 0
	MOV	ES,AX
	MOV	AX,[BP+.oldhandler]	; Get old vector
	MOV	DX,[BP+.oldhandler+2]
	MOV	[ES:DI],AX		; Apply to vector table
	MOV	[ES:DI+2],DX
	MOV	ES,BX			; Restore old ES
%endif	; DONT_USE_IRQ
.skipchan:
	MOV	AL,[ES:PORT_OFF]
%ifndef SINGLE_DRV
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
;-----  Identifier must not be empty
	MOV	AX,[ES:DI+drvinfo.model]
	AND	AX,AX
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
	MOV	BYTE [DS:SI+39],000H	; Set zero terminator
	CALL	PrintStr		; Print model string
	MOV	DS,BX			; Restore DS
;-----	Copy parameters to physical drive table
	SUB	AH,AH
	MOV	AL,ES:PORT_OFF		; Get drive number
	AND	AL,07H
%ifndef TARGET_XT
	SHL	AX,2
%else
	SHL	AX,1
	SHL	AX,1
%endif
	MOV	SI,DPRM_TBL		; Load pointer to original params table
	ADD	SI,AX			; Apply offset from number
	MOV	AX,[ES:DI+drvinfo.numcyls]	; Fill the parameter entry
	MOV	[ES:SI+diskparm.cyls],AX
	MOV	AX,[ES:DI+drvinfo.numheads]
	MOV	[ES:SI+diskparm.heads],AL
	MOV	AX,[ES:DI+drvinfo.numsects]
	MOV	[ES:SI+diskparm.sects],AL
;-----	Print capacity from LBA
	PUSH	DX
	TEST	WORD [ES:DI+drvinfo.diskcaps],0200H
	JZ	.nolba
	MOV	AX,[ES:DI+drvinfo.nlbatotal]	; Load total sector count
	MOV	DX,[ES:DI+drvinfo.nlbatotal+2]
	JMP	.reportdone
.nolba:
;-----	Otherwise print capacity from CHS parameters
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
	CALL	WaitATA			; Wait for disk
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
%ifndef NO_TRANSLATION
	CMP	AX,TRNS_CYL
	JA	.headloop		; Proceed to translation calculation
%endif	; NO_TRANSLATION
	CMP	AX,1024			; Cap cylinder count to 1024
	JBE	.notrans
	MOV	AX,1024
%ifndef NO_TRANSLATION
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
%endif	; NO_TRANSLATION
.notrans:
	; AX - cylinders
	; BX - heads (if >16 translation required)
;-----	Calculate offset for parameter table
	PUSH	AX
	SUB	AH,AH
	MOV	AL,[ES:DISK_NUM]	; Get drive number
%ifndef TARGET_XT
	SHL	AX,2			; Multiply by 4
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
%ifdef SINGLE_DRV
	ADD	AX,PARM_TBL
%endif	; SINGLE_DRV
	MOV	SI,AX
	POP	AX
;-----	Populate the parameter entry
	MOV	[ES:SI+diskparm.cyls],AX
	MOV	[ES:SI+diskparm.heads],BL
	MOV	AL,[ES:DI+drvinfo.numsects]
	MOV	[ES:SI+diskparm.sects],AL
;-----	Prepare the drive entry
%ifndef SINGLE_DRV
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
%endif	; SINGLE_DRV
;-----	Tally drives
	INC	BYTE [ES:DISK_NUM]	; Increment for phys. and virt. drives
%ifndef SINGLE_DRV
	INC	BYTE [ES:PHYS_NUM]	; Increment for physical drives
%endif	; SINGLE_DRV
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
;	test if the work area has been corrupted by software	:
;	error to protect against data corruption.		:
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
%ifndef TARGET_XT
ATA_BASE	DW 0E1F0H		; Primary channel on IRQ 14
%ifndef SINGLE_DRV
		DW 0F170H		; Secondary channel on IRQ 15
		DW 0A168H		; Tertiary channel on IRQ 10
		DW 0B1E8H		; Quaternary channel on IRQ 11
%endif	; SINGLE_DRV
%else
ATA_BASE	DW 05300H		; XT-IDE at port 300h IRQ 5
%endif	; TARGET_XT


;---------------------------------------------------------------:
; Strings area							:
;---------------------------------------------------------------:

%ifdef BOOTLOADER
strBootFail	DB 0AH,0DH
		DB 'No bootable disk found.',0AH,0DH
		DB 'Insert system disk then bash key when ready...'
		DB 0AH,0DH,00H
%endif

strPrev		DB 'Previous instance detected - Skipping'
		DB 0AH,0DH,00H

strBIOSpresent	DB 'Fixed disk(s) set by system BIOS '
		DB 'or other adapter - Not installing'
		DB 0AH,0DH,00H

strROMSEG	DB 'ROM Segment:',00H

%ifndef TARGET_XT
strScanMsg	DB 'Scanning ATA on port ',00H
%else
strScanMsg	DB 'Scanning XT-IDE on port ',00H
%endif	; TARGET_XT
strIRQ		DB ' IRQ ',00H
strNoVect	DB 0AH,0DH,'No drive(s) found - '
		DB 'Not installing',00H
strScanDone	DB 'Device scan complete',0AH,0DH,00H

strTimeout	DB 'Timeout on init - Drive ignored',0AH,0DH,00H

strM:		DB 'M:',00H
strS:		DB 'S:',00H

strTranslate	DB 0AH,0DH,'  Translated as ',00H

strCyl		DB 'C:',00H
strHead		DB ' H:',00H
strSect		DB ' S:',00H
strMB		DB 'MB',00H
strDivider	DB ' - ',00H

strTainted:	
		DB 'L-ATA TABLES TAINTED',0AH,0DH
		DB 'SYSTEM HALTED',00H

strCRLF		DB 0AH,0DH,00H
