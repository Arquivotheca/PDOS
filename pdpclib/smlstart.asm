;
; Released to the public domain by Alexey Frunze.
;

; File: c0dh_jw.asm
;
; Attempt to adapt Smaller C's c0dh.asm from NASM to JWASM.

.model large
.386

    public ___intstart
    public ___exita

    public ___creat
    public ___open
    public ___close
    public ___read
    public ___write
    public ___remove
    public ___rename
    public ___seek
    public ___lesf2
    public ___mulsf3
    public ___floatsisf
    public ___addsf3
    public ___negsf2
    public ___gesf2
    public ___divsf3
    public ___fixsfsi
    public ___subsf3

    public ___psp
    public ___envptr
    public ___osver

;extrn __start:proc
    extern ___start:byte
    extern __start__relot:byte, __stop__relot:byte
    extern __start__relod:byte, __stop__relod:byte
    extern __start__bss:byte, __stop__bss:byte

text segment "CODE"

___intstart:

    ; perform code/data relocations

    call    labnext
labnext:
    xor     ebx, ebx
    mov     bx, cs
    shl     ebx, 4
    xor     eax, eax
    pop     ax
    add     ebx, eax
    sub     ebx, offset labnext ; ebx = base physical address

    mov     esi, offset __start__relot
relo_text_loop:
    cmp     esi, offset __stop__relot
    jae     relo_text_done

    lea     edi, [ebx + esi] ; edi = physical address of a relocation table element

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     edi, [di]
    add     edi, ebx ; edi = physical address of a dword to which to add ebx and which then to transform into seg:ofs far pointer

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     eax, [di]
    add     eax, ebx

    shl     eax, 12
    rol     ax, 4

    mov     [di], eax

    add     esi, 4
    jmp     relo_text_loop
relo_text_done:

    mov     esi, offset __start__relod
relo_data_loop:
    cmp     esi, offset __stop__relod
    jae     relo_data_done

    lea     edi, [ebx + esi] ; edi = physical address of a relocation table element

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     edi, [di]
    add     edi, ebx ; edi = physical address of a dword to which to add ebx

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    add     [di], ebx ; actual relocation

    add     esi, 4
    jmp     relo_data_loop
relo_data_done:

    ; Init .bss

    push    ebx

    mov     esi, offset __start__bss
    lea     edi, [ebx + esi]
    mov     esi, offset __stop__bss
    lea     ebx, [ebx + esi]
    sub     ebx, edi
    ror     edi, 4
    mov     es, di
    shr     edi, 28
    xor     al, al
    cld

bss1:
    mov     ecx, 32768
    cmp     ebx, ecx
    jc      bss2

    sub     ebx, ecx
    rep     stosb
    and     di, 15
    mov     si, es
    add     si, 2048
    mov     es, si
    jmp     bss1

bss2:
    mov     cx, bx
    rep     stosb

    pop     ebx

    mov     esi, offset term
    lea     eax, [ebx + esi]
    shl     eax, 12
    rol     ax, 4
    push    eax

    mov     esi, offset ___start
    lea     eax, [ebx + esi]
    shl     eax, 12
    rol     ax, 4
    push    eax

    retf        ; call ___start

term:
    mov     al, 0
    mov     ah, 4ch
    int     21h ; terminate

rt:
    dd      offset rt

;___exita proc, retcode: word
___exita:
        ret
;___exita endp

___creat:
___open:
___close:
___read:
___write:
___remove:
___rename:
___seek:
___lesf2:
___mulsf3:
___floatsisf:
___addsf3:
___negsf2:
___gesf2:
___divsf3:
___fixsfsi:
___subsf3:
        ret

text ends

_relot segment dword "CONST"
    dd      offset rt ; .relot must exist for __start__relot and __stop__relot to also exist
_relot ends

data segment "DATA"
rd:
    dd      offset rd

banner  db  "PDPCLIB"
___psp   dd  ?
___envptr dd  ?
___osver dw  ?
data ends

_relod segment dword "CONST"
    dd      offset rd ; .relod must exist for __start__relod and __stop__relod to also exist
_relod ends

_bss segment "BSS"
    ; .bss must exist for __start__bss and __stop__bss to also exist
    dd      ?
_bss ends

end




/*
  Released to the public domain by Alexey Frunze.
*/

// File: x87.c

#ifdef __SMALLER_C_32__

#ifdef __HUGE__
#define __HUGE_OR_UNREAL__
#endif
#ifdef __UNREAL__
#define __HUGE_OR_UNREAL__
#endif

