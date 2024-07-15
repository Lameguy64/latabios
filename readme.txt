		     ____           ______________
		    |    |         |              |
		    |    |  __     |____      ____|
		    |    | |__|  _____  |    |  _____
		    |    |____  |  _  | |    | |  _  |
		    |         | | |_| | |    | | |_| |
		    |_________| |_| |_| |____| |_| |_|
		     ________________________________
		    |                                |
		    |________________________________|

L-ATA Fixed Disk Support BIOS by John "Lameguy" Wilbert Villamor
2022-24 Meido-Tek Productions - Released under MPLv2
Version 1.01


Table of Contents

    1. Introduction
    2. Installation
        2.1. Installing on PC/AT Systems
	2.2. Installing on PC/XT Systems
	2.3. L-ATA Builds
	2.4. Note for 386+ Users
    3. Compatibility & Internal Operation
	3.1. Workspace Storage
	3.2. ATA Drive Compatibility
	3.3. Disk BIOS Implementation Table
	3.4. Incompatible Systems & Software
    4. Sector Translation
    5. Version History
    6. References and Acknowledgements


1. Introduction

    L-ATA, short for Lame-AT Attachment, is an option ROM for adding enhanced
    support for ATA disk drives to both PC/XT and PC/AT compatible platforms-
    to be able to use more conventional ATA disks (including CompactFlash) on
    these older platforms.
  
    L-ATA not only adds full ATA disk support with automatic drive detection,
    but also disk capacities greater than 504 megabytes through sector trans-
    lation- it can map drives up to a maximum of 8,032 megabytes, the maximum
    imposed by the upper limit of INT 13h.

    L-ATA offers excellent system compatibility  by  means of proper software
    implementation- it functions not too dissimilar to the boot ROM of a SCSI
    or ESDI host adapter and has exhibited a great deal of compatibility with
    many BIOS vendors- clone or otherwise.
    
    This ROM software is intended for  use  on  PC/XTs and early PC/AT systems
    that predated support for custom drive parameters in the CMOS setup- or to
    upgrade a 386 or early 486 system to be able to use drive capacities grea-
    ter than 504 megabytes.
    
   
