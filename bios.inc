;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - BIOS functions			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022 Meido-Tek Productions					:
;								:
; Main file: latabios.asm					:
;---------------------------------------------------------------:

;---------------------------------------------------------------:
; INT 19h bootstrap loader (required for very early AT clones)	:
;---------------------------------------------------------------:
%ifdef BOOTLOADER

BOOT_LOCN	EQU 07C00H		; Location for IPL

BOOTSTRAP:
;-----	Initial preparations
	STI				; Enable interrupts
	SUB	DX,DX			; Establish segments at 0000
	MOV	DS,DX
	MOV	ES,DX
	MOV	BX,BOOT_LOCN		; Load address of boot location
.retry:					; Retry loop
	SUB	DX,DX			; Set zero for floppy drive
;-----	Clear boot location area
	CLD
	SUB	AX,AX
	MOV	CX,256
	MOV	DI,BX
	REP	STOSW
;-----	Try to boot floppy disk
	MOV	CX,4			; Retry count
.fdloop:				; Floppy retry loop
	PUSH	CX			; Save retry count
	SUB	AX,AX			; Reset drive
	INT	013H			; Disk function
	JC	.fdretry		; Was carry set by call?
	MOV	AX,0201H		; Read sector(s)
	MOV	CX,1			; Set first sector
	INT	013H			; Disk function
.fdretry:
	POP	CX			; Restore retry count
	JNC	.fdipl			; If read was OK, jump to boot routine
	CMP	AH,080H			; Check if timeout (ie. empty drive)
	JE	.hddboot		; Try hard disk if that's the case
	LOOP	.fdloop			; Retry floppy until all tries spent
	JMP	.hddboot		; Boot hard disk
.fdipl:
	CMP	BYTE DS:BOOT_LOCN,06H	; Check if first byte is a valid opcode
	JB	.hddboot		; If not then disk is unlikely bootable
	MOV	DI,BOOT_LOCN		; Load IPL location address
	MOV	CX,8			; Test 8 words
	MOV	AX,[DS:DI]		; Load first word
.checkloop:
	ADD	DI,2			; Advance address
	CMP	AX,[DS:DI]		; Compare against first word
	LOOPZ	.checkloop		; Continue iterating if non-matching
	JZ	.hddboot		; Boot hard disk if check failed
.jumpstart:
	JMP	0:BOOT_LOCN		; Absolute jump to IPL location
;-----	Hard disk boot
.hddboot:
	SUB	AX,AX			; Reset floppy just in case
	INT	013H			; Disk function
	MOV	CX,4			; Set retry count for hard disk
	MOV	DX,0080H		; Fixed disk 0
.hddloop:
	PUSH	CX			; Save retry counter
	SUB	AX,AX			; Reset hard drive
	INT	013H			; Disk function
	JC	.hddretry		; Retry on bad reset
	MOV	AX,0201H		; Read sector(s)
	MOV	CX,0001H		; Set first sector
	INT	013H			; Disk function
.hddretry:
	POP	CX			; Restore retry counter
	JC	.hddtry			; Try again on read fault
	CMP	WORD [DS:BOOT_LOCN+510],0AA55H	; Check generic boot block mark
	JE	.jumpstart		; Jump to boot location
.hddtry:
	LOOP	.hddloop		; Try hard disk again
;-----	Boot failure routine
.bootfail:
	PUSH	DS			; Prompt that no bootable disks
	MOV	AX,CS			; were found
	MOV	DS,AX
	MOV	SI,strBootFail		; Print boot failure message
	CALL	PrintStr
	POP	DS
	SUB	AH,AH			; Wait for key press
	INT	016H
	JMP	.retry

%endif

;---------------------------------------------------------------:
; =DISK_IO							:
;	INT 13h disk functions.					:
;								:
;	The entrypoint for INT 13h calls. Drive numbers that	:
;	are not fixed disk (DL < 80h) are passed to the floppy	:
;	disk vector relocated at interrupt vector 40h.		:
;---------------------------------------------------------------:
DISK_IO:
	CMP	DL,080H			; Is it a fixed disk number?
	JAE	FIXED_DISK		; Jump to fixed disk routine
	INT	040H			; Otherwise invoke floppy vector
DISK_RET:
	RETF	2			; Far return, discarding flags

;---------------------------------------------------------------:
; Disk BIOS work variables (global to all INT 13h functions)	:
;---------------------------------------------------------------:

wrk.ioSEG	EQU -2			; Data read/write far pointer
wrk.sect	EQU -3			; Sector number
wrk.nsects	EQU -4			; Number of sectors requested
wrk.cyl		EQU -6
wrk.head	EQU -7
wrk.ndone	EQU -8
wrk.len		EQU 8

;---------------------------------------------------------------:
; =FIXED_DISK							:
;	Setup function for fixed disk routines.			:
;---------------------------------------------------------------:
FIXED_DISK:
	STI				; Enable interrupts
	AND	AH,AH
	JNZ	.notreset
;	INT	040H
	SUB	AH,AH
	CMP	DL,(080H+MAX_DRV-1)
	JA	DISK_RET
.notreset:
	CMP	AH,08H			; Check if get drive parameters
	JE	GET_PARAMS		; Report drive parameters
	CMP	AH,014H			; Reject disk commands beyond 014H
	JG	FIXED_RET
	PUSH	BP			; Setup BP
	MOV	BP,SP
	SUB	SP,wrk.len
	PUSH	BX			; Save all registers bar AX
	PUSH	CX
	PUSH	DX
	PUSH	DS
	PUSH	SI
	PUSH	DI
;-----	Pre-setup
	MOV	[BP+wrk.ioSEG],ES	; Save far data pointer of disk call
	MOV	DI,BX			; DI is I/O offset
	MOV	[BP+wrk.nsects],AL	; Sector count
	MOV	SI,WRK_SEG		; Setup BDA segment
	MOV	ES,SI
;-----	Verify parameter tables are not tainted
	PUSH	AX
	PUSH	CX
	CALL	CalcCSums
	CMP	AL,ES:CKSUM
	JNE	TABLE_TAINTED
	POP	CX
	POP	AX
	CALL	DO_DISKOP		; Perform the actual disk operation
	MOV	ES,[BP+wrk.ioSEG]	; Restore ES from ioSEG
	POP	DI
	POP	SI
	POP	DS
	POP	DX
	POP	CX
	POP	BX
	ADD	SP,wrk.len
	POP	BP
FIXED_RET:
	CMP	AH,1			; Set carry if AH is non-zero
	CMC
	RETF	2			; Far return, discarding flags

;---------------------------------------------------------------:
; =GET_PARAMS							:
;	Returns disk parameters for drive number in DL to the	:
;	caller.							:
;---------------------------------------------------------------:
GET_PARAMS:
	PUSH	SI
	PUSH	ES
	PUSH	DI
	MOV	SI,WRK_SEG
	MOV	ES,SI
	CMP	DL,ES:DISK_NUM		; Check if disk number is valid
	JL	.numgood
	MOV	AH,INIT_FAIL
	JMP	.done
.numgood:
	PUSH	AX
	SUB	AH,AH			; Get (normalized) drive number
	MOV	AL,DL			; Get (normalized) drive number
%ifndef TARGET_XT
	SHL	AL,2			; Multiply by 4
%else
	SHL	AL,1
	SHL	AL,1
%endif	; TARGET_XT
%ifdef SINGLE_DRV
	ADD	AX,PARM_TBL
%endif	; SINGLE_DRV
	MOV	SI,AX			; Move to SI pointer
	MOV	CL,[ES:SI+diskparm.sects]
	MOV	DH,[ES:SI+diskparm.heads]
	MOV	AX,[ES:SI+diskparm.cyls]
	SUB	AX,2			; Deduce two cylinders
	DEC	DH			; Deduce one head
	MOV	CH,AL
%ifndef TARGET_XT
	SHL	AH,6
%else
	PUSH	CX
	MOV	CL,6
	SHL	AH,CL
	POP	CX
%endif	; TARGET_XT
	OR	CL,AH
	MOV	DL,ES:DISK_NUM
	SUB	BL,BL
	POP	AX
	SUB	AH,AH
	MOV	ES:DISK_STATUS,AH
.done:
	POP	DI
	POP	ES
	POP	SI
	JMP	FIXED_RET

;---------------------------------------------------------------:
; =TABLE_TAINTED						:
;	Tainted table handler (forces an error screen)		:
;---------------------------------------------------------------:
TABLE_TAINTED:
	SUB	AH,AH			; Change to text mode
	MOV	AL,00
	INT	010H
	CLI				; Disable IRQs
	MOV	SI,CS			; Print tainted error message
	MOV	DS,SI
	MOV	SI,strTainted
	CALL	PrintStr
.stop:	JMP	.stop

;---------------------------------------------------------------:
; Function table for DO_DISKOP					:
;---------------------------------------------------------------:
	ALIGN 2
FTBL:
	DW	DISK_READ		; 02 Read sector(s)
	DW	DISK_WRITE		; 03 Write sector(s)
	DW	DISK_VERIFY		; 04 Verify sector(s)
	DW	DISK_FORMAT		; 05 Format track
	DW	BAD_COMMAND		; 06 Not implemented on AT
	DW	BAD_COMMAND		; 07 Not implemented on AT
	DW	0			; 08 Get drive parameters (padding here)
	DW	DUMMY			; 09 Initialize drive parameters
	DW	BAD_COMMAND		; 0A Read long, not on AT
	DW	BAD_COMMAND		; 0B Write long, not on AT
	DW	DISK_SEEK		; 0C Seek to cylinder
	DW	DUMMY			; 0D Reset hard disks
	DW	DUMMY			; 0E Read sector buffer, not on AT
	DW	DUMMY			; 0F Write sector buffer, not on AT
	DW	DUMMY			; 10 Test drive ready
	DW	DISK_RECAL		; 11 Recalibrate drive
	DW	BAD_COMMAND		; 12 Ctlr. RAM diagnostic, not on AT
	DW	BAD_COMMAND		; 13 Drive diagnostic, not on AT
	DW	BAD_COMMAND		; 14 Controller diagnostic, not on AT

