/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  genstart - startup routine for generic BIOS/OS interface         */
/*                                                                   */
/*********************************************************************/

extern void (*__exposfunc)(void *cbdata, int funccode, void *retptr, char *str);
extern void *__cbdata;

int main(void);


/* This name is known to certain versions of "ld" as the entry
point of an application and there is no particular reason to not
use it. */

int __crt0(void (*exposfunc)(void *cbdata, int funccode, void *retptr, char *str), void *cbdata)
{
    __exposfunc = exposfunc;
    __cbdata = cbdata;
    return (main());
}


/* Unfortunately gccwin and gcc have been built without
HAS_INIT_SECTION defined, so this dummy function is
required to satisfy the mandatory call from main() */

int __main(void)
{
    return (0);
}


/* Watcom requires this to be defined */

void _cstart_(void)
{
    return;
}
