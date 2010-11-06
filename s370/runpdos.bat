@echo off
rem You can copy this to your path if you want
rem and run PDOS from batch jobs
rem You can also simply add the %PDOS% directory
rem to your path

set rcfile=auto_run

if %1. == . goto usage
if not exist %1 goto usage

if %2. == . goto usage

pushd %PDOS%
if exist one.at.a.time goto chk1
copy nul one.at.a.time
popd
goto chk2
:chk1
popd
goto exit
:chk2

set in=none
if %3. == . goto noin
if %3. == none. goto noin
set in=%3
:noin

set out=none
if %4. == . goto noout
if %4. == none. goto noout
set out=%4
:noout

set tranin=binary
set tranintape=pctomf_bin
if %5. == . goto fintranin
if %5. == binary. goto fintranin
if %5. == rdwund. goto rdwuin
if %5. == rdwvar. goto rdwvin
if %5. == vart. goto vartin
if not %5. == ascii. goto usage
set tranin=ascii
set tranintape=pctomf_text
goto fintranin
:rdwuin
set tranintape=pctomf_rdwund
goto fintranin
:rdwvin
set tranintape=pctomf_rdwvar
goto fintranin
:vartin
set tranintape=pctomf_vart
:fintranin

set tranout=binary
set tranouto=-b
if %6. == . goto fintranout
if %6. == binary. goto fintranout
if not %6. == ascii. goto usage
set tranout=%6
set tranouto=-t
:fintranout

set aws=none
if %7. == . goto noaws
if %7. == none. goto noaws
set aws=%7
:noaws

rem copy input data file (typically .zip)
del %PDOS%\hercauto.zip
rem create dummy file
echo dummy >%PDOS%\hercauto.zip
copy %PDOS%\%tranintape%.tdf %PDOS%\pctomf.tdf
if %in% == none goto noin2
copy %in% %PDOS%\hercauto.zip
:noin2

rem initialize output tape (typically to receive .zip)
rem actually, might use dasdseq here
hetinit %PDOS%\mftopc.het MFTOPC OWNER

rem initialize AWS tape (typically only used for load/unload)
hetinit %PDOS%\awstap.aws AWSTAP OWNER
if %aws% == none goto noaws2
copy %aws% %PDOS%\awstap.aws
:noaws2

rem batch file
copy %1 %PDOS%\hercauto.bat

rem now run everything
pushd %PDOS%




rem note that we need to rebuild the PDOS disk every run
rem because we don't yet have ftp/tape/card reader
rem operational. We want a clean disk for a controlled
rem environment anyway

del pdos00.cckd
dasdload -bz2 ctl.txt pdos00.cckd






SET HERCULES_RC=%PDOS%/%rcfile%.rc
SET HERCULES_CNF=%PDOS%/pdos.cnf

rem If you want to run Hercules as a batch tool and
rem not see any execution, you just need to redirect
rem both stdout and stderr

rem hercules 2>&1 | tee hercules.log
hercules >hercules.log

del hercauto.zip
popd

rem copy printer output
copy %PDOS%\hercules.log %2

rem extract output data file (typically .zip if binary)
if %out% == none goto noout2
hetget %tranouto% %PDOS%\mftopc.het %out% 1
:noout2

rem copy AWS tape in case it was changed
if %aws% == none goto noaws3
copy %PDOS%\awstap.aws %aws%
:noaws3

pushd %PDOS%
del one.at.a.time
popd

GOTO EXIT
:USAGE
echo Usage runpdos batch.file print.file [aux_input.file^|none] [aux_output.file^|none] [ascii^|binary^|rdwund^|rdwvar^|vart] [ascii^|binary] [awstap.file^|none]
echo e.g. runpdos compile.bat output.txt world.c world.s ascii ascii
echo or runpdos buildgcc.bat output.txt source.zip exe.zip
echo default is for files (if provided) to be binary

:EXIT