;---------------------------------------------------------------:
; Performs the actual BIOS disk functions			:
;---------------------------------------------------------------:
DO_DISKOP:
	CMP	AH,00H			; Disk reset
	JE	DUMMY
	CMP	AH,01H			; Return disk system status if AH=01H
	JNE	.nostatus
	MOV	AL,ES:DISK_STATUS	; Get the last status
	MOV	BYTE ES:DISK_STATUS,0	; Clear status
	RET				; Return
.nostatus:
	SUB	DL,080H			; Convert number to based zero
	CMP	DL,ES:DISK_NUM		; Drive number legal?
	JAE	BAD_COMMAND
	MOV	BYTE ES:DISK_STATUS,0	; Clear disk status
	PUSH	AX
;-----	Set PORT_OFF from drive map table
	SUB	AH,AH			; Copy drive number
	MOV	AL,DL
%ifndef SINGLE_DRV
%ifndef TARGET_XT
	SHL	AX,2			; Multiply by 4
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
	MOV	SI,DMAP_TBL		; Load pointer to map table
	ADD	SI,AX			; Apply offset
	MOV	AL,[ES:SI+3]		; Load upper byte
	MOV	[ES:PORT_OFF],AL	; Set as selected drive
%else
	MOV	SI,DRV_FLAG		; Load pointer to flag table
	ADD	SI,AX			; Apply offset
	MOV	AL,[ES:SI]		; Load upper byte
	AND	DL,07H
	OR	AL,DL
	MOV	[ES:PORT_OFF],AL
%endif	; SINGLE_DRV
;-----	Jump to specified function number
	POP	AX
	PUSH	CX
	MOV	CL,AH			; Copy function number
	SUB	CL,2
	SUB	CH,CH
	SHL	CX,1			; Multiply number by two
	MOV	SI,CX			; Copy result to SI
	POP	CX
	JMP	WORD [CS:SI+FTBL]	; Jump to function

;---------------------------------------------------------------:
; Special return functions					:
;---------------------------------------------------------------:
BAD_COMMAND:				; Return with bad command error
	MOV	AH,BAD_CMD
	MOV	ES:DISK_STATUS,AH
	RET				; Exit

DUMMY:					; Dummy return with good response
	SUB	AL,AL
	RET

;---------------------------------------------------------------:
; Read sector(s)					AH=02H  :
;---------------------------------------------------------------:
DISK_READ:
	CALL	PrepParams		; Prepare disk parameters
	JNC	.paramgood
	MOV	AH,BAD_SEEK
	SUB	AL,AL
	JMP	.done
.paramgood:
	MOV	BYTE [BP+wrk.ndone],00H	; Zero out sectors done count
;-----	Calculate pointer for the internal parameter table
	SUB	AH,AH
	MOV	AL,[ES:PORT_OFF]
	AND	AL,07H
%ifndef TARGET_XT
	SHL	AX,2			; Multiply number by 4
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
	MOV	SI,DPRM_TBL		; Base offset of internal param. table
	ADD	SI,AX
.trkloop:
;-----	Cap requested sectors to track bounds (new)
	MOV	AL,[BP+wrk.nsects]	; Load sector count requested
	SUB	AL,[BP+wrk.ndone]	; Subtract by total tallied
	MOV	AH,[ES:SI+diskparm.sects]
	CMP	AL,AH			; Check if cap is required
	JB	.nocap
	MOV	AL,AH			; Apply cap
.nocap:
	MOV	AH,[BP+wrk.sect]	; Load sector number
	DEC	AH			; Decrement to range 0-62
	ADD	AH,AL			; Add count to number
	SUB	AH,[ES:SI+diskparm.sects]
	JBE	.nocapb
	SUB	AL,AH
.nocapb:
;-----	Wait ready
;	SUB	BH,BH			; Load standard timeout value
;	MOV	BL,DISK_TIMEOUT
;	CALL	WaitRDY
;	PUSH	AX
;	MOV	AL,[ES:DISK_STATUS]	; Has wait invoked a timeout?
;	AND	AL,AL
;	POP	AX
;	JNZ	ERR_TIMEOUT		; Jump to timeout error
;	MOV	BYTE [ES:DISK_STATUS],0	; Clear disk status
;-----	Apply disk parameters for the current block
	MOV	AH,[BP+wrk.sect]
	MOV	CX,[BP+wrk.cyl]
	MOV	DH,[BP+wrk.head]
	CALL	SetCHS
