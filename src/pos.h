/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pos - some low-level dos functions                               */
/*                                                                   */
/*  In my opinion, all the MSDOS functions should have had both a    */
/*  low-level assember interrupt definition, and a functional        */
/*  interface for higher level languages such as C.  It is important */
/*  to have both interfaces as it allows anyone creating a new       */
/*  language to implement the functions themselves.  It is also a    */
/*  good idea to make the API start with a specific mneumonic like   */
/*  the Dos* and Win* etc functions you see in the OS/2 API.  It is  */
/*  not a good idea to pretend to be performing data abstraction by  */
/*  typedefing long as LONG etc.  Hence, the following is what I     */
/*  think should have been done to formalise the HLL Dos API.        */
/*                                                                   */
/*  Obviously it's a bit late to be standardizing this now, and not  */
/*  only that, but it's not much of a standard when I'm the only     */
/*  user of it either!  However, this interface is first and         */
/*  foremost to provide myself with a clearcut API so that I can     */
/*  make it 32-bit at the drop of a hat.                             */
/*                                                                   */
/*********************************************************************/

#include <stddef.h>

/* API functions */

void PosTermNoRC(void); /* int 20h */

/* int 21h */
void PosDisplayOutput(int ch); /* func 2 */

int PosDirectCharInputNoEcho(void); /*func 7*/

int PosGetCharInputNoEcho(void); /* func 8 */

void PosDisplayString(const char *buf); /* func 9 */

int PosSelectDisk(int drive); /* func e */

int PosGetDefaultDrive(void); /* func 19 */

void PosSetDTA(void *dta); /* func 1a */

void PosSetInterruptVector(int intnum, void *handler); /* func 25 */

void PosGetSystemDate(int *year, int *month, int *day, int *dw); /* func 2a */

void PosGetSystemTime(int *hour, /* func 2c */
                      int *min,
                      int *sec,
                      int *hundredths);

void *PosGetDTA(void); /* func 2f */

unsigned int PosGetDosVersion(void); /* func 30 */

void *PosGetInterruptVector(int intnum); /* func 35 */

void PosGetFreeSpace(int drive,
                     unsigned int *secperclu,
                     unsigned int *numfreeclu,
                     unsigned int *bytpersec,
                     unsigned int *totclus); /* func 36 */

int PosChangeDir(char *to); /* func 3b */

int PosCreatFile(const char *name, /* func 3c */
                 int attrib,
                 int *handle);

int PosOpenFile(const char *name, /* func 3d */
                int mode,
                int *handle);

int PosCloseFile(int handle); /* func 3e */

void PosReadFile(int fh, /* func 3f */
                 void *data,
                 size_t bytes,
                 size_t *readbytes);

int PosWriteFile(int handle, /* func 40 */
                 const void *data,
                 size_t len);

int PosDeleteFile(const char *fname); /* func 41 */

long PosMoveFilePointer(int handle, long offset, int whence); /* func 42 */

int PosGetFileAttributes(const char *fnm,int *attr);/*func 43*/

int PosGetDeviceInformation(int handle, unsigned int *devinfo);
      /* func 44 subfunc 0 */

int PosBlockDeviceRemovable(int drive); /* func 44 subfunc 8 */

int PosBlockDeviceRemote(int drive,int *da); /* func 44 subfunc 9 */

int PosGenericBlockDeviceRequest(int drive,
                                 int catcode,
                                 int function,
                                 void *parmblock); /*func 44 subfunc 0D*/


int PosGetCurDir(int drive, char *dir); /* func 47 */

void *PosAllocMem(unsigned int size); /* func 48 */

/* func 48 alternate entry really for 16-bit */
void *PosAllocMemPages(unsigned int pages, unsigned int *maxpages);

int PosFreeMem(void *ptr); /* func 49 */

/* func 4a */
int PosReallocPages(void *ptr, unsigned int newpages, unsigned int *maxp);

void PosExec(char *prog, void *parmblock); /* func 4b */

void PosTerminate(int rc); /* func 4c */

int PosGetReturnCode(void); /* func 4d */

int PosFindFirst(char *pat, int attrib); /* func 4e */

int PosFindNext(void); /* func 4f */

int PosRenameFile(const char *old, const char *new); /* func 56 */

int PosTruename(char *prename,char *postname); /*func 60*/


/* The following functions are extensions... */

void PosDisplayInteger(int x); /* func f3.00 */

void PosReboot(void); /* func f3.01 */

void PosSetRunTime(void *pstart, void *capi); /* func f3.02 */

unsigned int PosAbsoluteDiskRead(int drive, unsigned long start_sector,
                                 unsigned int sectors,void *buf); /*INT25 */


/* An easier-for-HLLs-to-use interface should also have been provided
   for executables.  Here is the makings of such an interface... */

typedef struct {
    int len;
    int (*__start)(void *exep);
    int (*printf)(const char *fmt, ...);
    void *(*malloc)(size_t size);
} POS_C_API;

typedef struct {
    int len;
    int abscor;
    int subcor;
    unsigned char *psp;
    unsigned char *cl;
    /* if anything is added before callback, strt32.s will need to change */
    void (*callback)(void);
    int (*pstart)(int *i1, int *i2, int *i3, void *eparms);
    /* beyond this point, no corrections have been done */
    void *posapi;
    void *bosapi;
    POS_C_API *crt;
} POS_EPARMS;

/*Structure for BPB*/
typedef struct {
    unsigned char BytesPerSector[2];
    unsigned char SectorsPerClustor;
    unsigned char ReservedSectors[2];
    unsigned char FatCount;
    unsigned char RootEntries[2];
    unsigned char TotalSectors16[2];
    unsigned char MediaType;
    unsigned char FatSize16[2];
    unsigned char SectorsPerTrack[2];
    unsigned char Heads[2];
    unsigned char HiddenSectors_Low[2];
    unsigned char HiddenSectors_High[2];
    unsigned char TotalSectors32[4];
} BPB;
/**/

/*Structure  for function call 440D*/
typedef struct {
    unsigned char special_function;
    unsigned char reservedspace[6];
    BPB bpb;
    unsigned char reserve2[100]; /* for OSes with bigger BPBs */
}PB_1560;
/**/

/*Structure for int25*/
typedef struct{
    unsigned long sectornumber;
    unsigned int numberofsectors;
    void *transferaddress;
}DP;
/**/

/*Structure for tempDTA in case of multiple PosFindFirst Calls*/
typedef struct { 
    int handle; 
    char pat[20]; 
}FFBLK;
/**/
