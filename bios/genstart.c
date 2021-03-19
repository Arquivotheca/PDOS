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

extern void (*__cbfunc)(void *cbdata, int funccode, void *retptr, char *str);
extern void *__cbdata;

int main(void);

int __crt0(void (*cbfunc)(void *cbdata, int funccode, void *retptr, char *str), void *cbdata)
{
    __cbfunc = cbfunc;
    __cbdata = cbdata;
    return (main());
}