;-----	Perform read request of current block
	MOV	BH,AL			; Copy target counter
	SUB	BL,BL			; Sector counter
	MOV	AL,CMD_READ		; Issue READ command
	CALL	SendCMD
.rdloop:				; Sector read loop
	PUSH	BX
	MOV	BL,DISK_TIMEOUT		; Load standard timeout value
	CALL	WaitRDY			; Wait for drive to be ready
	POP	BX
;-----	Check timeout condition
	MOV	AL,[BP+wrk.ndone]	; Load current sectors read total
	ADD	AL,BL
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
;-----	Set wait bit for next IRQ
	INC	BL			; Increment sectors read
	CMP	BL,BH
	JGE	.noint			; Skip if equal to requested count
	MOV	AL,[ES:PORT_OFF]	; Get channel number
	AND	AL,06H
	SHR	AL,1			; Shift out master/slave bit
	MOV	CL,AL			; Create bit mask
	MOV	AL,1
	SHL	AL,CL
	OR	[ES:ATA_STATUS],AL	; Apply to status byte
.noint:
;-----	Fetch data (required by ATA spec)
	PUSH	ES
	MOV	ES,[BP+wrk.ioSEG]	; Load segment for block transfer
	CALL	ReadBLK
	POP	ES
;-----	Check if operation posted an error
	MOV	AH,[BP+wrk.ndone]
	ADD	AH,BL
	DEC	AH
	ADD	DX,ATA_STAT
	IN	AL,DX
	TEST	AL,STAT_ERR|STAT_DWF	; Test error bits from DRQ wait
	XCHG	AL,AH
	JNZ	FDISK_ERROR
	TEST	AH,STAT_CORR		; Check for correctable read error
	JZ	.nocorr
	OR	BYTE ES:ATA_STATUS,010H	; Set correctable error status
.nocorr:
	CMP	BL,BH			; Compare sector count
	JB	.rdloop			; Exit loop when sectors are read
.rddone:
;-----	Update counters for next read iteration
	ADD	[BP+wrk.ndone],BL	; Add sector count
	MOV	BYTE [BP+wrk.sect],01H	; Reset sector count to 1
	MOV	AL,[BP+wrk.head]	; Increment head
	INC	AL
	CMP	AL,[ES:SI+diskparm.heads]	; Has head surpassed geometry?
	JB	.noheadc		; Advance cylinder
	INC	WORD [BP+wrk.cyl]	
	SUB	AL,AL			; Reset head counter to zero
.noheadc:
	MOV	[BP+wrk.head],AL
	MOV	AL,[BP+wrk.ndone]	; Check if any sectors remain
	CMP	AL,[BP+wrk.nsects]
	JB	.trkloop		; Repeat if more sectors to read
;-----	Check if correctable read occurred
	MOV	AH,ES:ATA_STATUS	; Test status bit
	TEST	AH,010H
	JZ	.retok
	AND	AH,~010H		; Mask off status bit
	MOV	BYTE ES:ATA_STATUS,AH
	MOV	AH,DATA_CORRECTED	; Return data corrected status
	JMP	.done
.retok:
	SUB	AH,AH			; Return good status
.done:
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET

;---------------------------------------------------------------:
; Write sector(s) 					AH=03H  :
;---------------------------------------------------------------:
DISK_WRITE:
	CALL	PrepParams		; Fix-up drive parameters
	JNC	.paramgood
	MOV	AH,BAD_SEEK
	SUB	AL,AL
	JMP	.done
.paramgood:
	MOV	BYTE [BP+wrk.ndone],00H	; Zero out sectors done count
	MOV	SI,DI			; Move I/O offset to SI
	MOV	DS,[BP+wrk.ioSEG]	; Load I/O segment to DS
;-----	Calculate pointer for the internal parameter table
	SUB	AH,AH
	MOV	AL,[ES:PORT_OFF]
	AND	AL,07H
%ifndef TARGET_XT
	SHL	AX,2			; Multiply number by 4
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
	MOV	DI,DPRM_TBL		; Base offset of internal param. table
	ADD	DI,AX
	CLD				; Clear direction flag
.trkloop:				; Track loop for multi-track writes
;-----	Cap requested sectors to track bounds (new)
	MOV	AL,[BP+wrk.nsects]	; Load sector count requested
	SUB	AL,[BP+wrk.ndone]	; Subtract by total tallied
	MOV	AH,[ES:DI+diskparm.sects]
	CMP	AL,AH			; Check if cap is required
	JB	.nocap
	MOV	AL,AH			; Apply cap
.nocap:
	MOV	AH,[BP+wrk.sect]	; Load sector number
	DEC	AH			; Decrement to range 0-62
	ADD	AH,AL			; Add count to number
	SUB	AH,[ES:DI+diskparm.sects]
	JBE	.nocapb
	SUB	AL,AH
