; BOOTSEC.ASM

; This is a disassembly of the boot sector of an MSDOS 5.0 formatted disk
; (floppy and hard disk are identical), with comments added by Paul Edwards.
; There is no sign of a copyright on this program on the boot sector, and 
; anyhow disks are bought pre-formatted, and even if they weren't, I don't 
; think the output from the "format" command is copyrightable.  Also, I've 
; never heard of anyone who was given a formatted blank disk charged with 
; pirating software.

; The comments are all mine, and I am releasing them to the public domain.  
; Contact your lawyer before trying to use the code though!

        jmp    bypass      ; bypass the bpb
bpb db 03ch dup(?)

bypass:  
        cli                ; disable interrupts
        xor ax,ax          ; set ax = 0
        mov ss,ax          ; set ss = 0
        
; The stack is set to 07C00H, which is where this program was loaded
; The stack goes down so will not overwrite anything

        mov sp,7C00        ; set sp = 7C00
        
        push ss            ; ...
        pop es             ; set es = 0          
        
; Now at location x'00078' is the vector for interrupt 1E.  This is
; not a real interrupt however, and contains instead a pointer to a
; disk parameter table.  I think the format of the table is:
; 00 - first specify byte
; 01 - second specify byte
; 02 - timer ticks to wait before turning off drive motor
; 03 - number of bytes per sector
;      (00 = 128, 01 = 256, 02 = 512, 03 = 1024)
; 04 - sectors per track
; 05 - gap length
; 06 - data length
; 07 - gap length when formatting
; 08 - fill byte for format
; 09 - milliseconds to wait for head to settle
; 0a - time to wait for starting up drive motor (in 1/8 second)
        
        mov bx,0078        ; bx points to INT 1E location
        lds si,ss:[bx]     ; load ds:si with pointer at INT 1E
        push ds            ; save ds
        push si            ; save si               
        push ss            ; save ss                           
        push bx            ; save bx
        mov di,7C3E        ; set di to point to x'07C3E'
        mov cx,000B        ; need to move 11 bytes
        cld                ; count up when moving
        rep movsb          ; do the move
        push es            ; ...  
        pop ds             ; set ds = es         
        
        ; di has now changed because of the rep movsb
        mov byte ptr [di-02],0F      ; make 2nd last byte of DPT x'0F',
                                     ; ie 15 milliseconds for head settle
        mov cx,[7C18]      ; offset x'D' in BPB (sectors per track)
        mov [di-07],cl     ; put result in 7th last byte of DPT, ie
                           ; sectors per track
        mov [bx+02],ax     ; set segment of INT 1E = x'0000'
        mov word ptr [bx],7C3E  ; set offset of INT 1E = '7C3E' (our one!)
        sti                ; enable interrupts again
        int 13             ; reset the disk system
        jb GeneralErr        ; if CF=1 an error occurred
        xor ax,ax          ; ax = 0              
        cmp [7C13],ax      ; if total sectors in BPB = 0      
        je ExtendSect      ; we are dealing with an extended BPB
        mov cx,[7C13]      ; load cx with total number of sectors
        mov [7C20],cx      ; store it in the extension
        
