/* low-level functions - see funcs.c for doco */

int BosDiskSectorRead(void         *buffer,
                      unsigned long sectors,
                      unsigned long drive,
                      unsigned long track,
                      unsigned long head,
                      unsigned long sector);

int BosDriveParms(unsigned long drive,
                  unsigned long *tracks,
                  unsigned long *sectors,
                  unsigned long *heads,
                  unsigned long *attached,
                  unsigned char **parmtable,
                  unsigned long *drivetype);

int BosDiskReset(unsigned long drive);

int BosDiskStatus(unsigned long drive, unsigned long *status);






int BosPrintScreen(void);
int BosSetVideoMode(unsigned long mode);
int BosSetCursorType(int top, int bottom);
int BosSetCursorPosition(int page, int row, int column);
int BosReadCursorPosition(int page, 
                          int *cursorStart,
                          int *cursorEnd,
                          int *row,
                          int *column);
int BosReadLightPen(int *trigger,
                    unsigned long *pcolumn,
                    unsigned long *prow1,
                    unsigned long *prow2,
                    int *crow,
                    int *ccol);
int BosScrollWindowUp(int numLines,
                      int attrib,
                      int startRow,
                      int startCol,
                      int endRow,
                      int endCol);
int BosScrollWindowDown(int numLines,
                        int attrib,
                        int startRow,
                        int startCol,
                        int endRow,
                        int endCol);
int BosReadCharAttrib(int page, int *ch, int *attrib);
int BosWriteCharAttrib(int page, int ch, int attrib, unsigned long num);
int BosWriteCharCursor(int page, int ch, int col, unsigned long num);
int BosSetColorPalette(int id, int val);
int BosWriteGraphicsPixel(int page, 
                          int color, 
                          unsigned long row,
                          unsigned long column);
int BosReadGraphicsPixel(int page, 
                         unsigned long row,
                         unsigned long column,
                         int *color);
int BosWriteText(int page, int ch, int color);
int BosGetVideoMode(int *columns, int *mode, int *page);
