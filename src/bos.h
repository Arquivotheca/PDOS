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

int BosPrintScreen(void); /* 5: */
int BosSetVideoMode(unsigned int mode); /* 10:0 */
int BosSetCursorType(int top, int bottom); /* 10:1 */
int BosSetCursorPosition(int page, int row, int column); /* 10:2 */
int BosReadCursorPosition(int page, /* 10:3 */
                          int *cursorStart,
                          int *cursorEnd,
                          int *row,
                          int *column);
int BosReadLightPen(int *trigger, /* 10:4 */
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

int BosReadKeyboardCharacter(int *scancode, int *ascii); /* 16:0 */

void BosSystemWarmBoot(void); /* 19: */

unsigned long BosGetSystemTime(void);/*1A.0*/

int BosGetSystemDate(int *century,int *year,int *month,int *day);/*1A.4*/
