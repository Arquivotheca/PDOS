# linsupa.asm - support code for C programs for Linux
# 
# This program written by Paul Edwards
# Released to the public domain

.globl __setj
__setj:
push %ebp
mov %esp, %ebp

movl 8(%ebp), %eax
push %ebp
mov %esp, %ebx
push %ebx # esp
        
movl %ecx, 4(%eax)
movl %edx, 8(%eax)
movl %edi, 12(%eax)
movl %esi, 16(%eax)

pop %ebx
movl %ebx, 20(%eax) # esp
movl 0(%ebp), %ebx
movl %ebx, 24(%eax) # ebp

movl 4(%ebp), %ebx # return address
movl %ebx, 28(%eax) # return address

pop %ebx
movl %ebx, 0(%eax)
mov $0, %eax

pop %ebp
ret



.globl __longj
__longj:
push %ebp
mov %esp, %ebp

movl 8(%ebp), %eax
movl 20(%eax), %ebp
mov %ebp, %esp

pop %ebx # position of old ebx
pop %ebx # position of old ebp
pop %ebx # position of old return address

mov 28(%eax), %ebx # return address
push %ebx

mov 24(%eax), %ebx # ebp saved as normal
push %ebx
mov %esp, %ebp

movl 0(%eax), %ebx
movl 4(%eax), %ecx
movl 8(%eax), %edx
movl 12(%eax), %edi
movl 16(%eax), %esi       

movl 32(%eax), %eax

pop %ebp

ret
