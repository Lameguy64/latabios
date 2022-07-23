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
2022 Meido-Tek Productions - Released under MPLv2


Table of Contents

    1. Introduction
    2. Installation
	2.1. Note for 386+ Users
    3. Compatibility
	3.1. Workspace Storage
	3.2. ATA Drive Compatibility
	3.3. Disk BIOS Implementation Table
	3.4. Incompatible Systems
    4. Sector Translation
    5. References and Acknowledgements
    6. File Contents


1. Introduction

    L-ATA, short for Lame-AT Attachment,  is option ROM software that adds en-
    hanced support for ATA fixed disk drives  to  any  PC/AT compatible system
    equipped with a 80286 or newer processor. L-ATA not only offers hard drive
    auto-detection and support for up to four ATA channels,  but also provides
    sector translation to support drive capacities greater than 504 megabytes-
    allowing to address up to 8,032 megabytes  using the standard INT 13h disk
    interfaces.
    
    This software is intended  for upgrading older 80286 and 386 systems which
    do not support ATA drives or are constrained by hard-coded drive parameter
    tables.  L-ATA addresses  this  problem by augmenting the system BIOS with
    L-ATA's own disk BIOS in a manner similar to that of an option ROM present
    on SCSI or ESDI host adapters.  The ATA drives detected by L-ATA appear as
    ordinary fixed disk drives to any operating system.
    
   
2. Installation

    L-ATA is meant to be installed  as an option ROM starting at address C8000
    or higher. This is easily accomplished by means of a programmable ROM chip
    and a Network Interface Card.  The ATA interface itself can be provided by
    any 16-bit Multi I/O board.  In some cases the IDE interface  of  a  sound
    card is sufficient provided the interface is configured using jumpers  and
    not by software.

    When using a sound card,  make  sure  the  IDE  interface  is  set  to the
    following resources.  If  hard  disks are not being detected properly make
    sure an IOCHRDY or IORDY jumper is set.

	Port 1F0h IRQ 14 (Primary)
	Port 170h IRQ 15 (Secondary)
	Port 168h IRQ 10 (Tertiary)
	Port 1E8h IRQ 11 (Quaternary)

    The first ATA drive found will  be  used  as  the boot drive regardless of
    channel. However, protected mode  operating  systems  may fail if the boot
    drive is not connected to the first two ATA channels.

    L-ATA is provided in two versions; the 'normal' version and the 'sensible'
    version, the latter of which is indicated by an '-S' suffix on the version
    number.  The 'sensible' version differs  in  that  only one ATA channel is
    supported to allow the BIOS' work  area to be stored immediately after the
    BIOS Data Area, but this may provide better compatibility on some systems.  
    The normal version with quad ATA  channel support uses the normally unused
    first-half of the BIOS stack area.  Both versions  are  also provided with
    either a bootloader included or not.  The bootloader equipped variation is
    required for older systems which  do not boot the hard drive and expects a
    disk BIOS to provide a hard disk aware bootloader.

    Make sure that no fixed disk drives  are  defined in the CMOS settings, or
    else L-ATA will refuse to install as it may cause superfluous issues which
    may cause software to fail.


  2.1. Note for 386+ Users

    On 386 and newer systems,  memory between C8000 to EFFFF normally reserved
    for Option ROMs and memory mapped peripherals  can  be  reclaimed as extra
    memory known as Upper Memory Blocks (UMBs)  by  using a 386 memory manager
    such as EMM386. UMBs are most commonly used for device drivers and TSR
    software to free more conventional memory to applications.
    
    When an option ROM is installed, part of this memory space will be used by
    the option ROM itself  and  cannot be reclaimed as upper memory.  Start-up
    files may require changes for pre-existing  installations after the system
    has been upgraded with L-ATA.


3. Compatibility

    L-ATA has been tested mostly against clone systems equipped with  a  clone
    BIOS from vendors such as American Megatrends, Award Software, Quadtel and
    Microid Research to name a few, in which L-ATA has displayed a high degree
    of compatibility.


  3.1. Workspace Storage

    L-ATA stores its internal variables and parameter tables on memory address
    0030:0000 or 00300 which points to the unused first-half of the BIOS stack
    area worth 256 bytes, but  only  the last 128 bytes is actually used. As a
    result no amount of conventional memory has been effectively used by L-ATA
    to operating systems. User-specified  drive  parameters are stored in this
    area on AMI BIOS.

    For the sensible version,  the variables  and  parameter tables are stored
    just immediately  after  the  BIOS Data Area  at  address 0030:01E0 or the
    equivalent memory address of 0040:00E0 occupying 19 bytes.  As a result of
    using a much smaller work area this version only supports one ATA channel,
    but may offer better compatibility for some OEM systems. The sensible ver-
    sion still provides sector translation  to  support drives larger than 504
    megabytes.
    
    To ensure  that  software  does  not  overwrite the internal tables during
    operation- which may result  in  data corruption upon further disk access,
    L-ATA performs a checksum test of the internal tables on every disk access
    operation.  An 'INTERNAL TABLES TAINTED' screen  is  invoked if the tables
    have been corrupted to prevent damage to data on the drives.


  3.2. ATA Drive Compatibility

    L-ATA only supports ATA drives that recognizes the IDENTIFY command as the
    software relies on auto-detection.  Older drives which do not support this
    command will not work with L-ATA,  but such a drive would have to be early
    enough to have a suitable drive type entry in the system BIOS.

    L-ATA will not work with XTA drives or ATA interfaces on a 8-bit data bus.
    The ATA interface must be present with a 16-bit data interface.


  3.3. Disk BIOS Implementation Table

    L-ATA implements most of the INT 13h functions as supported by the IBM MFM
    Fixed Disk Controller.  The table below lists the implementation level for
    each function number.

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
    | D  - Dummy implementation, returns good response (CF=0, AH=00H)	|
    | Y  - Implemented							|
    | Y+ - Implemented, required by MS-DOS				|
    +-------------------------------------------------------------------+

    Refer to Ralph Brown's Interrupt List at http://ctyme.com/intr/int-13.htm
    for documentation regarding the usage of the above functions calls.


  3.4. Incompatible Systems

    Currently, only one machine is found to exhibit problems with L-ATA.

    * Toshiba T3200SX (with ROM 3C BIOS)

      Starting Windows for Workgroups with networking installed crashes with a
      black screen or returns back to MS-DOS,  but after showing the 'INTERNAL
      TABLES TAINTED' screen  for  a  very brief period.  Disabling networking
      solves this problem.


4. Sector Translation

    Sector translation is a workaround  for  supporting drives larger than 504
    megabytes.  Because the  INT 13h  functions have a 1024 cylinder limit yet
    supports up to 256 heads, and ATA drives could only address up to 16 heads
    by design of the ATA interface.  To support more cylinders requires trans-
    lation of the additional cylinders into additional heads, in which the INT
    13h interfaces do support.

    The new geometry for sector translation  is  determined by multiplying the
    head count until the resulting cylinder count is reduced to 1024 cylinders
    or less. The new cylinder count is obtained from the total track count (by
    multiplying the cylinder  count  with  the  head  count)  and dividing the
    result with the new head count. Naturally these translated coordinates are
    translated back to the drive's actual reported geometry for disk I/O.

    A drive which reports the following parameters:

	1532 cylinders, 15 heads, 63 sectors

    Will be translated to software using INT 13h interfaces as:

	766 cylinders, 30 heads, 63 sectors


5. References and Acknowledgements

    The L-ATA BIOS was developed using technical references such as the
    official ASC X3 document 791D "AT Attachment Interface for Disk Drives
    Revision 4C" (d0791r4c.pdf), the "IBM XT Fixed Disk Adapter" (IBM document
    6139790) and Ralph Brown's Interrupt List at http://ctyme.com/rbrown.htm

    L-ATA was developed without direct involvement of any "mainstream" group
    or community, with all testing conducted internally and independently. No
    source code of other similar solutions have been used as reference for
    the development of L-ATA and is therefore a completely from-scratch work.

    Thanks to HighTreason610 for assisting in testing L-ATA on a number of
    much older machines, including systems where other, more popular solutions
    have been found to cause problems.

    Despite not having a page dedicated to the L-ATA project, the rest
    of Lameguy's material can be found at http://lameguy64.net

    L-ATA BIOS Project page:

	    https://github.com/lameguy64/latabios

		€ﬂﬂﬂﬂﬂ€ ﬂﬂﬂ€‹ﬂ‹ ‹ﬂﬂ‹  €ﬂﬂﬂﬂﬂ€
		€ €€€ €  ﬂ‹€ ﬂ€‹‹‹ €€ € €€€ €
		€ ﬂﬂﬂ €  ‹ﬂ€ﬂ€€ ﬂﬂﬂﬂ‹ € ﬂﬂﬂ €
		ﬂﬂﬂﬂﬂﬂﬂ €‹ﬂ € € ﬂ ﬂ‹€ ﬂﬂﬂﬂﬂﬂﬂ
		ﬂﬂ €ﬂ‹ﬂ  ﬂ‹ ﬂﬂ€‹ﬂﬂ € ‹ﬂ‹‹ ‹‹ﬂ
		 ﬂﬂﬂﬂ€ﬂ€ﬂﬂ  ﬂ €€‹€  €‹‹€ﬂ‹ﬂ ‹
		 ‹ €€ﬂﬂﬂ €€ ‹ﬂﬂ€€‹  ﬂ €€‹‹‹‹€
		 €€ ﬂﬂﬂﬂ€ﬂﬂ ‹‹€ﬂ‹ ﬂ€ﬂﬂﬂ‹€ € €
		 ﬂ‹ﬂ ﬂﬂ‹  ‹€ﬂ  € ‹ﬂ€ﬂﬂ  ‹ﬂ‹‹
		 ﬂ ‹‹ ﬂ€ﬂ ﬂ‹ﬂ€ﬂﬂ€ ‹€  ‹ ﬂ€‹ ﬂ
		 ﬂﬂﬂ ﬂﬂﬂ‹€ ﬂ‹€ € ﬂﬂ €ﬂﬂﬂ€€ﬂﬂ
		€ﬂﬂﬂﬂﬂ€  ‹‹€‹€ ﬂ€€‹€€ ﬂ €ﬂ ‹‹
		€ €€€ € €€€ﬂﬂ€‹€€‹€ﬂﬂ€ﬂﬂﬂﬂ  €
		€ ﬂﬂﬂ € ‹€ €ﬂﬂ ﬂﬂﬂ  ﬂﬂ‹€ﬂ‹€ﬂ€
		ﬂﬂﬂﬂﬂﬂﬂ ﬂ     ﬂ     ﬂﬂ ﬂ ﬂ
