tasm -zi -ml -Dmemodel=large support.asm
tasm -zi -ml -Dmemodel=large protinta.asm
tasm -zi -ml -Dmemodel=large lldos.asm

bcc -v -ml -c prottest.c protint.c
bcc -v -l3 -M -ml prottest.obj protint.obj protinta.obj support.obj lldos.obj