2. Installation

    L-ATA is option ROM software and requires a programmable ROM chip  (EPROM
    or EEPROM,  ideally 64 to 256kbit) and accompanying programmer to install
    for both PC/XT or PC/AT.
    
    Once burned the most inexpensive method  of  installing the ROM is to use
    an Ethernet adapter- which often feature a socket for an RPL ROM.
    
    ROMs attached to this socket usually appear  as standard adapter ROMs and
    is what L-ATA needs for installation. The ROM is either configurable with
    jumpers or in software with a configuration utility depending on adapter.
    In any case,  L-ATA should be mapped at address C8000 and other disk rel-
    ated option ROMs from SCSI controllers should be disabled,  if the system
    has any.
    
    The type of programmable ROM to use for this installation also depends on
    the Ethernet adapter  to  be used- very old ones may lack bus buffers and
    would simply not work or prevent the system from working altogether if an
    EEPROM is used- older UV-erasable EPROMs  or  PROMs should be used on the
    older adapters instead.

    Worth noting that using ROM sizes smaller  than what is actually install-
    ed, perhaps an attempt to save UMB space, may not work properly, and keep
    in mind that unused parts of the ROM are still usable for UMBs-  at least
    for 386s.


  2.1. Installing on PC/AT Systems

    The 16-bit 'sensible' and standard builds of L-ATA  must  be used to take
    advantage of the 16-bit data bus. L-ATA can run with ROM shadowing on 386
    systems to gain  a  small performance improvement, especially when sector
    translation is being employed.

    L-ATA works with standard ATA interfaces provided  by  Multi-I/O cards or
    the system board itself.  Sometimes the IDE interface on a sound card can
    be used for hard disks despite being intended for CD-ROM drives, but this
    method *will* work provided it is configured using jumpers and not  by  a
    software driver- which most of them are.

    The standard L-ATA ROM tests the following resources for hard drives, the
    standard one only tests the primary channel (alternate ports are included
    for reference in case of hardware conflicts):

	Port 1F0-1F7h, 3F6-3F7h IRQ 14 (Primary)
	Port 170-1F7h, 376-377h IRQ 15 (Secondary)
	Port 168-16Fh, 36E-36Fh IRQ 10 (Tertiary)
	Port 1E8-1EFh, 3EE-3EFh IRQ 11 (Quaternary)

    The first ATA drive discovered will be used as disk 0 regardless of which
    channel it was found.  However,  protected mode operating systems that do
    not use INT 13h may fail if the boot  drive is not connected to the first
    two ATA channels (see 3.4. Incompatible Systems & Software).


  2.2. Installing on PC/XT Systems

    The 8-bit 'XTI' build of L-ATA must  be  used for PC/XTs when used with a
    XT-IDE compatible adapter.

    Apart from which build of L-ATA  to  use,  installation on PC/XTs differs
    entirely on the interface. Since PC/XTs generally use 8088s and therefore
    only have 8-bit ISA slots, but the ATA interface requires  a  16-bit data
    bus for the data port- a special interface adapter is required.

    Currently L-ATA supports 'XT-IDE' compatible methods with the 'XTI' build
    (see section 2.2 for details). Currently L-ATA is hard wired to test only
    port 300h for drives and hardware interrupts are not used  (because soft-
    ware emulators did not implement it),  but this can be  adjusted  in  the
    assembler source.
    

  2.2. L-ATA Builds

    The L-ATA pre-built archive includes three variations;  the standard ver-
    sion which includes quad ATA support, the 'sensible' version with support
    for only a single ATA channel and the  'XTI'  version for use with PC/XTs
    with an XT-IDE adapter.

    Additional variations  to  fit  very specific configurations can be built
    from the L-ATA assembler source.

    Since version 1.01 all three variations include a bootloader.


  2.3. Note for 386+ Users

    On 386 and newer systems, unused portions of the adapter space  (the last
    384K of the first 1MB of memory) is  often reclaimed as system memory for
    use as Upper Memory Blocks or UMBs, typically provided by an expanded me-
    mory manager such as EMM386.

    As L-ATA appears as an option ROM it  occupies some of this adapter space
    for itself-  reducing available UMBs by 4KB (the present size of the ROM)
    even when the ROM mapped is larger thanks  to  ROM shadowing supported by
    most later 386 boards. This results in a reduction of UMB memory and TSRs
    may end up loading in conventional memory when upgrading an existing sys-
    tem with L-ATA.


