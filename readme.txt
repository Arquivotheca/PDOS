          PDOS - Public Domain Operating System
          -------------------------------------

Version 0.85, released 2002-07-26
Written by Paul Edwards, kerravon@w3.to
Released to the public domain
You may use this entire package for any purpose whatsoever
without restriction.


INTRODUCTION
------------

There are two aspects to PDOS.

1. Practical - I'm not sure what the market requirement
for this is, but if you need to distribute a floppy disk
or bootable CDROM which contains a 16-bit MSDOS program,
that doesn't use a lot of services, and mainly you just
need an MSDOS-like operating system that will load your
program and allow you to read files, and perhaps then
take over the whole computer, maybe a game, then PDOS will
allow you to do this without having to pay licensing fees.
In addition, you can modify the source and not be bound
by or worry you are breaking, any licencing restrictions.
The PDOS floppy disk contains 100% public domain code,
including the boot sector, the loader, the operating
system and the command processor.  A 32-bit version can
also be compiled, but then it obviously ceases to be
a clone of MSDOS.  A bootable CDROM can be created from
a bootable floppy using CD burner software.

2. Theoretical - This is about as basic an operating
system as you are likely to find if you want to see what
the requirements are to write an operating system.  It is
the 32-bit version that I am most interested in.  It is
32-bit from both a user's point of view and a 
programmer's point of view.  Prior to reaching version 
1.00 the internals are subject to change, pending any 
suggestions anyone may have on technical issues.  I am 
mainly concerned about the programming interface, the 
rest (e.g. task protection, virtual memory) can change 
quietly.  I do not profess to be an operating system 
expert or even a DOS expert.  I have spent most of my 
life avoiding anything that falls out of the scope of 
strictly conforming ISO C code.  There is also a 16-bit 
version of PDOS available, which you may find more 
convenient to use for testing purposes, since any DOS 
C compiler can be used to write programs for it, while
the only compiler I know that works for the 32-bit 
version is EMX 0.9d.


The package more accurately contains multiple,
mostly independent things:

1. BOS application programming interface, for 
standardized access to the BIOS.

2. POS application programming interface, for 
standardized access to MSDOS interrupts.

3. PDPCLIB, a C runtime library, for multiple
environments, e.g. MVS, MSDOS and any POS-compliant
operating system (e.g. MSDOS and PDOS are both
POS-compliant).

4. PDOS-16, an operating system which can be
considered a clone of MSDOS.

5. PDOS-32, a 32-bit operating system, which
can't be considered a clone of any existing
operating system (that I know of, anyway).
It shares similarities with MSDOS, except it 
is 32-bit.  It shares similarities with DOS 
extenders, except the API is different, and
it doesn't require an OS for support.  It shares
similarities with Unix (the executable format),
except it doesn't support the Posix API.  So all
in all, it's a new operating system which thus
necessitates all applications to be recompiled,
although not necessarily rewritten.  At a later
stage, it may support the Windows NT API so that
32-bit console mode executables can be run
unchanged.


INSTALLING
----------

To get the 16-bit version on floppy disk, you need
to get hold of a program called "rawrite" from the
internet and then follow the instructions to get
pdos16.dsk onto a 1.44 MB floppy disk.  pdos32.dsk
will give you the 32-bit version, but it is a lot
less useful unless you're planning on rewriting
(or maybe recompiling) your software.

To install on hard disk, you need MSDOS (I tested
with version 5.0) to format a FAT-16 partition,
then edit the doinst.bat file, choose location of
executables you wish to use (use full path including
drive letter!), plus the drive you want to install
it on.  PDOS will happily run from a logical
partition.  You can't use Win98 or Freedos to do
this work, because the "sys" command produces an
incompatible boot sector.


DEVELOPMENT ENVIRONMENT
-----------------------

I use the following bits of software.  I hope that
in the future I can eliminate the commercial software
to make it more easily accessible, probably using
Open Watcom.

MSDOS 5.0
Borland C++ 3.1 for DOS

Turbo C++ 1.01
http://bdn.borland.com/article/0,1410,21751,00.html

TASM 3.1 for DOS

Not used:
Arrowsoft Assembler 1.00d
ftp://ftp.simtel.net/pub/simtelnet/msdos/asmutl/valarrow.zip

Not used:
NASM 0.98:
http://sourceforge.net/projects/nasm

Not used:
MASM 6.15:
http://msdn.microsoft.com/vstudio/downloads/tools/ppack/download.aspx


EMX 0.9d (which includes GCC 2.8.1) can be obtained from:
http://hobbes.nmsu.edu/cgi-bin/h-browse?dir=/pub/os2/dev/emx/v0.9d