.nocapb:
;-----	Wait ready
;	SUB	BH,BH			; Load standard timeout value
;	MOV	BL,DISK_TIMEOUT
;	CALL	WaitRDY
;	PUSH	AX
;	MOV	AL,[ES:DISK_STATUS]	; Has wait invoked a timeout?
;	AND	AL,AL
;	POP	AX
;	JNZ	ERR_TIMEOUT		; Jump to timeout error
;	MOV	BYTE [ES:DISK_STATUS],0	; Clear disk status
;-----	Apply disk parameters for the current block
	MOV	AH,[BP+wrk.sect]
	MOV	CX,[BP+wrk.cyl]
	MOV	DH,[BP+wrk.head]
	CALL	SetCHS			; Set disk parameters
;-----	Perform write request of current block
	MOV	BH,AL
	SUB	BL,BL
	MOV	AL,CMD_WRITE		; Send write command
	CALL	SendCMD
.wrloop:				; Sector write loop
	CALL	WriteBLK
;-----	Wait for drive ready
	PUSH	BX
	MOV	BL,DISK_TIMEOUT
	CALL	WaitRDY
	POP	BX
;-----	Check if operation was an error
	MOV	AL,[BP+wrk.nsects]
	ADD	AL,BL
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
	ADD	DX,ATA_STAT		; Adjust to status register
	MOV	AH,AL			; Transfer sector total
	MOV	CX,15			; To make sure drive is acknowledged
	REP	IN AL,DX		; Read status register
	SUB	DX,ATA_STAT		; Adjust back to data port for next rd.
	TEST	AL,STAT_ERR|STAT_DWF	; Test if error has occured
	MOV	AL,AH			; Put sector total back to AL
	JNZ	FDISK_ERROR
	INC	BL			; Increment sector count
	CMP	BL,BH			; Compare sector count
	JGE	.wrdone			; Don't set if counts are equal (done)
;-----	Reset interrupt bit
	MOV	AL,[ES:PORT_OFF]	; Get channel number
	AND	AL,06H
	SHR	AL,1			; Shift out master/slave bit
	MOV	CL,AL			; Create bit mask
	MOV	AL,1
	SHL	AL,CL
	OR	ES:ATA_STATUS,AL	; Apply to status byte
	JMP	.wrloop
.wrdone:
;-----	Update counters for next iteration
	ADD	[BP+wrk.ndone],BL	; Accumulate sector total
	MOV	BYTE [BP+wrk.sect],01H	; Reset sector count to 1
	MOV	AL,[BP+wrk.head]	; Increment head
	INC	AL
	CMP	AL,[ES:DI+diskparm.heads]	; Has head surpassed geometry?
	JB	.noheadc
	INC	WORD [BP+wrk.cyl]	; Advance cylinder
	SUB	AL,AL
.noheadc:
	MOV	[BP+wrk.head],AL	; Set updated head counter
	MOV	AL,[BP+wrk.ndone]	; Check for any sectors remaining
	CMP	AL,[BP+wrk.nsects]
	JB	.trkloop		; Do another read request
	SUB	AH,AH			; Return good status
.done:
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET	

;---------------------------------------------------------------:
; Verify sector(s)					AH=04H  :
;---------------------------------------------------------------:
DISK_VERIFY:
	CALL	PrepParams		; Prepare disk parameters
	JNC	.paramgood
	MOV	AH,BAD_SEEK
	SUB	AL,AL
	JMP	.done
.paramgood:
	MOV	BYTE [BP+wrk.ndone],00H	; Zero out sectors done count
;-----	Calculate pointer for the physical parameter table
	SUB	AH,AH
	MOV	AL,[ES:PORT_OFF]
	AND	AL,07H
%ifndef TARGET_XT
	SHL	AX,2			; Multiply number by 4
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
	MOV	SI,DPRM_TBL		; Base offset of internal param. table
	ADD	SI,AX
.trkloop:
;-----	Cap requested sectors to track bounds (new)
	MOV	AL,[BP+wrk.nsects]	; Load sector count requested
	SUB	AL,[BP+wrk.ndone]	; Subtract by total tallied
	MOV	AH,[ES:SI+diskparm.sects]
	CMP	AL,AH			; Check if cap is required
	JB	.nocap
	MOV	AL,AH			; Apply cap
.nocap:
	MOV	AH,[BP+wrk.sect]	; Load sector number
	DEC	AH			; Decrement to range 0-62
	ADD	AH,AL			; Add count to number
	SUB	AH,[ES:SI+diskparm.sects]
	JBE	.nocapb
	SUB	AL,AH
