@echo off
wasmr -zq -zcm -Dmemodel=tiny pbootsec.asm
tlink -t -x pbootsec,pbootsec.com,,,
