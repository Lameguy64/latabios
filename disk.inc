;---------------------------------------------------------------:
; L-ATA Fixed Disk BIOS - ATA I/O definitions			:
; Written by John 'Lameguy' Wilbert Villamor			:
; 2022 Meido-Tek Productions					:
;								:
; Main file: latabios.asm					:
;---------------------------------------------------------------:

;---------------------------------------------------------------:
; ATA Registers (relative to I/O port)				:
;---------------------------------------------------------------:

ATA_DATA	EQU	00H		; Data I/O register (16-bit R/W)
ATA_ERR		EQU	01H		; Error register (R)
ATA_FEAT	EQU	01H		; Set features register (W)
ATA_NSECT	EQU	02H		; Number of sectors (R/W)
ATA_SECT	EQU	03H		; Sector number (R/W)
ATA_CYLL	EQU	04H		; Cylinder low (R/W)
ATA_CYLH	EQU	05H		; Cylinder high (R/W)
ATA_DHEAD	EQU	06H		; Drive and head (R/W)
ATA_STAT	EQU	07H		; Status (R)
ATA_CMD		EQU	07H		; Command (W)

;---------------------------------------------------------------:
; ATA Registers (relative to Control port)			:
;---------------------------------------------------------------:

%ifndef TARGET_XT

;-----	AT port offsets
ATA_CT_ASTAT	EQU	0206H		; Alternate status register (R)
ATA_CT_DCTRL	EQU	0206H		; Device control register (W)
ATA_CT_DADDR	EQU	0207H		; Drive Address Register (R) (not used)

%else

;-----	XT-IDE offsets (base addr. 300H)
ATA_CT_ASTAT	EQU	0EH		; Alternate status register (R)
ATA_CT_DCTRL	EQU	0EH		; Device control register (W)

%endif ; TARGET_XT

;---------------------------------------------------------------:
; ATA Commands							:
;---------------------------------------------------------------:

CMD_NOP		EQU	000H		; No-operation (1)
CMD_FORMAT	EQU	050H		; Format track (2)
CMD_IDENTIFY	EQU	0ECH		; Identify drive (1)
;
; The read/write commands were erroneously described in decimal notation in
; the document d0791r4c.pdf... Naturally this resulted in a brief period of
; insanity.
;
CMD_READ	EQU	020H		; Read sector(s) (1)
CMD_WRITE	EQU	030H		; Write sector(s) (2)
CMD_VERIFY	EQU	040H		; Verify sector(s)
CMD_RECAL	EQU	010H		; Recalibrate (1)
CMD_SEEK	EQU	070H		; Seek (1)
CMD_SETFEAT	EQU	0EFH		; Set features (1)
CMD_SETMULTI	EQU	0C6H		; Set multiple mode (1)
CMD_INIT	EQU	091H		; Init drive parameters (1)
CMD_FORMAT	EQU	050H		; Format track (2)

;---------------------------------------------------------------:
; Bit definitions for ATA_ERR register				:
;---------------------------------------------------------------:

ERR_AMNF	EQU	000000001B	; Address mark not found
ERR_TK0NF	EQU	000000010B	; Track 0 not found
ERR_MCR		EQU	000000100B	; Media change requested
ERR_ABRT	EQU	000001000B	; Aborted command
ERR_IDNF	EQU	000010000B	; Sector ID not found
ERR_MC		EQU	000100000B	; Media changed
ERR_UNC		EQU	001000000B	; Uncorrectable data error
ERR_BBK		EQU	010000000B	; Bad block detected

;---------------------------------------------------------------:
; Bit definitions for ATA_STAT and ATA_CT_ASTAT registers	:
;---------------------------------------------------------------:

STAT_ERR	EQU	000000001B	; Error occurred on last operation
STAT_IDX	EQU	000000010B	; Index mark passed
STAT_CORR	EQU	000000100B	; Correctable data error occurred
STAT_DRQ	EQU	000001000B	; Data request
STAT_DSC	EQU	000010000B	; Drive seek complete
STAT_DWF	EQU	000100000B	; Drive write fault
STAT_DRDY	EQU	001000000B	; Drive ready
STAT_BUSY	EQU	010000000B	; Drive busy

;---------------------------------------------------------------:
; Bit definitions for ATA_DHEAD registers			:
;---------------------------------------------------------------:

DHEAD_HS0	EQU	000000001B	; 
DHEAD_HS1	EQU	000000010B	; Head select bits (0-15)
DHEAD_HS2	EQU	000000100B	; LBA bits 24-27 if LBA=1
DHEAD_HS3	EQU	000001000B	; 
DHEAD_DRV	EQU	000010000B	; Drive select
DHEAD_LBA	EQU	001000000B	; LBA mode

;---------------------------------------------------------------:
; Bit definitions for ATA_CT_DCTRL register			:
;---------------------------------------------------------------:

DC_IEN		EQU	000000010B	; Interrupt enable
DC_SRST		EQU	000000100B	; Software reset (manual clear)

;---------------------------------------------------------------:
; Bit definitions for ATA_CT_DADDR register (note: inverted)	:
;---------------------------------------------------------------:

DA_DS0		EQU	000000001B	; Drive 0 selected
DA_DS1		EQU	000000010B	; Drive 1 selected
DA_HS0		EQU	000000100B	; 
DA_HS1		EQU	000001000B	; Head select bits (0-15)
DA_HS2		EQU	000010000B	;  
DA_HS3		EQU	000100000B	;
DA_WTG		EQU	001000000B	; Write gate

;---------------------------------------------------------------:
; Struct for data returned by the IDENTIFY command		:
;---------------------------------------------------------------:

STRUC		drvinfo
.config		RESW	1		; General configuration
.numcyls	RESW	1		; Number of cylinders
.res1		RESW	1		; Reserved
.numheads	RESW	1		; Number of heads
.numbtrack	RESW	1		; Unformatted bytes per track
.numbsect	RESW	1		; Unformatted bytes per sector
.numsects	RESW	1		; Number of sectors per track
.vendor		RESW	3		; Vendor specific
.serial		RESB	20		; Serial number (byte-swapped ASCII)
.bufftype	RESW	1		; Buffer type
.buffsize	RESW	1		; Buffer size
.ecclen		RESW	1		; Number of ECC bytes for long commands
.firmrev	RESB	8		; Firmware revision (byte-swapped ASCII)
.model		RESB	40		; Drive model (byte-swapped ASCII)
.multicap	RESW	1		; Multiple commands capabilities
.dwordcap	RESW	1		; Double-word I/O capabilities
.diskcaps	RESW	1		; Drive capabilities
.res2		RESW	1		; Reserved
.piocaps	RESW	1		; PIO transfer timing mode
.dmacaps	RESW	1		; DMA transfer timing mode
.res3		RESW	1		; Reserved
.ncurrcyls	RESW	1		; Number of current cylinders
.ncurrheads	RESW	1		; Number of current heads
.ncurrsects	RESW	1		; Number of current sectors per track
.ncurrtotal	RESW	2		; Current capacity in sectors
.multstat	RESW	1		; Multiple sector status
.nlbatotal	RESW	2		; Total number of sectors (LBA mode)
.swdmastat	RESW	1		; Single-word DMA status
.mwdmastat	RESW	1		; Multi-word DMA status
ENDSTRUC