.nocapb:
;-----	Wait ready
;	SUB	BH,BH			; Load standard timeout value
;	MOV	BL,DISK_TIMEOUT
;	CALL	WaitRDY
;	PUSH	AX
;	MOV	AL,[ES:DISK_STATUS]	; Has wait invoked a timeout?
;	AND	AL,AL
;	POP	AX
;	JNZ	ERR_TIMEOUT		; Jump to timeout error
;	MOV	BYTE [ES:DISK_STATUS],0	; Clear disk status
;-----	Apply disk parameters for the current block
	MOV	AH,[BP+wrk.sect]
	MOV	CX,[BP+wrk.cyl]
	MOV	DH,[BP+wrk.head]
	CALL	SetCHS
;-----	Perform read request of current block
	MOV	BH,AL			; Transfer sector count of iteration
	MOV	AL,CMD_VERIFY		; Issue VERIFY command
	CALL	SendCMD
	PUSH	BX
	MOV	BL,DISK_TIMEOUT		; Load standard timeout value
	CALL	WaitRDY			; Wait for ready
	POP	BX
;-----	Check if operation returned an error
	MOV	AL,[BP+wrk.nsects]
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
	MOV	AH,AL
	ADD	DX,ATA_NSECT		; Get sector count from drive in case
	IN	AL,DX			; ... of drive error
;-----	Calculate remaining sectors to verify
	MOV	AH,[BP+wrk.ndone]	; Get sector total
	ADD	AH,BH			; Add count for iteration
	SUB	AH,AL			; Subtract by count from drive
	ADD	DX,ATA_STAT-ATA_NSECT	; Adjust to status register
;-----	Read status and acknowledge drive
	MOV	CX,15			; Guarantees acknowledge
	REP	IN AL,DX		; Read status register
	TEST	AL,STAT_ERR|STAT_DWF	; Test if error has occured
	MOV	AL,AH			; Transfer count from earlier
	JNZ	FDISK_ERROR
;-----	Add counters for next read iteration
	ADD	[BP+wrk.ndone],BH	; Add to sector total
	MOV	BYTE [BP+wrk.sect],01H	; Reset sector count to 1
	MOV	AL,[BP+wrk.head]	; Increment head
	INC	AL
	CMP	AL,[ES:SI+diskparm.heads]	; Has head surpassed geometry?
	JB	.noheadc
	INC	WORD [BP+wrk.cyl]	; Advance cylinder
	SUB	AL,AL
.noheadc:
	MOV	[BP+wrk.head],AL
	MOV	AL,[BP+wrk.ndone]	; Check if any sectors remain
	CMP	AL,[BP+wrk.nsects]
	JB	.trkloop		; Continue iterating
	SUB	AH,AH			; Return good status
.done:
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET

;---------------------------------------------------------------:
; Format track				     		AH=05H  :
;---------------------------------------------------------------:
DISK_FORMAT:
	AND	CL,0C0H			; Force sector number 1 just in case
	OR	CL,1
	CALL	PrepParams		; Prepare drive parameters
	JNC	.paramgood
	MOV	AH,BAD_SEEK
	SUB	AL,AL
	JMP	.done
.paramgood:
	MOV	SI,DI			; Move I/O offset to SI
	MOV	DS,[BP+wrk.ioSEG]	; Load I/O segment
;-----	Calculate offset of physical parameter table
	MOV	BL,AL			; Transfer sector count
	SUB	AH,AH			; Get drive number
	MOV	AL,[ES:PORT_OFF]	; Get port offset
	AND	AL,07H			; Mask off other bits
%ifndef TARGET_XT
	SHL	AX,3			; Multiply by 8
%else
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
	MOV	DI,DPRM_TBL		; Load offset to drive parameter table
;-----	Wait ready
;	SUB	BH,BH			; Load standard timeout value
;	MOV	BL,DISK_TIMEOUT
;	CALL	WaitRDY
;	MOV	AL,[ES:DISK_STATUS]	; Has wait invoked a timeout?
;	AND	AL,AL
;	JNZ	ERR_TIMEOUT		; Jump to timeout error
;	MOV	BYTE [ES:DISK_STATUS],0	; Clear disk status
;-----	Perform operation
	MOV	AH,01H			; Set sector to 1 just in case
	MOV	AL,[ES:DI+diskparm.sects]	; Get sector count from table
	MOV	CX,[BP+wrk.cyl]
	MOV	DH,[BP+wrk.head]
	CALL	SetCHS			; Apply parameters
	MOV	AL,CMD_FORMAT		; Send format command
	CALL	SendCMD
	CALL	WriteBLK
;-----	Wait for IRQ
	PUSH	BX
	MOV	BL,DISK_TIMEOUT
	CALL	WaitRDY
	POP	BX
;-----	Check result of operation
	MOV	AL,BL			; AL will return sectors per track 
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
	ADD	DX,ATA_STAT		; Adjust to status register
	MOV	CX,15			; Guarantees acknowledge
	REP	IN AL,DX		; Read status register
	SUB	DX,ATA_STAT		; Subtract back to data port
	TEST	AL,STAT_ERR|STAT_DWF	; Test if error has occured
	MOV	AL,BL			; AL will return sectors per track 
	JNZ	FDISK_ERROR		; Jump to disk error otherwise
	SUB	AH,AH			; Otherwise return good status
	MOV	AL,BL			; AL will return sectors per track 
