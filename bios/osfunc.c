/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  osfunc - interface to call to get BIOS/OS services               */
/*                                                                   */
/*********************************************************************/

extern int x;

void (*__cbfunc)(void *cbdata, int funccode, void *retptr, char *str);
void *__cbdata;

void osfunc(int funccode, void *retptr, char *str)
{
    /* x = 4; */
    __cbfunc(__cbdata, funccode, retptr, str);
    return;
}
