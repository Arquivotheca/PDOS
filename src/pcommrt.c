/*********************************************************************/
/*                                                                   */
/*  pcommrt - install runtime apis                                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "pos.h"

int __start(void *exep);

static POS_C_API c_api;

/* pstart - routine to examine exeparms */

int pstart(int *i1, int *i2, int *i3, void *eparms)
{
    void *(*mymalloc)(size_t size);
    POS_EPARMS *parms = eparms;
    POS_C_API *crt;
    
    crt = (POS_C_API *)((char *)parms->crt - parms->subcor + parms->abscor);    
    mymalloc = crt->malloc;
    mymalloc = (void *(*)(size_t))
               ((char *)mymalloc - parms->subcor + parms->abscor);
    parms->crt = mymalloc(crt->len);
    parms->crt->malloc = mymalloc;
    parms->crt->printf = (int (*)(const char *fmt, ...))
        ((char *)crt->printf - parms->subcor + parms->abscor);
    parms->crt->__start = (int (*)(void *exep))
        ((char *)crt->__start - parms->subcor + parms->abscor);
    return (parms->crt->__start(eparms));
}

void install_runtime(void)
{
    c_api.malloc = malloc;
    c_api.printf = printf;
    c_api.__start = __start;
    PosSetRunTime(pstart, &c_api);
    return;
}