ExtendSect:        
        mov al,[7C10]      ; get number of FATs from BPB
        mul word ptr [7C16] ; multiply by number of sectors/fat (from BPB)
        add ax,[7C1C]      ; add number of hidden sectors (from BPB)
        adc dx,[7C1E]      ; (add high order to dx of same)
        add ax,[7C0E]      ; add number of reserved sectors from BPB
        adc dx,0000        ; carry any overflow
        mov [7C50],ax      ; smash over some more used code with the answers!
        mov [7C52],dx      ; ie the total number of system sectors is in
        mov [7C49],ax      ; ax:dx
        mov [7C4B],dx      ;             
        mov ax,0020        ; Load ax with x'0020' (length of directory entry)
        mul word ptr [7C11] ; multiply by max # of root entries (from BPB)
        mov bx,[7C0B]      ; bx = # bytes per sector (from BPB)
        add ax,bx          ; ax has bytes in root plus an extra sector
        dec ax             ; Subtract 1 (in case we've added a whole sector)
        div bx             ; divide by bytes/sector to give total sectors
        add [7C49],ax      ; add this to our total number of system sectors
        adc word ptr [7C4B],0000 ; remember the carry
        mov bx,0500        ; Want to store sector at location x'00500'
        mov dx,[7C52]      ; ...
        mov ax,[7C50]      ; restore dx and ax
        call CvtSect       ; Ascertain track, head and sector numbers
        jb GeneralErr      ; Some problem calculating track number
        mov al,01          ; Read 1 sector
        call ReadSec       ; Do the read  
        jb GeneralErr      ; Some problem occurred doing the read      
        mov di,bx          ; Set di to x'0500', where sector was loaded
        mov cx,000B        ; set cx to 11
        mov si,7DE6        ; point to "io.sys" string
        rep cmpsb          ; do compare
        jne GeneralErr     ; if io.sys isn't first directory entry - error
        lea di,[bx+20]     ; Go to next directory entry (32 bytes in)
        mov cx,000B        ; compare 11 bytes
        rep cmpsb          ; do compare with "MSDOS.SYS" (very next entry)
        je GotMSDOS        ; go and load the files!
        
GeneralErr:                            
        mov si,7D9E        ; make si point to "\r\nNon-System etc"
        call PrintStr      ; print the string
        xor ax,ax          ; set function code = 0
        int 16             ; wait for keypress                 
        pop si             ; get back what was bx ...
        pop ds             ;   and ss which point to INT 1E
        pop word ptr [si]  ; restore what used to be in INT 1E
        pop word ptr [si+02] ;              
        int 19             ; System warm boot                 
        
CvsFail:        
        pop ax             ; We failed to do a sector conversion,
        pop ax             ;   so instead we're just clearing out the
        pop ax             ;   pushed registers and doing a normal abort
        jmp GeneralErr     ; I don't think we ever get here                    
        
GotMSDOS:        
        mov ax,[bx+1A]     ; directory entry for IO.SYS - begin cluster
        dec ax             ; ...
        dec ax             ;   subtract 2 from the cluster number
        mov bl,[7C0D]      ; load number of sectors per cluster                 
        xor bh,bh          ; clear out bh
        mul bx             ; now have start sector number in ax
        add ax,[7C49]      ; add number of system sectors (low)
        adc dx,[7C4B]      ; add with carry, number of system sectors (high)
        mov bx,0700        ; load at location x'00700'
        mov cx,0003        ; Want to read a total of 3 sectors
        
ReadPrep:        
        push ax            ; Save ax
        push dx            ; Save dx                 
        push cx            ; Save cx                 
        call CvtSect       ; Do track conversion            
        jb CvsFail         ; If a problem converting sectors, then fail
        mov al,01          ; Transfer 1 sector
        call ReadSec       ; Do the read                 
        pop cx             ; restore cx    
        pop dx             ; restore dx                 
        pop ax             ; restore ax                 
        jb GeneralErr      ; If problem loading sector, general fail
        add ax,0001        ; increment sector number (must be contiguous!)
        adc dx,0000        ; Carry over any carry                 
        add bx,[7C0B]      ; Add to bx the number of bytes per sector
        loop ReadPrep      ; Read next sector
        mov ch,[7C15]      ; Get media descriptor from BPB
        mov dl,[7C24]      ; Get physical drive number
        mov bx,[7C49]      ; Get number of system sectors (low)
        mov ax,[7C4B]      ; Get number of system sectors (high)
        jmp 0070:0000      ; Go to IO.SYS that you've just loaded at x'00700'
        
PrintStr:
        lodsb              ; Get byte at si
        or al,al           ; Do we have a non-zero byte?
        je CvtRet          ; Normal return (shared with CvtSect!)
        mov ah,0E          ; Write text in Teletype Mode function
        mov bx,0007        ; Display page 0, grey colour
        int 10             ; Do the teletype display
        jmp PrintStr       ; Get next byte
        
        
; CvtSect - convert absolute sector number into track, head and sector.
        
CvtSect:                        
        cmp dx,[7C18]      ; Make sure the high part of sector number
        jnb SectorOv       ; is less than the total sectors per track
        
; the above restriction is so that when a divide is done, there is no
; high-order remainder.  This means that the maximum number of tracks
; that will result is 65535.  With a maximum sector of 63, and a sector
; size of 512, that means that the sectors loaded at this stage must 
; all reside within the first 65535*63*512 = 2015 megabytes (ie less
; than 2 gig).  By changing this boot code, this restriction can be
; bypassed.

; Note another restriction - since the BIOS calls only allow 10 bits
; for the cylinder number, you must boot from within the first 1024
; cylinders, until such time as you reach a stage where you can bypass
; the BIOS (which you don't under DOS, which means you must keep all
; your DOS-visible partitions completely within the 1024 cylinders).
        
        div word ptr [7C18]  ; divide # sectors by sectors/track
        inc dl             ; plus 1 to get sector number 1-based
        mov [7C4F],dl      ; store sector number in x'7C4F'
        xor dx,dx          ; clear remainder
        div word ptr [7C1A] ; divide by # heads            
        mov [7C25],dl      ; store head # in reserved field
        mov [7C4D],ax      ; store remainder (track #) in x'7C4D'
        clc                ; make CF = 0 (success)
        ret                ; return
SectorOv:        
        stc                ; set CF = 1 (failure)
CvtRet:        
        ret                ; return            
        
ReadSec:
        mov ah,02          ; Read Disk Sectors function code
        mov dx,[7C4D]      ; Read track number [could be 10 bits]
        mov cl,06          ; Must move the highest 2 bits to the ...
        shl dh,cl          ;   extreme left of dh
        or dh,[7C4F]       ; Move sector number in as well
        mov cx,dx          ; Put in in cx, ready for swap
        xchg cl,ch         ; Swap the bytes
        mov dl,[7C24]      ; Read drive number from BPB
        mov dh,[7C25]      ; Get head number
        int 13             ; Do BIOS read sector
        ret                ; return

non_sys db "non-system disk, replace and press any key"  
sig     dw aa55
