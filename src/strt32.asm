; This program written by Paul Edwards
; released to the public domain
; write a 'Z' to first character on the screen then return

;% .model large,c
  .386p

_DATA   segment word public 'DATA'
_DATA   ends
_BSS    segment word public 'BSS'
_BSS    ends
_STACK  segment word stack 'STACK'
        db 1000h dup(?)
_STACK  ends

DGROUP  group   _DATA,_BSS

extrn pdos32:proc

_TEXT32 segment word use32 public 'CODE'
assume cs:_TEXT32, ds:DGROUP
start:
        mov edi,0b8000h
        mov byte ptr [edi],'Z'
        call pdos32
        ret
_TEXT32 ends

end start
