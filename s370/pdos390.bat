@echo off

rem Use this for manual startup


pushd %PDOS390%
if exist one.at.a.time goto chk1
copy nul one.at.a.time
popd
goto chk2
:chk1
popd
goto exit
:chk2


SET HERCULES_RC=%PDOS390%/auto_ipl.rc

rem Use this for automatic startup
rem SET HERCULES_RC=%PDOS390%/auto_ipl.rc

if %1.==. goto bypass
SET HERCULES_RC=%PDOS390%/%1.rc

:bypass

SET HERCULES_CNF=%PDOS390%/pdos.conf

pushd %PDOS390%
hercules >hercules.log

del one.at.a.time
popd

:exit
