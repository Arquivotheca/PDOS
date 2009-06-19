/* Startup code for Linux */
/* written by Paul Edwards */
/* released to the public domain */

#include "errno.h"
#include "stddef.h"

#define __KERNEL__
#include <asm-i486/unistd.h>


extern int __start(int argc, char **argv);

static _syscall1(int, exit, int, rc);
static _syscall1(unsigned long, time, void *, ptr);


/* We can get away with a minimal startup code, plus make it
   a C program. There is no return address. Instead, on the
   stack is a count, followed by all the parameters as pointers */

int _start(char *p)
{
    int rc;

    rc = __start(*(int *)(&p - 1), &p);
    exit(rc); /* this doesn't call the real exit. It calls a
                 static interrupt */
    return (rc);
}

void __exita(int rc)
{
    exit(rc); /* call our static function */
}

static char membuf[31000000];

void *__allocmem(size_t size)
{
    return (membuf);
}

unsigned long __time(void)
{
    return (time(NULL)); /* call static time */
}
