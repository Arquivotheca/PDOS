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

void (*__exposfunc)(void *cbdata, int funccode, void *retptr, char *str);
void *__cbdata;

void osfunc(int funccode, void *retptr, char *str)
{
    __exposfunc(__cbdata, funccode, retptr, str);
    return;
}
