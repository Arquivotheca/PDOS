/* public domain by Paul Edwards */

#define MODE_OLDFILE 1005
#define MODE_NEWFILE 1006

void *Input(void);
void *Output(void);
void *Open(const char *fnm, long b);
long Read(void *a, void *b, long c);
long Write(void *a, void *b, long c);
