/* public domain by Paul Edwards */

#define MODE_OLDFILE 1005
#define MODE_NEWFILE 1006

#define OFFSET_BEGINNING -1
#define OFFSET_CURRENT 0
#define OFFSET_END 1

void *Input(void);
void *Output(void);
void *Open(const char *fnm, long b);
long Read(void *a, void *b, long c);
long Write(void *a, void *b, long c);
long Seek(void *a, long offs, long c);
int Close(void *a);
