#include <stdio.h>
#include <stdlib.h>

#include <alloc.h>

#include "protint.h"
#include "support.h"
#include "unused.h"
#include "pdos.h"
#include "support.h"
#include "lldos.h"

static unsigned long doreboot(unsigned long parm);

int main(void)
{
    unsigned char buf[] = { 0xb8, 0x78, 0x56, 0x34, 0x12, 0xc3 };
    static unsigned char bpb[2000];
    static unsigned char stack[2000];
    char far *f;
    static unsigned char transferbuf[512];
    pdos_parms pp;
    int ret;

    f = farmalloc(220000);
    if (f == NULL)
    {
        return (0);
    }
    a20e(); /* enable a20 line */
    pp.transferbuf = ADDR2ABS(transferbuf);
    pp.doreboot = (unsigned long)doreboot;
    pp.bpb = ADDR2ABS(bpb);
    ret = runaout("pdos.exe", ADDR2ABS(f), ADDR2ABS(&pp));
    printf("ret is %d\n", ret);
    return (0);
}

static unsigned long doreboot(unsigned long parm)
{
    unused(parm);
    reboot();
    return (0);
}
