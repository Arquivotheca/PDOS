set loc=c:\devel\pdos\src
set drive=d

copy %loc%\pload.com %loc%\io.sys
copy %loc%\pdos.exe %loc%\msdos.sys
copy %loc%\pcomm.exe %loc%\command.com
patchver %loc%\io.sys
sys %loc% %drive%:
