/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  file - mini stdio implementation                                 */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>

#include "fat.h"
#include "bos.h"
#include "lldos.h"
#include "unused.h"

static FATFILE fatfile;
extern FAT gfat;

FILE *fopen(const char *name, const char *mode)
{
    unused(mode);
    fatOpenFile(&gfat, name, &fatfile);
    return ((FILE *)1);
}

int fclose(FILE *fp)
{
    unused(fp);
    return (0);
}

size_t fread(void *buf, size_t size, size_t nelem, FILE *fp)
{
    size_t ret;
    
    unused(size);
    unused(fp);
    ret = fatReadFile(&gfat, &fatfile, buf, nelem);
    return (ret);
}
