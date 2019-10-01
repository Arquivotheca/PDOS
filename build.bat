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
copy %loc%\pload.com %loc%\io.sys
copy %loc%\pdos.exe %loc%\msdos.sys
copy %loc%\pcomm.exe %loc%\command.com
md 16bit
copy pload.com 16bit
copy pdos.exe 16bit
copy pcomm.exe 16bit
copy ..\pdpclib\pdptest.exe 16bit
patchver %loc%\io.sys
sys %loc% %drive%:
copy ..\pdpclib\pdptest.exe %drive%:
echo pdos16.img | raread -n -d A

cd ..\pdpclib
del *.o
call compp
cd ..\src
call comp4
call comp5
call comp6
format %drive%: /u
copy %loc%\pload.com %loc%\io.sys
copy %loc%\pdos.exe %loc%\msdos.sys
copy %loc%\pcomm.exe %loc%\command.com
md 32bit
copy pload.com 32bit
copy pdos.exe 32bit
copy pcomm.exe 32bit
copy ..\pdpclib\pdptest.exe 32bit
sys %loc% %drive%:
copy ..\pdpclib\pdptest.exe %drive%:
unzip -d %drive%: \samp32\uemvs1.zip
unzip -d %drive%: \samp32\pdptest.zip
unzip -d %drive%: \samp32\dll.zip
echo pdos32.img | raread -n -d A

call compb
call compbu
bootupd pdos16.img pbootsec.com
bootupd pdos32.img pbootsec.com

rem rawrite -f pdos16.img -d a -n
rem rawrite -f pdos32.img -d a -n

cd ..
