#include <stddef.h>

#pragma linkage(__aopen, OS)
void *__aopen(const char *ddname, int mode, int *recfm, 
              int *lrecl, const char *mem);
#pragma linkage(__aread, OS)
int __aread(void *handle, void *buf);
#pragma linkage(__awrite, OS)
int __awrite(void *handle, const void *buf);
#pragma linkage(__aclose, OS)
void __aclose(void *handle);
#pragma linkage(__getclk, OS)
unsigned int __getclk(void *buf);
#pragma linkage(__getm, OS)
void *__getm(size_t sz);
#pragma linkage(__freem, OS)
void __freem(void *ptr);
