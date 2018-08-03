/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  bos.h - this is the interface to the BIOS for use by HLLs        */
/*                                                                   */
/*********************************************************************/

#define BOS_ERROR 1 

void BosPrintScreen(void); /* 5: */
unsigned int BosSetVideoMode(unsigned int mode); /* 10:0 */
void BosSetCursorType(unsigned int top, unsigned int bottom); /* 10:1 */
void BosSetCursorPosition(unsigned int page, unsigned int row, 
                          unsigned int column); /* 10:2 */
void BosReadCursorPosition(unsigned int page, /* 10:3 */
                           unsigned int *cursorStart,
                           unsigned int *cursorEnd,
                           unsigned int *row,
                           unsigned int *column);
void BosReadLightPen(int *trigger, /* 10:4 */
                    unsigned int *pcolumn,
                    unsigned int *prow1,
                    unsigned int *prow2,
                    int *crow,
                    int *ccol);
int BosSetActiveDisplayPage(int page); /* 10:5 */
                    
int BosScrollWindowUp(int numLines, /* 10:6 */
                      int attrib,
                      int startRow,
                      int startCol,
                      int endRow,
                      int endCol);
int BosScrollWindowDown(int numLines, /* 10:7 */
                        int attrib,
                        int startRow,
                        int startCol,
                        int endRow,
                        int endCol);
int BosReadCharAttrib(int page, int *ch, int *attrib); /* 10:8 */
int BosWriteCharAttrib(int page, 
                       int ch, 
                       int attrib, 
                       unsigned int num); /* 10:9 */
int BosWriteCharCursor(int page, int ch, int col, unsigned int num); /* 10:A */
int BosSetColourPalette(int id, int val); /* 10:B */
int BosWriteGraphicsPixel(int page, /* 10:C */
                          int colour, 
                          unsigned int row,
                          unsigned int column);
int BosReadGraphicsPixel(int page, /* 10:D */
                         unsigned int row,
                         unsigned int column,
                         int *colour);
int BosWriteText(int page, int ch, int colour); /* 10:E */
int BosGetVideoMode(int *columns, int *mode, int *page); /* 10:F */

int BosVBEGetInfo(void *buffer); /* 10:4F00 */

int BosVBEGetModeInfo(unsigned int mode, void *buffer); /* 10:4F01 */

int BosVBESetMode(unsigned int mode, void *buffer); /* 10:4F02 */

int BosVBEGetMode(unsigned int *mode); /* 10:4F03 */

int BosVBEPaletteOps(unsigned int operation, /* 10:4F09 */
                     unsigned int entries,
                     unsigned int start_index,
                     void *buffer);

int BosDiskReset(unsigned int drive); /* 13:0 */

int BosDiskStatus(unsigned int drive, unsigned int *status); /* 13:1 */

int BosDiskSectorRead(void         *buffer, /* 13:2 */
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned int track,
                      unsigned int head,
                      unsigned int sector);

int BosDiskSectorWrite(void         *buffer, /* 13:3 */
                       unsigned int  sectors,
                       unsigned int  drive,
                       unsigned int  track,
                       unsigned int  head,
                       unsigned int  sector);

int BosDriveParms(unsigned int drive, /* 13:8 */
                  unsigned int *tracks,
                  unsigned int *sectors,
                  unsigned int *heads,
                  unsigned int *attached,
                  unsigned char **parmtable,
                  unsigned int *drivetype);

int BosFixedDiskStatus(unsigned int drive); /* 13:10 */

int BosDiskSectorRLBA(void         *buffer, /* 13:42 */
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned long sector,
                      unsigned long hisector);

int BosDiskSectorWLBA(void         *buffer, /* 13:43 */
                      unsigned int sectors,
                      unsigned int drive,
                      unsigned long sector,
                      unsigned long hisector);

long BosExtendedMemorySize(void); /* 15:88 */

void BosReadKeyboardCharacter(int *scancode, int *ascii); /* 16:0 */

int BosReadKeyboardStatus(int *scancode, int *ascii); /* 16:1 */

void BosSystemWarmBoot(void); /* 19: */

void BosGetSystemTime(unsigned long *ticks, unsigned int *midnight);/*1A.0*/

unsigned int BosGetSystemDate(int *century,int *year,int *month,
                              int *day);/*1A.4*/

void BosSetSystemDate(int century,int year,int month,
                              int day);/*1A.5*/