.done:
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET

;---------------------------------------------------------------:
; Seek to cylinder					AH=0CH  :
;---------------------------------------------------------------:
DISK_SEEK:
	CALL	PrepParams		; Prepare drive parameters
;-----	Wait ready
;	SUB	BH,BH			; Load standard timeout value
;	MOV	BL,DISK_TIMEOUT
;	CALL	WaitRDY
;	PUSH	AX
;	MOV	AL,[ES:DISK_STATUS]	; Has wait invoked a timeout?
;	AND	AL,AL
;	POP	AX
;	JNZ	ERR_TIMEOUT		; Jump to timeout error
;	MOV	BYTE [ES:DISK_STATUS],0	; Clear disk status
;-----	Perform operation
	MOV	AL,[ES:DI+diskparm.sects]	; Get sector count from table
	MOV	CX,[BP+wrk.cyl]
	MOV	DH,[BP+wrk.head]
	CALL	SetCHS			; Apply parameters
	MOV	AL,CMD_SEEK		; Send SEEK command
	CALL	SendCMD
	MOV	BL,DISK_TIMEOUT
	CALL	WaitRDY
	SUB	AL,AL
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
	ADD	DX,ATA_STAT		; Adjust to status register
	MOV	AH,AL
	MOV	CX,15			; Guarantees acknowledge
	REP	IN AL,DX		; Read status register
	SUB	DX,ATA_STAT		; Subtract back to data port
	TEST	AL,STAT_ERR		; Test if error has occured
	MOV	AL,AH
	JNZ	FDISK_ERROR		; Jump to disk error otherwise
	SUB	AH,AH			; Otherwise return good status
	SUB	AL,AL			; Zero out AL
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET

;---------------------------------------------------------------:
; Recalibrate drive					AH=11H  :
;---------------------------------------------------------------:
DISK_RECAL:
	SUB	AX,AX			; Provide empty parameters
	INC	AH
	SUB	DH,DH
	SUB	CX,CX
	CALL	SetCHS			; Apply parameters
	MOV	AL,CMD_RECAL		; Send recalibrate command
	CALL	SendCMD
	MOV	BL,DISK_TIMEOUT
	CALL	WaitRDY
	SUB	AL,AL
	MOV	AH,ES:DISK_STATUS	; Check if error occurred
	AND	AH,AH			; WaitDisk sets DISK_STATUS on timeout
	JNZ	ERR_TIMEOUT
	ADD	DX,ATA_STAT		; Adjust to status register
	MOV	AH,AL
	MOV	CX,15			; Guarantees acknowledge
	REP	IN AL,DX		; Read status register
	SUB	DX,ATA_STAT		; Subtract back to data port
	TEST	AL,STAT_ERR		; Test if error has occured
	MOV	AL,AH
	JNZ	FDISK_ERROR		; Jump to disk error otherwise
	SUB	AH,AH			; Otherwise return good status
	SUB	AL,AL			; Zero out AL
	MOV	ES:DISK_STATUS,AH	; Set status byte
	RET

;---------------------------------------------------------------:
; Return with a drive error					:
;								:
; Expects: AL = Sector count					:
;---------------------------------------------------------------:
FDISK_ERROR:
	PUSH	AX
	IN	AL,DX
	MOV	CL,AL
	POP	AX
	MOV	AH,CL
	TEST	AH,STAT_DWF
	JZ	.E
	MOV	AH,WRITE_FAULT
	JMP	.return
.E:
	ADD	DX,ATA_ERR-ATA_STAT	; Adjust to error register
	PUSH	AX
	MOV	CX,15
.ioloop:
	IN	AL,DX
	LOOP	.ioloop
	MOV	CL,AL
	POP	AX
	MOV	AH,CL
	TEST	AH,ERR_AMNF		; Address mark not found
	JZ	.E0
	MOV	AH,BAD_ADDR_MARK
	JMP	.return
.E0:
	TEST	AH,ERR_TK0NF		; Track 0 not found
	JZ	.E1
	MOV	AH,BAD_TRACK		; Report as bad track
	JMP	.return
.E1:	
	TEST	AH,ERR_AMNF		; Address mark not found
	JZ	.E2
	MOV	AH,BAD_ADDR_MARK
	JMP	.return
.E2:
	TEST	AH,ERR_UNC		; Uncorrectable data error
	JZ	.E3
	MOV	AH,BAD_ECC
	JMP	.return
.E3:
	TEST	AH,ERR_BBK		; Bad block
	JZ	.E4
	MOV	AH,BAD_TRACK
	JMP	.return