3. Compatibility & Internal Operation

    L-ATA has been tested mostly against clone  systems  equipped  with  BIOS
    software from vendors  as  American Megatrends,  Award Software,  Quadtel
    and Microid Research to name a few,  to  which L-ATA has displayed a high
    degree of compatibility with.


  3.1. Workspace Storage

    Standard L-ATA stores its internal variables and parameter tables on mem-
    ory address 0030:0000 or 00300, using  the  unused half of the BIOS stack
    area of 256 bytes. As a result, no amount of precious conventional memory
    has been reserved to L-ATA from operating systems. AMI BIOS  also  stores
    its user-specified drive parameters this area as well.

    For both 'sensible' and 'XTI' versions,  internal variables and parameter
    tables are stored just after the BIOS Data Area at address 0030:01E0,  or
    the equivalent memory address of 0040:00E0,  occupying 19 bytes.  Due  to
    the smaller work area this version  only supports  a  single ATA channel,
    but may provide better compatibility on some OEM systems that use more of
    the BIOS stack area,  or  have  special hardware mapped to I/O ports that
    happen to conflict with extra ATA channels.

    To ensure  that  software  does  not overwrite the internal tables during
    operation  which  may result in data corruption upon further disk access,
    L-ATA performs a checksum test of the internal tables for every disk ope-
    ration.  An 'INTERNAL TABLES TAINTED' screen is invoked if the tables are
    found  to  be corrupted in the next disk access to prevent damage to data
    on the drives.


  3.2. ATA Drive Compatibility

    L-ATA relies on the IDENTIFY command to obtain parameters of the attached
    drives- older drives predating the official ATA specification may not im-
    plement the IDENTIFY command and therefore, will not work with L-ATA. But
    such an old drive should have a type entry in the fixed parameter tables.

    However,  some hard drives have been found to exhibit quirks when used as
    a slave drive- some IBM Travelstar drives meant  for  laptops inherit the
    CHS parameters of the master drive, even when the parameters yield a cap-
    acity much greater than the drive is rated for.  This  is  unlikely a bug
    with L-ATA as it was able to obtain the correct model string from the
    drive, and this appears right after the CHS parameter fields.


  3.3. Disk BIOS Implementation Table

    L-ATA implements most of the INT 13h functions  as  supported by the ori-
    ginal hard disk controller for the PC; the IBM MFM Fixed Disk Controller.
    
    Only functions applicable to the PC/AT platform are implemented.
    The table below lists each function and their implementation level.

    +-------------------------------------------------------------------+
    | Func. | Status | Description					|
    |-------|--------|--------------------------------------------------|
    |  00H  |   D    | Reset Disk System				|
    |  01H  |   Y+   | Get status of last operation			|
    |  02H  |   Y+   | Read sector(s)					|
    |  03H  |   Y+   | Write sector(s)					|
    |  04H  |   Y+   | Verify sector(s)					|
    |  05H  |   Y+   | Format track					|
    |  06H  |   N    | Format track & set bad sector flags (XT only)	|
    |  07H  |   N    | Format drive (XT only)				|
    |  08H  |   Y+   | Get drive parameters				|
    |  09H  |   D    | Initialize drive parameters			|
    |  0AH  |   N    | Read long sector(s) (optional)			|
    |  0BH  |   N    | Write long sector(s) (optional)			|
    |  0CH  |   Y    | Seek to cylinder					|
    |  0DH  |   D    | Reset hard disks					|
    |  0EH  |   N    | Read sector buffer (XT only)			|
    |  0FH  |   N    | Write sector buffer (XT only)			|
    |  10H  |   D    | Test drive ready					|
    |  11H  |   Y    | Recalibrate drive				|
    |  12H  |   N    | Controller RAM diagnostic (XT only)		|
    |  13H  |   N    | Drive diagnostic (XT only)			|
    |  14H  |   N    | Controller diagnostic (XT only)			|
    +-------------------------------------------------------------------+
    | N  - Not implemented, returns bad response (CF=1, AH=01H)		|
    | D  - Dummy implementation- returns good response (CF=0, AH=00H)	|
    | Y  - Implemented							|
    | Y+ - Implemented, required by MS-DOS				|
    +-------------------------------------------------------------------+

    Refer to Ralph Brown's Interrupt List at http://ctyme.com/intr/int-13.htm
    for documentation regarding the usage of the above functions calls.


  3.4. Incompatible Systems & Software

    Only two things have been found to be incompatible with L-ATA so far;

    * Toshiba T3200SX (with ROM 3C BIOS)

      Starting Windows for Workgroups 3.11 with networking installed using an
      NE2000 compatible Ethernet adapter  would  either exit to MS-DOS during
      start-up, or crash with a black screen-  but  not  before  invoking the
      'INTERNAL TABLES TAINTED' message for a split second.
      
      It is not yet known if this problem still occurs when using a different
      Ethernet adapter,  but WfW 3.11 with networking has been tested to work
      properly with L-ATA- albeit with a different adapter and system.

    * Drives translated by L-ATA will  not  work  properly on early protected
      mode operating systems that  bypass  the  BIOS,  such  as  OS/2 1.3 and
      Novell Netware 286 and 386.

      A workaround is to assemble  a  build of L-ATA with the  'NO_TRANSLATE'
      preprocessor definition set- this disables sector translation and mere-
      ly caps the cylinder count  for  drives larger than 504MB.  Conversely,
      only the first first 504MB of the drive may be usable.


4. Sector Translation

    Sector translation is a workaround  for  supporting  drives larger than 504
    megabytes through the  INT 13h  BIOS interface.  Because INT 13h has a 1024
    cylinder limit yet supports up to  256 heads,  but  the  ATA interface only
    supports addressing 16 heads-  to support drives  with  more cylinders than
    what is supported by INT 13h requires translation  of  the additional
    cylinders into additional heads.

    The new geometry for sector translation  is  determined by first obtaining
    the total track count from multiplying  the head count with the cylinders.
    Next,  the head count is initially multiplied by a factor of 2 and the re-
    sult is then used to obtain the new cylinder count from dividing the total
    track count.  If the result is not less than 1152 cylinders  (threshold is
    adjustable in source), the head count is  multiplied further with an in-
    crementing factor until either the resulting cylinders falls within the
    threshold, or the head count has maxed out at 255.

    The following describes the above description as BASIC-like pseudo-code:

        total_cylinders = heads * cylinders
	mulfactor = 2

	repeat:

	new_heads = heads * mulfactor
	if new_heads > 255 then new_heads = 255

	new_cylinders = total_cylinders / heads

	if new_cylinders > 1152 then
	    if new_heads < 255 then
		mulfactor = mulfactor + 1
		goto repeat
	    end if
	end if

    For example, a drive that reports the following disk parameters:

	1532 cylinders, 15 heads, 63 sectors

    Is translated to the following parameters for INT 13h:

	766 cylinders, 30 heads, 63 sectors

    Naturally disk coordinates that correspond  to these translated geometries
    are internally translated back to 'native' coordinates of the drive-  this
    process occurs completely transparent to software using INT 13h.


