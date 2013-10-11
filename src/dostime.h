/* public domain by Paul Edwards */
/* these routines were originally written as part of msged */

#include <time.h>

void timet_to_dos(time_t now,unsigned int *fdate,unsigned int *ftime);
time_t dos_to_timet(unsigned int fdate,unsigned int ftime);
