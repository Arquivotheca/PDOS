/* public domain by Paul Edwards */
/* these routines were originally written as part of msged */

#include <time.h>

void timet_to_dos(time_t now, void *dos);
time_t dos_to_timet(void *dos);
