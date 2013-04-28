; pbootsec.asm - pdos boot sector
;
; This program written by Paul Edwards
; Released to the public domain

; This code will be loaded to location 07C00h by the BIOS
; We set the stack at 07C00h and make it grow down

; Registers at entry are:
; CS:IP = 07c00H, probably 0000:7c00
; DL = drive number
; nothing else guaranteed

; We should store the drive number in BPB[25], since
; pload is expecting it there.  pload also expects
; that the CS:IP will have an IP of 0 when it is called.
; It also expects some sort of stack to be set up.

% .model memodel

_DATA   segment word public 'DATA'
_DATA   ends
_BSS segment word public 'BSS'
_BSS ends

_TEXT segment word public 'CODE'

org 07c00h
top:

public start
start proc

;Jump over our BPB
 jmp bypass
 nop

; HD:
;
; 000000  EB479050 444F5320 782E78
;         00 02  bytes per sector
;           04   sectors per cluster
;          0100  # reserved
;             .G.PDOS x.x.....
; 000010  02           # fats
;     0002          # directory entries (512)
;      01 EC        total logical sectors
;     F8      media byte
;       3B00     sectors per FAT
;
; 3b=59*2 + 32 (directory sectors) + 1 (reserved) = 151
;
;         3F00      sectors per track (63)
;          1000     number of heads (16)
;         3F00   hidden (63)
; 151 + hidden=63 = 214
; 214-3*63 = 25 + 1 (1-based) = 26
;          0000  ......;.?...?...
; 000020  00000000 800029EC 1C591E4E 4F204E41  ......)..Y.NO NA
; 000030  4D452020 20204641 54313620 202031C0  ME FAT16   1.
; 000040  FA8ED0BC 007CFB0E 1FB80000 8ED0BC00  .....|..........
; 000050  7C0E1FE8 5D00B870 0050B800 0050CB06  |...]..p.P...P..
; 000060  53505152 1EB80000 8EC0BB78 00268B47  SPQR.......x.&.G
; 000070  028ED826 8B078BD0 B800008E C0BB3E7C  ...&..........>|
; 000080  B9000083 F90B7D0E 538BDA8A 075B2688  ......}.S....[&.
; 000090  07424341 EBEDB800 008EC0BB 780026C7  .BCA........x.&.
;
; floppy:
;
; 000000  EB3C904C 494E5558 342E31
;         00 02  bytes per sector
;           01   sectors per cluster
;          0100  # reserved
;             .<.LINUX4.1.....
; 000010  02           # fats
;     E000          # directory entries (224)
;      40 0B        total logical sectors
;     F0      media byte
;       0900     sectors per FAT
;
; 9*2 + 224*32/512 + 1 = 33
;         1200      sectors per track (18)
;          0200     number of heads (2)
;         0000   hidden
; 33 - 18 = 15 + 1 (1-based) = 16
;          0000  ...@............
; 000020  00000000 000029E3 7FFA4420 20202020  ......)...D
; 000030  20202020 20204641 54313220 2020FAFC  FAT12   ..
; 000040  31C08ED8 BD007CB8 E01F8EC0 89EE89EF  1.....|.........
; 000050  B90001F3 A5EA5E7C E01F0000 60008ED8  ......^|....`...
; 000060  8ED08D66 A0FB807E 24FF7503 885624C7  ...f...~$.u..V$.
; 000070  46C01000 C746C201 00E8E900 46726565  F....F......Free
; 000080  444F5300 8B761C8B 7E1E0376 0E83D700  DOS..v..~..v....
; 000090  8976D289 7ED48A46 1098F766 1601C611  .v..~..F...f....


;bpb db 51 dup(?)

;Lets make an actual BPB
OEMName           db 'PDOS x.x'
BytesPerSector    dw 512   ;512 bytes is normal ;)
SectorsPerClustor db 1     ;Sector == cluster
ReservedSectors   dw 1     ;Reserve the boot sector
FatCount          db 2     ;2 copies of the fat is standard
RootEntries       dw 224   ;# of root entries
TotalSectors16    dw 0     ;# of sectors on the disk
MediaType         db 0f8h  ;Ignored for most things
FatSize16         dw 0     ;Size of a single fat in sectors
SectorsPerTrack   dw 0     ;Sectors Per Track
Heads             dw 0     ;Head count
HiddenSectors     dd 0     ;# of hidden sectors
TotalSectors32    dd 0     ;# of sectors, 32-bit!

;Start of our Fat12/16 info
DriveNum    db  0     ;Drive number (0x0x - floppy, 0x8x - hd
Reserved    db  0     ;Reserved, set to 0
BootSig     db  0x29    ;Set to 0x29 if next 3 values are present
VolumeID    dd  0     ;Volume's ID, we don't care for it ;)
VolumeLabel db  'PDOS Volume'  ;11 bytes
FileSystem  db  'FAT  '   ;File system (FAT12 or FAT16)


BootDisk db 0
; new disk parameter table
;dpt db 11 dup(?)


bypass:
;Always clear direction bit first, just incase bios leaves it unset
 cld
 xor  ax, ax   ;Zeroize ax
 mov  ss, ax
 mov  sp, 07c00h
 mov  ds, ax   ;Set data segment to 0x000
 mov  es, ax

 mov  [BootDisk], dl   ;Store our boot disk

 call CalculateLocation   ;Gets our data sector into ax
 mov  cx, 3         ;Load 3 sectors
 mov  bx, 0700h     ;Loaded to es:bx (0x00:0x0700)
 call ReadSectors   ;Read the actual sectors

 mov  ax, 0070h
 push ax
 mov  ax, 0
 push ax
 retf
start endp

;Calculates the location of our file to load (after Fat, after hidden 
;sectors, etc)
;Inputs:
;   (None)
;Outputs:
;   ax - sector to load
CalculateLocation proc
;In order to calculate our data start:
;RootStart = ReservedSectors + FatCount * FatSectors
;DataStart = RootStart + RootEntryCount * 32 / BytesPerSector

 ;Store bx, cx, and dx for later
 push bx
 push cx
 push dx

 ;Use bx to hold our value, as ax is needed by mul/div!
 mov  bx, [ReservedSectors]
 mov  ax, [FatSize16]
 xor  dx, dx      ;Must zeroize dx before a multiply so our carry flag 
                  ;doesn't get set
 mul  byte ptr [FatCount]
 add  bx, ax

 mov  ax, [RootEntries]
 mov  cl, 5
 shl  ax, cl      ;Same as * 32
 xor  dx, dx      ;Must zeroize dx before a divide
 div  word ptr [BytesPerSector]   ;Divide by # of bytes per sector

 ;Add previous calculation stored in bx into ax to give it our final location!
 add  ax, bx

 ;Restore dx, and cx, bx
 pop  dx
 pop  cx
 pop  bx
 ret
CalculateLocation endp

;Convert LBA -> CHS
;Inputs:
; ax - sector
;Outputs:
;   Standard CHS format for use by int 13h
Lba2Chs proc
 xor  dx, dx      ;Must zeroize dx before a divide
 div  word ptr [SectorsPerTrack]
 mov  cl,  dl     ;Sector #
 inc  cl
 xor  dx, dx       ;Zero out dx again
 div  word ptr [Heads]
 mov  dh,  dl     ;Mov dl into dh (dh=head)
 mov  ch,  al     ;Mov cylinder into ch
 push cx
 mov  cl, 6
 shl  ah,  cl
 pop  cx
 or  cl,  ah
 ret
Lba2Chs endp

;Used to reset our drive before we use it
;Inputs:
;   (None)
;Outputs:
;   (None)
ResetDrive proc
 push ax
 push dx
 RetryReset:
  mov  dl, [BootDisk]
  mov  ax,  0
  int  013h
  jc  RetryReset    ;Didn't reset, lets try again
 pop  dx
 pop  ax
 ret
ResetDrive endp

;Read a single sector from a floppy disk
;Inputs:
;   ax - sector to read
;   es:bx - dest
;Outputs:
;   (None)
ReadSingleSector proc
 push ax
 push bx
 push cx
 push dx
 push es
 call Lba2Chs     ;Grab our CHS
 RetryRead:
  call ResetDrive   ;Get drive ready..
  mov  dl, [BootDisk]  ;Grab our boot disk
  mov  ax, 0x0201   ;Read function, one sector
  int  13h
  jc   RetryRead
 push es
 push dx
 push cx
 push bx
 push ax
 ret
ReadSingleSector endp

;Read multiple sectors
;Inputs:
;   ax - sector to read
;   es:bx - dest
;   cx - # of sectors
;Outputs:
;   (None)
ReadSectors proc
 push ax
 push bx
 ReadNextSector:
  call ReadSingleSector
  add  bx,  [BytesPerSector] ;Next sector
  inc  ax                    ;Next LBA
  loop ReadNextSector        ;Until cx = 0
 pop  bx
 pop  ax
 ret
ReadSectors endp

org 02feh
lastword dw 0aa55h

_TEXT ends

end top