5. Version History

    1.00    * Initial version.


    1.01    * Added XT support through XT-IDE style I/O on port 300h without
	      hardware interrupts- dubbed with an '-XTI' suffix, but not yet
	      tested on real hardware. Interrupts supported but optional.

	    * Removed DEBRAND build option as anyone desiring to plagiarize
	      this work or 'for appropriation' can just as easily edit the
	      strings anyway- string sections become messy with preprocessors
	      otherwise.

	    * Added NO_TRANSLATE preprocessor definition to disable disk
	      sector translation- limiting the disk BIOS to 504MB.

	    * Added CRLF after the 'Device Scan Complete' message- inserting
	      a space between the disk BIOS and operating system messages if
	      the system BIOS doesn't clear the  screen after running option
	      ROMs.

	    * Fixed detection problems when a CD-ROM drive is present on any
	      ATA channel.


6. References and Acknowledgements

    The L-ATA BIOS was developed using  official technical references as ASC
    X3  Document  791D "AT Attachment Interface for Disk Drives Revision 4C"
    (d0791r4c.pdf), the "IBM XT Fixed Disk Adapter" (IBM document #6139790).

    Ralph Brown's Interrupt List at http://ctyme.com/rbrown.htm was also used
    for the BIOS interfaces of L-ATA.

    L-ATA was developed "from scratch" with  no involvement from "mainstream"
    online communities-  no line of code or reverse engineering of other sol-
    utions have been lifted from  or  referred  to  during the development of
    L-ATA.

    Thanks to HighTreason610 for testing L-ATA on a number of older machines,
    including ones where  'mainstream'  solutions  have been found to exhibit
    severe problems.

    Despite not having a page dedicated to the L-ATA project, the rest
    of Lameguy's material can be found at http://lameguy64.net

    L-ATA BIOS Project Repository:

	    https://github.com/lameguy64/latabios

		ÛßßßßßÛ ßßßÛÜßÜ ÜßßÜ  ÛßßßßßÛ
		Û ÛÛÛ Û  ßÜÛ ßÛÜÜÜ ÛÛ Û ÛÛÛ Û
		Û ßßß Û  ÜßÛßÛÛ ßßßßÜ Û ßßß Û
		ßßßßßßß ÛÜß Û Û ß ßÜÛ ßßßßßßß
		ßß ÛßÜß  ßÜ ßßÛÜßß Û ÜßÜÜ ÜÜß
		 ßßßßÛßÛßß  ß ÛÛÜÛ  ÛÜÜÛßÜß Ü
		 Ü ÛÛßßß ÛÛ ÜßßÛÛÜ  ß ÛÛÜÜÜÜÛ
		 ÛÛ ßßßßÛßß ÜÜÛßÜ ßÛßßßÜÛ Û Û
		 ßÜß ßßÜ  ÜÛß  Û ÜßÛßß  ÜßÜÜ
		 ß ÜÜ ßÛß ßÜßÛßßÛ ÜÛ  Ü ßÛÜ ß
		 ßßß ßßßÜÛ ßÜÛ Û ßß ÛßßßÛÛßß
		ÛßßßßßÛ  ÜÜÛÜÛ ßÛÛÜÛÛ ß Ûß ÜÜ
		Û ÛÛÛ Û ÛÛÛßßÛÜÛÛÜÛßßÛßßßß  Û
		Û ßßß Û ÜÛ Ûßß ßßß  ßßÜÛßÜÛßÛ
		ßßßßßßß ß     ß     ßß ß ß
