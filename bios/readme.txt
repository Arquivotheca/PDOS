The entire contents of this package are public domain.

bios.exe is a BIOS that exports various functions in the
C library, starting with a bodged printf for now. There
is an AmigaOS version (currently dependent on having
a 68020 or above unless you rebuild using your own
68000 C compiler). Use makefile.ami to build. Use
compbios.bat to build a Windows version. Use allmvs.bat
to build an MVS version.

osworld.exe is a 68000 (not 68020) program that depends
on a suitable BIOS such as the above. Although it is in
AmigaOS hunk format, it will not be possible to run it
from AmigaOS but maybe a future version of AmigaPDOS
would support this interface (or something similar) for
application programs, but it would probably need a different
identifier, probably 00000F3F instead of 000003F3. It
especially makes a difference whether
AmigaPDOS is being built to run AmigaOS executables or
whether AmigaPDOS has been built to run its own standard
for executables, which would require a different format
so that it can identify the processor, the size of char,
short etc, so that it can adjust program arguments
appropriately and switch to a different coprocessor if
required. I used compami.bat to build. You can also
build an 80386 a.out executable with comppdos.bat and
the MVS version is built as part of the above.

Run by going:

bios.exe osworld.exe

on any AmigaOS system, with a 68020 or above unless you
have compiled your own bios.exe. The reason it has been
built using 68020 instructions is because I don't yet
have public domain 68000 assembler code (or even C code)
to do 32-bit division as needed by the compiler I am
currently using (vbcc). Same deal for running on
Windows, and MVS is done as part of the above.

See exeloadLoadAmiga() in exeload.c for copious documentation
on how everything should work.