.E4:
	TEST	AH,ERR_IDNF		; Sector ID not found
	JZ	.E5
	MOV	AH,RECORD_NOT_FND
	JMP	.return
.E5:
	MOV	AH,UNDEF_ERR
.return:
	MOV	ES:DISK_STATUS,AH
	RET

;---------------------------------------------------------------:
; Return with TIME_OUT error					:
;---------------------------------------------------------------:

ERR_TIMEOUT:
	MOV	ES:DISK_STATUS,AH
	RET

;---------------------------------------------------------------:
; =PrepParams							:
;	Reorganizes disk parameter values of INT 13H calls to	:
;	a more useable form. Resulting parameters are stored to	:
;	variables in the temporary work area (BP+wrk.*).	:
;	Additionally, drive parameters are also being checked	:
;	against bad disk coordinates or reading beyond the	:
;	parameters reported by INT 13h.				:
;								:
;	Hint: This is also an ideal location for performing	:
;	sector translation operations.				:
;								:
; Arguments:							:
;	CL	Sector (bit 0-5), cylinder high (bit 6-7)	:
;	CH	Cylinder low					:
;	DH	Head number					:
;	DL	Drive number					:
;---------------------------------------------------------------:
PrepParams:
	MOV	AH,CL			; Use AH as sector number
	AND	AH,03FH			; Mask off upper bits for cylinder
%ifndef TARGET_XT
	SHR	CL,6			; Shift upper 2 bits of cylinder down
%else
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
	SHR	CL,1
%endif	; TARGET_XT
	XCHG	CH,CL			; Exchange to correct byte order
	AND	AH,AH			; Check if sector number zero
	JZ	.badret
;-----	Calculate offset for the drive parameter table
	PUSH	AX
	SUB	AH,AH
	MOV	AL,DL
%ifndef TARGET_XT
	SHL	AX,2
%else
	SHL	AX,1
	SHL	AX,1
%endif	; TARGET_XT
%ifdef SINGLE_DRV
	ADD	AX,PARM_TBL
%endif	; SINGLE_DRV
	MOV	SI,AX
	POP	AX
;-----	Challenge the parameters
	CMP	CX,[ES:SI+diskparm.cyls]
	JAE	.badret
	CMP	DH,[ES:SI+diskparm.heads]
	JAE	.badret
;-----	Copy parameters to work area
	MOV	[BP+wrk.cyl],CX
	MOV	[BP+wrk.head],DH
	MOV	[BP+wrk.sect],AH
;-----	Test the read total
	MOV	AL,AH			; Move sector number
	DEC	AL			; Decrement sector number
	SUB	AH,AH			; Clear upper byte
	ADD	AL,[BP+wrk.nsects]	; Add count to number
	ADC	AH,0
	DEC	AX			; Decrement once more
	SUB	BH,BH			; Calc if params surpasses a track
	MOV	BL,[ES:SI+diskparm.sects]
	SUB	DX,DX			; Zero out DX
	DIV	BX			; Divide by sectors per track
	SUB	DH,DH
	MOV	DL,[BP+wrk.head]	; Add divided result with head number
	ADD	AX,DX
	DIV	BYTE [ES:SI+diskparm.heads]	; Divide by heads per cylinder
	SUB	AH,AH			; Clear remainder
	ADD	CX,AX			; Add to cylinder count
	CMP	CX,[ES:SI+diskparm.cyls]
	JAE	.badret
;-----	Do parameters need translation?
	TEST	BYTE [ES:PORT_OFF],08H	; Translate bit
	JNZ	.dotrans
	CLC
	RET
.dotrans:
;-----	Convert cylinder:head to absolute track
	SUB	AH,AH			; Multiply by heads per cylinder
	MOV	AL,[ES:SI+diskparm.heads]
	MUL	WORD [BP+wrk.cyl]	; Multiply head total by cylinders
	MOV	BL,[BP+wrk.head]	; Add head parameter
	ADD	AX,BX
	ADC	DX,0			; DX:AX = Absolute track
;-----	Convert absolute track back to cylinder:head
	PUSH	AX
	SUB	AH,AH
	MOV	AL,[ES:PORT_OFF]	; Get drive number
	AND	AL,07H			; Mask-in only drive number
%ifndef TARGET_XT
	SHL	AL,2			; Multiply by 4
%else
	SHL	AL,1
	SHL	AL,1
%endif	; TARGET_XT
	MOV	SI,DPRM_TBL		; Load parameter table
	ADD	SI,AX			; Apply offset
	POP	AX
	SUB	BH,BH			; Get drive's head total
	MOV	BL,[ES:SI+diskparm.heads]
	DIV	BX			; Divide absolute track with head total
	MOV	[BP+wrk.cyl],AX		; Set translated cylinder (quotient)
	MOV	[BP+wrk.head],DL	; Set translated head (remainder)
	CLC
	RET
.badret:
	STC
	RET
