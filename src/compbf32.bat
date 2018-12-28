@echo off
wasmr -zq -zcm -Dmemodel=tiny fat32.asm
tlink -t -x fat32,fat32.com,,,
