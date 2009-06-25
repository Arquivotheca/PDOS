# linsupa.asm - support code for C programs for Linux
# 
# This program written by Paul Edwards
# Released to the public domain

.globl __setj
__setj:
push %ebp
mov %esp, %ebp
push %eax
push %ebx
push %ecx
push %edx

pop %edx
pop %ecx
pop %ebx
pop %eax
pop %ebp
ret

.globl __longj
__longj:
push %ebp
mov %esp, %ebp
push %eax
push %ebx
push %ecx
push %edx

pop %edx
pop %ecx
pop %ebx
pop %eax
pop %ebp
ret