raread/rawrite (unknown origin)


RECOMPILING
-----------

If you have the above software, you can just type "build"
and it will rebuild the two DSK files used in the shipment.
Note that you need to edit build.bat and put the proper
paths in before that will actually work.

The step by step guide is below...


To compile 16-bit version using Borland C++ 3.1:

Go to pdpclib and type "compile"
Go to src and type "comp1" then "comp2" then "comp3".
Then type "bcc -c -ml -I..\pdpclib world.c"
then ...
tlink -x ..\pdpclib\dosstart.obj+world.obj,world.exe,,..\pdpclib\borland.lib,

To compile 32-bit version using EMX 0.9d and Borland C++ 3.1:

Go to pdpclib and type "compp"
Go to src and type "comp4" then "comp5" then "comp6".
Then type "gcc -s -c -I../pdpclib world.c"
then "ld -s -o world ../pdpclib/pdosst32.o world.o ../pdpclib/pdos.a
then "ren world world.exe"

Run comp0 and then doinst (after editing it!!!) 
to install it.
NOTE! Installing PDOS in this manner will use
the MSDOS boot sector rather than PDOS's one.
The MSDOS boot sector is potentially copyrighted
(but the funny thing is that if it is, even a
blank formatted disk is copyright!).  Their boot
sector is better than PDOS's, but it's up to you
whether you use it or not.

