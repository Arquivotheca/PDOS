@echo off
wasmr -zq -zcm -Dmemodel=tiny pbootsec.asm
rem tasm requires 2 passes to generate the desired near jump
rem for first instruction
rem tasm -m2 -Dmemodel=tiny pbootsec.asm
tlink -t -x pbootsec,pbootsec.com,,,
