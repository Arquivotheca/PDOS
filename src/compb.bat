@echo off
tasm -zi -mx -Dmemodel=tiny pbootsec.asm
tlink -t -x pbootsec,pbootsec.com,,,