The boot floppy (which uses PDOS's boot sector)
was created in the following manner:

format a floppy disk using MSDOS 5.0
run doinst using the 16-bit version
raread to get the floppy into a file, e.g. pdos16.dsk
compb
compbu
bootupd pdos16.dsk pbootsec.com
rawrite pdos16.dsk back to floppy


HISTORY
-------

Some years ago (circa 1988?) someone made a comment "until DOS is made
32-bit, DOS-extenders are just a kludge".  I didn't understand the
comment at the time, and I'm still not sure what's classified as a
"kludge".  A few years later (circa 1994?) I was in a job where I was
battling to try to get some DOS applications to run in 640k, moving
around device drivers, making use of the high memory area etc, all
very frustrating.  On a machine with 32 meg of memory on it!  I thought
it would make more sense to have DOS reside outside of real mode
memory, making more room for applications.  Nothing had been done about
making DOS 32-bit.  So, I decided to make my own 32-bit port of DOS.
But I don't like writing non-ISO C programs, so decided to start by
writing wrappers over all the DOS + BIOS functions.  This was just one
of many projects I was working on so nothing much happened for years.

Then I bought a cheap ($80) second-hand computer for use in the bedroom,
and didn't have the energy to install anything other than DOS on it.
And there's not much I actually wanted to do with DOS.  So I decided
to port another one of my projects (PDPCLIB) to DOS.  After that was
done, the only other thing that I could do under DOS was write my own
DOS, so I proceeded to write a 16-bit version of DOS, but nice and
clean, ie don't use near/far pointers.  With the intention that when
finished, all I needed to do was recompile under a 32-bit compiler and
voila!

The original point of contention was to free up real memory, but since
I am no longer in that position, the goal has instead changed to simply
producing a nice, clean 32-bit version of DOS, mainly for my own use
in writing my own ISO C programs.  But that raises the question, what 
does a 32-bit version of DOS actually look like?  This is my interpretation
of that originally posed question "until DOS is made 32 bit".  I figured
that you should be able to do things like write to location 0xb8000
without anyone complaining, but now you just addressed it as location
0xb8000!  However, with the compiler that I was using (EMX 0.9b), the
a.out executable format didn't include fixups, it assumed that CS and
DS would be pointing to where the program was loaded!  Rather than try
to change this, I just decided to provide an ABSADDR macro, so that you
can go "char *p = ABSADDR(0xb8000);" and then start writing directly to
screen memory.  ABSADDR refers to an operating system provided variable
which is how many bytes need to be subtracted from an address in order
for it to be able to access that absolute memory location.


PROGRAM INTERFACE
-----------------

The startup code for C programs needed to change for the 32-bit
version (this is something that programmers don't normally see,
it's handled by the C runtime library).  This meant deciding on
a new interface.  I took this opportunity to make a guess at what
the "perfect C environment" was.  I decided on this:

On startup, the program is called via a near call, with 4 parameters.
The first 3 are integers, whose value is currently undefined, but may
potentially be changed to argc, argv and one we won't talk about.  This
is not going to happen immediately, as doing C-style parsing of 
parameters is deciding on an overhead for one particular language
without anyone getting a say in the matter.  At the moment, I think
it's probably better for the program to decide that.

To exit the program, you can either do the normal DOS 0x4c call, or
you can do a near return, up to you.  Ideally what happens, is that
the executable has an entry point of main, it generates a call to
__main (e.g.), which in turn, skipping the stack changes generated
by main, uses the 4th parameter (described later), to go and change
the first 2-3 parameters.  That way on return to main, any accesses
to argc and argv will be correct, assuming the compiler generates
standard code like EMX does.  Not even the CRT needs to have any
assembler code!  Unfortunately EMX only generates this "standard
code" when optimization is off.  With optimization on, it pushes
more things onto the stack before calling __main, so is 
unpredictable.  It can probably be changed.

Now return from main doesn't give you a chance to call your exit
routine to close files etc.  So what __main does is set a value
(via the 4th parameter) of a routine to be called "by the operating
system" after main has exited.

However, I don't know how to make EMX actually set the entry point
to main!  So I haven't actually implemented it that way in PDPCLIB.
Instead I have done the traditional form of startup.  Also note that
if you exit via interrupt 4c, there is no 4th-parameter last-minute
callback facility, so call exit routines yourself before invocation.

The 4th parameter is a pointer to the following things:
1. integer, specifying length of this pointer, including the length
itself.
2. integer containing the value that needs to be subtracted from 
absolute addresses to make them addressable in this address space.
3. pointer to program segment prefix
4. pointer to command line argument
5. address of callback routine for program exit, 
initially set to 0 (no callback).
6. (unimplemented) a pointer to the entire Pos API.
7. (unimplemented) a pointer to the entire Bos API.
8. (unimplemented) a pointer to an entire C runtime library (actually
the same one used by pcomm PLUS a suggested startup routine for
calling by __main, so that the CRT that you need to link with has to
do virtually nothing, just accept this pointer, call the startup
routine, then all other C function calls are implemented via calls to
this parameter block.  This is sort of like using a DLL.  EXCEPT!  The
way DLL's work is to use memory-mapping so that they can virtually
install themselves in very high memory.  I don't currently have
memory mapping available so whilst the CRT mechanism is implemented, it
doesn't work, and won't work in the short term.


POS INTERFACE
-------------

Instead of calling int 21h directly yourself (which will currently
still work), you should instead use the routines provided in header
file "pos.h", which is the official access to the OS, similar to
OS/2's "os2.h".  For those who need to access the BIOS directly,
the routines are in "bos.h".  As with normal DOS, you're not meant
to be accessing the BIOS directly, but there's nothing to stop you
anyway.  You can access the hardware directly too, although you're
not meant to.

Here's an example of a BIOS routine, BosDiskSectorRead.

int BosDiskSectorRead(void         *buffer,
                      unsigned int  sectors,
                      unsigned int  drive,
                      unsigned int  track,
                      unsigned int  head,
                      unsigned int  sector);
                      
This particular function is meant to be called instead of INT 13 function 2.
                      
Now the first thing you should know is that because this is meant to be an
interface to INT 13 function 2, you should not expect to get more out of it
than INT 13 will give you.  E.g. even though the sector number above is
given as an unsigned int, the interrupt has a limit of 255 for sector
number, so that's all you will get.  The unsigned int is designed for
future expansion (ie where this interrupt is bypassed).

Potentially there will be multiple things that happen when you call
BosDiskSectorRead.

1. 16-bit program under 16-bit PDOS.
   It simply calls the interrupt.  This is designed for you to continue 
   writing DOS programs as you always have.  Will even work on an XT.  
   Of course your parameters must be far pointers, which means they are 
   equally constrained to addresses in the first meg.
   
2. 32-bit programs under 16-bit PDOS.
   All addresses are 32-bit.  If your program is running under plain DOS,
   the addresses will be translated to far pointers and the function will
   be done.  You do not need to make sure your addresses are in the lower 
   1 meg, the data will be buffered and converted accordingly.

3. 16-bit program under 32-bit PDOS.
   It calls the interrupt, which is then intercepted, a switch made
   to protected mode so that PDOS can handle it.  PDOS may or may not 
   return to real mode to execute the original BIOS interrupt.   
   
4. 32-bit program under 32-bit PDOS.
   It calls the protected mode interrupt.  PDOS may or may not return
   to real mode to get the 16-bit version called, but IF it does, it
   will translate all the 32-bit addresses, buffering them if required.


THE OPERATING SYSTEM ITSELF
---------------------------

To remain compatible with MSDOS 5's FORMAT, SYS + boot sector, the
operating system has been split up into the traditional 3 programs,
IO.SYS, MSDOS.SYS and COMMAND.COM, except they have been renamed to
PLOAD.COM, PDOS.EXE and PCOMM.EXE respectively.

As far as possible, all code has been written in C, for portability
between 16 + 32 bit systems, plus portability to other CPUs.  It is
even possible to compile the 16-bit version as an executable which
can then be debugged with Turbo Debugger etc.  The goal has been
that as far as possible, all you need to do is recompile the code on
another machine and it instantly works.

PLOAD.COM is a COM program, that has startup code which assumes
the first 3 sectors are loaded.  It then completely reloads PLOAD.COM 
and PDOS.EXE, using the FAT-reading logic.  It also has the logic to do
program relocation for use by PDOS.EXE.  Also in the 32 bit version
it has the switch to/from protected mode facility.

PDOS.EXE is a large model EXE program (16-bit) or an a.out program
in 32-bit.  It uses special startup code to ensure that it doesn't
attempt to access the non-existant PSP etc.

So far I have written cut-down versions of all three executables, so that
it boots (of floppy or hard disk, including logical partitions) and prints 
"Welcome to pcomm" or similar, and then prompts for a command.  Recognized 
commands are:

"type", which will type a file, e.g. "type config.sys".

"cd", which will change directories.

"dir", which will give a directory listing.

"exit", which does nothing if this is your primary shell

"reboot", which reboots the computer

You can also launch other programs, so long as it is a .exe in the
current directory.  Because you can't write files (yet) using PDOS, 
you can't run that sort of program.  Also the program can't be more 
than 60k in size for the 16-bit version.  Also the program can't do 
any as-yet unimplemented interrupts.  All of this means that it is not 
useful as a general purpose operating system as yet.  The file-write 
facility is likely to be the last thing implemented, in order to give 
PDOS a decent test before opening up the mechanism for disk trashing!

PDOS currently (even in the 32-bit version) uses the BIOS to read from
disk and do screen I/O and keyboard input.  This should hopefully make
it compatible with your existing hardware straight out of the box.
However, it is intended that as a non-default CONFIG.SYS option, you
can get PDOS to use internal device drivers.


FILES
-----

pload.c - loader for PDOS (IO.SYS)
pdos.c - PDOS, the operating system (MSDOS.SYS)
pcomm.c - command processor (COMMAND.COM).
comp0.bat - used to compile PATCHVER.EXE (used by doinst)
comp1.bat - used to compile PLOAD.COM (IO.SYS)
comp2.bat - used to compile PDOS.EXE (MSDOS.SYS)
comp3.bat - used to compile PCOMM.EXE (COMMAND.COM)
comp4.bat - used to compile 32-bit-PDOS-supporting PLOAD.COM (IO.SYS)
comp5.bat - used to compile 32-bit version of PDOS.EXE (MSDOS.SYS)
comp6.bat - used to compile 32-bit version of PCOMM.EXE (COMMAND.COM)
compb.bat - used to compile boot sector
compbu.bat - used to compile program to update boot sector
compexe.bat - compiles PDOS + PCOMM into a single executable for debugging
doinst.bat - used to rename the 3 executables to the MSDOS-style names so
             that the "sys" command can then be used to install them.
bos.* - bios functions
pos.* - dos functions
fat.* - fat access functions
pdpgoal.txt - not the license agreement


INTERNALS
---------

memory map:
4 gig
memory for protected mode applications (code & data)
200000
  unused for 32-bit, presumably will hold 16-bit device drivers

110000
  area of memory still addressable by 16-bit programs,
  giving an extra 64k for any application that knows
  how to do that.  Unused by PDOS itself.
100000
  used by i/o devices and bios on IBM PC, 640k limit
  being created by the a0000 starting address.
a0000
  space for application code

40000
  64k completely wasted for memory chain control
  blocks, to avoid having to drop the flat memory
  model coding in the memory management class. (16-bit only)

30000
  unused I suppose (16-bit only)
  
30700 (approx)
  MSDOS.SYS data & stack (32-bit only)
  
20700
  MSDOS.SYS
10700
  MSDOS.SYS psp
10600
  IO.SYS stack
07DFF
  sector 0 & bpb
07C00
07BFF
  IO.SYS
007FF
  MBR + IO.SYS
00700
  MBR
00600
00400
  REAL MODE INTERRUPTS
00000

The BIOS loads sector 0 into locations 7C00 - 7DFF.
[if a hard disk, then it is actually the MBR that gets loaded
 first, relocates itself to 0600-07FF, then loads sector 0 of
 the active partition]
IO.SYS gets loaded between 0700 and 7BFF (58 sectors).
The stack is at 10600 and grows down.
MSDOS.SYS gets loaded between 20000 and 2FFFF.
On transition to protected mode, sp gets set to 2FFFC, which is
the equivalent of 1FFFC except that it is relative to the start
of the program, so is more like 3FFFC absolute.  When calling
other programs, the stack is changed and set to 32k in size.  When
an interrupt is done, the previous stack is restored temporarily.
It is also restored when the program terminates.  When doing a real
mode interrupt, ss & sp are temporarily set to what PLOAD was using.
The interrupt vectors for protected mode are provided as part of the
PLOAD executable - a length of 800 to give 256 8-byte interrupts.
For BIOS calls that require pointers, e.g. reading from disk sectors,
a translation is done so that the data is read to a location provided
by PLOAD and then transferred up.
The memory used by applications under protected mode starts at 200000.

Protected mode switching is done as follows:
  runaout() runs an a.out executable, by doing the following:
    load file
    runprot()
  runprot() runs a protected mode program by doing the following:
    disabling interrupts
    rawprot()
    establishing protected-mode interrupts
    reenabling interrupts
  rawprot() does the actual protected mode switch and starts
    executing code, blissfully unaware of what it is. 

Note that that interface is designed to look clean, but in the case
of runprot(), the code's function is actually split between the
calling real-mode program and the protected-mode program.  Earlier
on runaout was too, the relocation of the a.out executable being
done after the switch to protected mode and in the a.out code!

You would use rawprot() if you wanted to quickly execute some
protected mode code, say already in memory, and were happy for
interrupts to be disabled whilst it was being executed.

runprot() will establish interrupt handlers for long-running bits
of code.

runaout() is just one executable format that may be run.

The first bit of the pload executable must fit into 3 * 512 bytes,
as that is all the boot sector loads.  When this was last measured,
1998-08-30, there were 465 bytes spare.  3*512 bytes corresponds to
a map reading of 0700h because of the org 0100h required for COM
files.


FUTURE DEVELOPMENT
------------------

New philosophy for PDOS:
Should be a poor man's Win 98 console mode.
Imagine someone has Windows/XP installed, but
something has gone wrong, Windows has stopped booting,
and thus they need a boot floppy to go and inspect the
hard disk, and copy some important files to a floppy.

Or maybe something has gone even more wrong than that,
and they need to write a small C program to repair some
damage and hopefully resurrect their system.  Because of
space limitations, assume they have obtained PDOS, loaded
with 3rd party utilities, on a bootable CD instead.

Maybe they've even got a second hard disk, so after
booting off the CD, they go "sys d:" to install PDOS
onto that, and then change their BIOS to boot off
that disk, and hey presto, plenty of space and a
writable disk, great for recovery work.

So something that looks identical to DOS, but is in
fact 32-bit, and can handle Win-98 32-bit console
mode executables.

And 3rd party software, such as gcc, can be
optionally distributed on the disk, but if someone
needs something comprised of purely public domain
source code, that is available too, they can put
their own (proprietary) disk-repair utility on to
a PDOS disk.

So note some important things.
1. VM not required.
2. Multitasking not required.
3. Ability to support FAT-32 is required.
4. Ability to write to disks is required.
5. Ability to handle Windows 32-bit executables is required.
6. Some utilities, such as "sys" are required.
7. Ability to handle long file names is required.
8. PDPCLIB needs to be ported to do Windows 32-bit.
9. An editor is required.
10. Something as simple as DOS, not as complicated as Unix, is required.
11. PDPZM should be ported to upload/download data.
12. All disk access via BIOS is fine and preferred.
13. Speed is not an issue.
14. The basics in place provide an opportunity for self-improvement.

The environment as described above can also be used
to develop Windows 32-bit console mode programs.


Old stuff...

Check to see if pdos (16 bit) static data is being initialized to 0
Find out why floppy disks aren't working in lba mode
See if freedos can be used to install.

Ability to write files
see how close gcc is to running
68000 port
support posix.1 programming interface
incorporate all source code I have into one huge pcomm.exe
look at minimizing code that is authorized to hang system.


OBTAINING
---------
The latest version currently resides at
http://sourceforge.net/projects/pdos


CONTACTING THE AUTHOR
---------------------

If you have any enhancements to PDOS, please send 
the code changes to the email address listed above.
Please ensure that your code changes are public 
domain, if you create a new source file, include 
an explicit PD notice.  It is my intention to build 
up a completely public domain system for use by EVERYONE,
including commercial vendors of operating systems.
