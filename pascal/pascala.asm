# This code is used as a part of a runtime for Free Pascal
# It converts the Pascal calls into C calls
# This should probably be written in Pascal instead, but
# I don't know how to do that. The code probably works on
# all 80386 environments but I only tested Win32
#
# Actually, another option would be for a compiler option
# in Free Pascal to generate C calling convention for the
# calls to the runtime library functions (instead of the
# current register calling convention used by Delphi and
# Free Pascal).

# written by Paul Edwards
# released to the public domain  

.section .text
	.balign 16,0x90

.globl fpc_initializeunits
fpc_initializeunits:
       call _fpc_initializeunits
       ret

.globl fpc_get_output
fpc_get_output:
       call _fpc_get_output
       ret


# edx has file handle
# ecx has string to print, first character has length
# eax has a 0, don't know what that is

.globl fpc_write_text_shortstr
fpc_write_text_shortstr:
       push %eax
       push %ecx
       push %edx
       call _fpc_write_text_shortstr

       add $12, %esp
       ret

.globl fpc_write_text_sint
fpc_write_text_sint:
       push %eax
       push %ecx
       push %edx
       call _fpc_write_text_sint

       add $12, %esp
       ret

.globl fpc_iocheck
fpc_iocheck:
       call _fpc_iocheck
       ret

.globl fpc_writeln_end
fpc_writeln_end:
       push %eax
       call _fpc_writeln_end
       add $4, %esp
       ret

.globl fpc_do_exit
fpc_do_exit:
       call _fpc_do_exit
       ret

# %eax is destination, %edx is source
.globl SYSTEM_$$_ASSIGN$TEXT$SHORTSTRING
SYSTEM_$$_ASSIGN$TEXT$SHORTSTRING:
       push %edx
       push %eax
       call _fpc_assign_short_string
       add $8, %esp
       ret

.globl SYSTEM_$$_REWRITE$TEXT
SYSTEM_$$_REWRITE$TEXT:
       push %eax
       call _fpc_rewrite
       add $4, %esp
       ret

.section .data

.globl INIT$_$SYSTEM
INIT$_$SYSTEM:

.globl INIT$_$FPINTRES
INIT$_$FPINTRES:

.globl THREADVARLIST_$SYSTEM$indirect
THREADVARLIST_$SYSTEM$indirect:
