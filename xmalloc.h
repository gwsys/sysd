#ifndef _XMALLOC_H
#define _XMALLOC_H


void *xmalloc(int bytes);
void *xmalloc_and_clean(int bytes);
void *xrealloc(void *ptr,int bytes);
void xfree(void *ptr) ;
char *xstrdup(char *s);
char *xstrncpy(char *dst,char *src, int n);
char *xsnprintf(char *dst,int  n, const char *fmt, ...);
unsigned int memuse();




#endif





