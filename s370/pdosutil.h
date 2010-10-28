/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdosutil - utilities used by PDOS and possibly PLOAD             */
/*                                                                   */
/*********************************************************************/

int findFile(int ipldev, char *dsn, int *c, int *h, int *r);
int fixPE(char *buf, int *len, int *entry, int rlad);
int processRLD(char *buf, int rlad, char *rld, int len);
