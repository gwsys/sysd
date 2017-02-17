#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>

#include "malloc.h"



static unsigned int  memorys=0;


void fatal()
{
	_exit(-1);
}

void *xmalloc(int bytes)
{
	void *ptr;

	ptr=malloc(bytes);

	if (ptr ) {
		memorys++;
		return ptr;

	}

	fatal();
	
	return NULL;
}

void *xmalloc_and_clean(int bytes)
{
	void *ptr;


	ptr=malloc(bytes);

	if (ptr ) {
		memorys++;
		memset(ptr, 0, bytes);
		return ptr;

	}

	fatal();
	
	return NULL;


}

void *xrealloc(void *ptr,int bytes)
{
	void *ptr1;

	ptr1=realloc(ptr, bytes);

	if (ptr1 ) {
		return ptr1;

	}

	fatal();
	
	return NULL;
}



void xfree(void *ptr) 
{
	free(ptr);
	memorys--;

	return;

}


char *xstrdup(char *s)
{
	int len;
	char *newstr;

	len=strlen(s);
	newstr=xmalloc(len+1);
	memcpy(newstr,s,len);
	newstr[len]=0;

	return newstr;



}

char *xstrncpy(char *dst,char *src, int n)
{
	if (n>0) { 
		strncpy(dst,src,n-1);
		dst[n-1]=0;
	} else {
		dst[0]=0;
	}
	return dst;

}


char *xsnprintf(char *dst,int  n, const char *fmt, ...)
{
	va_list ap;
	int size;

	if (n<=0) return NULL;

	va_start(ap,fmt);
	size=vsnprintf(dst,n-1,fmt,ap);
	va_end(ap);
	dst[n-1]=0;

	return dst;

}

unsigned int memuse()
{
	return memorys;
}












