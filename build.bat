rem assuming you have a formatted floppy in drive mentioned here,
rem and that your source is in the directory listed here

rem then this batch file will produce the 16-bit and 32-bit
rem boot images that comprise the executable distribution of pdos

set drive=a
set loc=c:\devel\pdos\src

cd pdpclib
del *.obj
call compile
cd ..\src
call comp0
call comp1
call comp2
call comp3
call compw16
copy %loc%\pload.com %loc%\io.sys
copy %loc%\pdos.exe %loc%\msdos.sys
copy %loc%\pcomm.exe %loc%\command.com
md 16bit
copy pload.com 16bit
copy pdos.exe 16bit
copy pcomm.exe 16bit
copy world.exe 16bit\world16.exe
patchver %loc%\io.sys
sys %loc% %drive%:
copy world.exe %drive%:world16.exe
echo pdos16.dsk | raread -n -d A

cd ..\pdpclib
del *.o
call compp
cd ..\src
call comp4
call comp5
call comp6
call compw32
del %drive%:world16.exe
copy %loc%\pload.com %loc%\io.sys
copy %loc%\pdos.exe %loc%\msdos.sys
copy %loc%\pcomm.exe %loc%\command.com
md 32bit
copy pload.com 32bit
copy pdos.exe 32bit
copy pcomm.exe 32bit
copy world.exe 32bit\world32.exe
sys %loc% %drive%:
copy world.exe %drive%:world32.exe
echo pdos32.dsk | raread -n -d A

call compb
call compbu
bootupd pdos16.dsk pbootsec.com
bootupd pdos32.dsk pbootsec.com

rem rawrite -f pdos16.dsk -d a -n
rem rawrite -f pdos32.dsk -d a -n

cd ..