#ifdef __HUGE_OR_UNREAL__
#define xbp "bp"
#define xsp "sp"
#else
#define xbp "ebp"
#define xsp "esp"
#endif

void __x87init(void)
{
  // Mask all exceptions, set rounding to nearest even and precision to 64 bits.
  // TBD??? Actually check for x87???
  asm("fninit");
}

float __floatsisf(int i)
{
  asm
  (
  "fild dword ["xbp"+8]\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

float __floatunsisf(unsigned i)
{
  asm
  (
  "push dword 0\n"
  "push dword ["xbp"+8]\n"
  "fild qword ["xbp"-8]\n" // load 32-bit unsigned int as 64-bit signed int
  "add  "xsp", 8\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

int __fixsfsi(float f)
{
  asm
  (
  "sub    "xsp", 4\n"
  "fnstcw ["xbp"-2]\n" // save rounding
  "mov    ax, ["xbp"-2]\n"
  "mov    ah, 0x0c\n" // rounding towards 0 (AKA truncate) since we don't have fisttp
  "mov    ["xbp"-4], ax\n"
  "fld    dword ["xbp"+8]\n"
  "fldcw  ["xbp"-4]\n" // rounding = truncation
  "fistp  dword ["xbp"+8]\n"
  "fldcw  ["xbp"-2]\n" // restore rounding
  "add    "xsp", 4\n"
  "mov    eax, ["xbp"+8]"
  );
}

unsigned __fixunssfsi(float f)
{
  asm
  (
  "sub    "xsp", 12\n"
  "fnstcw ["xbp"-2]\n" // save rounding
  "mov    ax, ["xbp"-2]\n"
  "mov    ah, 0x0c\n" // rounding towards 0 (AKA truncate) since we don't have fisttp
  "mov    ["xbp"-4], ax\n"
  "fld    dword ["xbp"+8]\n"
  "fldcw  ["xbp"-4]\n" // rounding = truncation
  "fistp  qword ["xbp"-12]\n" // store 64-bit signed int
  "fldcw  ["xbp"-2]\n" // restore rounding
  "mov    eax, ["xbp"-12]\n" // take low 32 bits
  "add    "xsp", 12"
  );
}

float __addsf3(float a, float b)
{
  asm
  (
  "fld  dword ["xbp"+8]\n"
  "fadd dword ["xbp"+12]\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

float __subsf3(float a, float b)
{
  asm
  (
  "fld  dword ["xbp"+8]\n"
  "fsub dword ["xbp"+12]\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

float __negsf2(float f)
{
  asm
  (
  "mov eax, ["xbp"+8]\n"
  "xor eax, 0x80000000"
  );
}

float __mulsf3(float a, float b)
{
  asm
  (
  "fld  dword ["xbp"+8]\n"
  "fmul dword ["xbp"+12]\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

float __divsf3(float a, float b)
{
  asm
  (
  "fld  dword ["xbp"+8]\n"
  "fdiv dword ["xbp"+12]\n"
  "fstp dword ["xbp"+8]\n"
  "mov  eax, ["xbp"+8]"
  );
}

float __lesf2(float a, float b)
{
  asm
  (
  "fld      dword ["xbp"+12]\n"
  "fld      dword ["xbp"+8]\n"
  "fucompp\n"
  "fstsw    ax\n" // must use these moves since we don't have fucomip
  "sahf\n"
  "jnp      .ordered\n"
  "mov      eax, +1\n" // return +1 if either a or b (or both) is a NaN
  "jmp      .done\n"
  ".ordered:\n"
  "jnz      .unequal\n"
  "xor      eax, eax\n" // return 0 if a == b
  "jmp      .done\n"
  ".unequal:\n"
  "sbb      eax, eax\n"
  "jc       .done\n"    // return -1 if a < b
  "inc      eax\n"      // return +1 if a > b
  ".done:"
  );
}

float __gesf2(float a, float b)
{
  asm
  (
  "fld      dword ["xbp"+12]\n"
  "fld      dword ["xbp"+8]\n"
  "fucompp\n"
  "fstsw    ax\n" // must use these moves since we don't have fucomip
  "sahf\n"
  "jnp      .ordered\n"
  "mov      eax, -1\n" // return -1 if either a or b (or both) is a NaN
  "jmp      .done\n"
  ".ordered:\n"
  "jnz      .unequal\n"
  "xor      eax, eax\n" // return 0 if a == b
  "jmp      .done\n"
  ".unequal:\n"
  "sbb      eax, eax\n"
  "jc       .done\n"    // return -1 if a < b
  "inc      eax\n"      // return +1 if a > b
  ".done:"
  );
}

#endif
