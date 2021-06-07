nasm mbr.asm
sectread 81 0 oldmbr.dat 
xychop oldmbr.dat oldmbr2.dat 440 511
copy /b mbr+oldmbr2.dat newmbr.dat
sectwrit 81 0 newmbr.dat
