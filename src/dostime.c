/* public domain by Paul Edwards */
/* these routines were originally written as part of msged */

/* This is what the "dos date" looks like, but we don't rely on the
   internal structure of the compiler to do our routines */

/*
   typedef struct
   {
   unsigned int day:5;
   unsigned int mon:4;
   unsigned int year:7;
   unsigned int sec:5;
   unsigned int min:6;
   unsigned int hour:5;
   }
   DOSDATE;
 */

#include <time.h>

#include "dostime.h"


/* convert the time_t variable into dos-format */

void timet_to_dos(time_t now, unsigned int *fdate,unsigned int *ftime)
{
    struct tm *ts;
    unsigned int x;

    ts = localtime(&now);
    
    x = ts->tm_year - 80;
    x = (x << 4) | (ts->tm_mon + 1);
    x = (x << 5) | ts->tm_mday;
    *fdate=x;
    
    x = ts->tm_hour;
    x = (x << 6) | ts->tm_min;
    x = (x << 5) | (ts->tm_sec / 2);
    *ftime=x; 
   
    /*
    arr[0] = (unsigned char)((x >> 16) & 0xff);
    arr[1] = (unsigned char)((x >> 24) & 0xff);
    arr[2] = (unsigned char)(x & 0xff);
    arr[3] = (unsigned char)((x >> 8) & 0xff);*/
}


/* convert dos format into a time_t */

time_t dos_to_timet(unsigned int fdate,unsigned int ftime)
{
    struct tm tms;
    time_t tt;
    unsigned int x;

    x=ftime;
    tms.tm_sec = (x & 0x1f) * 2;
    x >>= 5;
    tms.tm_min = x & 0x3f;
    x >>= 6;
    tms.tm_hour = x & 0x1f;

    x=fdate;
    tms.tm_mday = x & 0x1f;
    x >>= 5;
    tms.tm_mon = (x & 0x0f) - 1;
    x >>= 4;
    tms.tm_year = (x & 0x7f) + 80;
    tms.tm_isdst = -1;
    tt = mktime(&tms);
    return (tt);
}
