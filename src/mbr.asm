; Public domain MBR by Mark Raymond

; This is a standard MBR, which loads the VBR of the
; active partition to 0x7c00 and jumps to it.

; ## Preconditions

; * MBR is loaded to the physical address 0x7c00
; * dl contains the disk ID the system was booted from

; ## Postconditions

; * If loading was successful
;   - VBR is loaded to the physical addres 0x7c00
;   - dl contains the disk ID the system was booted from
;   - ds:si and ds:bp point to the partition entry the VBR was loaded from

; * If loading was unsuccessful
;   - Error message is displayed on screen
;   - System hangs

; ## Errors

; This MBR will output an error message and hang if:

; * There are no active partitions
; * The active partition has a partition type of 0
; * The BIOS does not support LBA extensions
; * The disk cannot be read
; * The VBR does not end with the boot signature (0xaa55)


[BITS 16]
[ORG 0x0600]

; Clear interrupts during initialization
cli

; Initialize segment registers and stack
xor ax,ax
mov ds,ax
mov es,ax
mov ss,ax
mov sp,0x7c00

; Allow interrupts
; See http://forum.osdev.org/viewtopic.php?p=236455#p236455 for more information
sti

; Relocate the MBR
mov si,0x7c00       ; Set source and destination
mov di,0x0600
mov cx,0x0100       ; 0x100 words = 512 bytes
rep movsw           ; Copy mbr to 0x0600
jmp 0:relocated     ; Far jump to copied MBR

relocated:

; Search partitions for one with active bit set
mov si,partition_table
mov cx,4
test_active:
test byte[si+partition.status],0x80
jnz found_active
add si,entry_length
loop test_active
; If we get here, no active partition was found,
; so output and error message and hang
mov bp,no_active_partitions
jmp fatal_error

; Found a partition with active bit set
found_active:
cmp byte[si+partition.type],0; check partition type, should be non-zero
mov bp,active_partition_invalid
jz fatal_error

; Check BIOS LBA extensions exist
mov ah,0x41
mov bx,0x55aa
int 0x13
mov bp,no_lba_extensions
jc fatal_error
cmp bx,0xaa55
jnz fatal_error

; Load volume boot record
mov eax,[si+partition.start_lba]  ; put sector number into LBA packet
mov [lba_packet.lba],eax
mov cx,3        ; three tries
push si         ; save pointer to partition info
try_read:
mov ah,0x42
mov si,lba_packet
int 0x13        ; BIOS LBA read (dl already set to disk number)
jnc read_done
mov ah,0
int 0x13        ; reset disk system
loop try_read
mov bp,read_failure
jmp fatal_error
read_done:
pop si          ; restore pointer to partition info

; Check the volume boot record is bootable
cmp word[0x7dfe],0xaa55
mov bp,invalid_vbr
jnz fatal_error

; Jump to the volume boot record
mov bp,si           ; ds:bp is sometimes used by Windows instead of ds:si
jmp 0x0000:0x7c00   ; if boot signature passes, we can jump,
                    ; as ds:si and dl are already set

output_loop:
int 0x10        ; output
inc bp
fatal_error:
mov ah,0x0e     ; BIOS teletype
mov al,[bp]     ; get next char
cmp al,0        ; check for end of string
jnz output_loop
hang:
; Bochs magic breakpoint, for unit testing purposes.
; It can safely be left in release, as it is a no-op.
xchg bx,bx
sti
hlt
jmp hang

; Error messages
no_active_partitions:     db "No active partition found!",0
active_partition_invalid: db "Active partition has invalid partition type!",0
no_lba_extensions:        db "BIOS does not support LBA extensions!",0
read_failure:             db "Failed to read volume boot record!",0
invalid_vbr:              db "Volume boot record is not bootable (missing 0xaa55 boot signature)!",0

; LBA packet for BIOS disk read
align 8,db 0
lba_packet:
.size       db 0x10
.reserved   db 0
.sectors    dw 0x0001
.offset     dw 0x7c00
.segment    dw 0x0000
.lba        dd 0
.lbapadding dd 0

; Pad to the end of the code section
times 440-($-$$) db 0

absolute $
sig:         resb 4
padding:     resb 2
partition_table:

struc partition
.start:
.status:     resb 1
.start_chs:  resb 3
.type:       resb 1
.end_chs:    resb 3
.start_lba:  resd 1
.length_lba: resd 1
.end:
endstruc

entry_length equ partition.end - partition.start
