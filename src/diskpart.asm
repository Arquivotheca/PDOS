; DISKPART.ASM

; This is a disassembly of the disk partition table of an MSDOS 5.0 
; partitioned hard disk.  This is located at track 0, head 0, sector 1
; which is the first sector loaded by the BIOS, to location 7c00.

; The comments were added by Paul Edwards.  There is no sign of a copyright 
; on this program in the partition table, and anyhow disks are bought 
; pre-partitioned, and even if they weren't, I don't think the output from 
; the "fdisk" command is copyrightable.  Also, I've never heard of anyone who
; bought a blank hard disk being charged with pirating software.

; The comments are all mine, and I am releasing them to the public domain.  
; Contact your lawyer before trying to use the code though!

        cli                ; disable interrupts 
        xor    ax,ax       ; set ax = 0
        mov    ss,ax       ; set ss = 0
        mov    sp,7C00     ; set sp = 7c00
        mov    si,sp       ; set si = 7c00
        push   ax          ;
        pop    es          ; set es = 0
        push   ax          ;
        pop    ds          ; set ds = 0
        sti                ; enable interrupts
        cld                ; clear direction flag
        mov    di,0600     ; set di = 0600
        mov    cx,0100     ; set cx = 0100
        repnz movsw        ; mov this 512 byte sector from 7c00 to 0600
        jmp    0000:061D   ; jmp continue, in new location
continue:        
        mov    si,07BE     ; set si to 07be (disk parameter table)
        mov    bl,04       ; set bl to 4
nextentry:        
        cmp    byte ptr [si], 80H ; if *si == 80 (got valid entry)
        je     validate    ; then go to partition validation routine
        cmp    byte ptr [si], 00H ; if *si != 0
        jne    invalid_part ; then go to invalid partition routine
        add    si,0010     ; otherwise go to next entry
        dec    bl          ; count down, max of 4 entries
        jne    nextentry   ; if not end of table, read next
        int    18          ; jump to rom basic (ie failure)
        
; now that we've found a bootable partion (80) make sure that all
; other partitions are set to 00 (not bootable)        
validate:
        mov    dx,[si]     ; get first word of DPT in dx
        mov    cx,[si+02]  ; get second word in cx
        mov    bp,si       ; save si in bp
nextzero:        
        add    si,0010     ; go to next DPT entry
        dec    bl          ; count down
        je     process     ; if reached end, continue processing, all OK
        cmp    byte ptr [si], 00H ; keep reading entries while they
        je     nextzero    ; are zero
;                            otherwise fall through to invalid partition rtn
invalid_part:
        mov    si,invp_msg ; "invalid partition table" message
nextch:        
        lodsb              ; check that we're not
        cmp    al,00       ; at the end of the string
        je     infloop     ; if we are then go into infinite loop
        push   si          ; push location of message string
        mov    bx,0007     ; set bx to 7 (display page + colour)
        mov    ah,0E       ; set ah to e (subfunction write text)
        int    10          ; write text function
        pop    si          ; restore si
        jmp    nextch      ; print next character
infloop:        
        jmp    infloop     ; infinite loop
        
; We have a good partition, bp points to the entry,
; dx and cx contain first two words
process:
        mov    di,0005     ; set di to 5 (number of read attempts)
readagain:        
        mov    bx,7C00     ; set bx to 7c00
        mov    ax,0201     ; set ax to 0201
        push   di          ; save di
        int    13          ; read 1 disk sector into 7c00, using
                           ; track, sector, head + drive info from DPT
        pop    di          ; restore di
        jnb    readok      ; if read was OK, continue
        xor    ax,ax       ; set ax = 0
        int    13          ; reset the disk
        dec    di          ; decrease count
        jne    readagain   ; redo the read
        mov    si,loaderr_msg ; set "Error Loading OS" message
        jmp    nextch      ; print out message
readok:        
        mov    si,missing_msg ; point to "Missing OS" message
        mov    di,7DFE     ; set di to 7DFE
        cmp    word ptr [di], 0AA55H ; see if last word is 0AA55
        jne    nextch      ; if it isn't, print out "missing OS" message
        mov    si,bp       ; set si to point back to DPT entry
        jmp    0000:7C00   ; jump to OS boot sector
        
; so at time the OS boot sector is called, si is pointing to DPT,
; and cx and dx contain values suitable for reading from int 13        

inp_msg  db "Invalid partion table", 0
loaderr_msg  db "Error Loading operating system", 0
missing_msg  db "Missing operating system", 0

; followed by DPT entries (4)
