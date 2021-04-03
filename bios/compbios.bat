rem pass -DTARGET_64BIT for 64-bit platforms
gcc %1 -o bios.exe bios.c exeload.c
