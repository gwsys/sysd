
#include "inc.h"

#include "xmalloc.h"
#include "strlib.h"

unsigned int sl_hash_uint32(unsigned int key)
{
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key;
}

arg_t* sl_arg_new(char *s)
{
	arg_t *arg;
	int len;
	if (s && s[0] ) len=strlen(s)+8;
	else len=0;
	len=( len>>3)<<3 ;
	
	arg=(arg_t*)xmalloc( sizeof(arg_t)+len+sizeof(char*)*SL_ARG_BFINC );
	arg->num=0;
	arg->token=(char**)(arg->bf+len);
	arg->bflen=len+sizeof(char*)*SL_ARG_BFINC;
	arg->strlen=len;
	arg->max=SL_ARG_BFINC;
	if (s) strcpy(arg->bf,s);
	
	return arg;
	
}

void sl_arg_destroy(arg_t *arg)
{
	if ( arg->token != (char**)(arg->bf+arg->strlen) ) xfree(arg->token);

	xfree(arg);

}

void sl_arg_check(arg_t *arg)
{
	char **newp;
	
	if (arg->num>=arg->max) {
		newp=(char**)xmalloc(sizeof(char*)*(arg->max+SL_ARG_BFINC));
		memcpy(newp,arg->token,sizeof(char*)*arg->max);
		if ( arg->token != (char**)(arg->bf+arg->strlen) ) xfree(arg->token);		
		arg->token=newp;
		arg->max+=SL_ARG_BFINC;
	}
}

void sl_arg_add(arg_t *arg, char *s, int len)
{
	sl_arg_check(arg);

	arg->token[arg->num]=s;
	s[len]=0;
	arg->num++;
}

char* sl_arg_str(arg_t *arg) 
{
	return (arg->strlen)?arg->bf:NULL;
}


char* sl_ltrim( char* s )
{
	if( !s ) return NULL;

	char* p;
	int len;

	len=strlen(s);

	for( p = s; *p != 0 && ( *p == 32 || *p == 9 || *p == 10 || *p == 13 ); p++ ) {}

	if( p>s )
	memcpy( s, p, s+len-p+1 );

	return s;
}

char* sl_rtrim( char *s )
{
	char *p;
	int len;

	if( s == NULL ) return NULL;

	len = strlen( s );

	for( p = s+len-1; p >= s && ( *p == 32 || *p == 9 || *p == 10 || *p == 13 ) ; p-- )
	{
		*p = 0;
	}

	return s;
}

char* sl_trim( char* s )
{
	sl_ltrim( s );

	sl_rtrim( s );

	return s;
}

char* sl_split_token(char *s, char *sep, int mode, int *len)
{
	char *p,*e;

	if ( s==NULL || *s==0 ) return NULL;

 	if (mode == SL_SPM_SPACE ) {
		for ( p=s ; *p && isspace(*p) ; p++ ) {}
		if ( *p ) {
			for (e=p; *e && ! isspace(*e) ; e++ ) {}
			*len=e-p;
			return p;
		}
 	}
	else if (mode == SL_SPM_CHAR ) {
		p=s;
		e=strchr(p,sep[0]);
		if ( e ) {
			*len= e-p;
		} else {
			*len=strlen(p);
		}

		return p;
		
	}
	else if (mode == SL_SPM_CHARSET ) {
		for (p=s; *p && strchr(sep,*p) ;p++ ) {}

		if ( *p ) {
			for ( e=p ; *e && ! strchr(sep,*e) ; e++ ) {}
			*len=e-p;
			return p;
		}
	}
	else if ( mode == SL_SPM_STR ) {
		p=s ;
		e=strstr(p,sep);
		if ( e ) {
			*len=e-p;
		} else {
			*len=strlen(p);
		}
		return p;
	}

	return NULL;
}




int sl_split_str(char *s, char *sep, int mode,char **arg, int max)
{
	char *p=s;
	char *e=p+strlen(s);
	int seplen=sep?strlen(sep):0;
	int len;
	int cnt=0;
	
	while (p && p<e && cnt<max) {
		p=sl_split_token(p,sep,mode,&len);
		if (p ) {
			p[len]=0;
			arg[cnt]=p;
			cnt++;
			if (mode==SL_SPM_STR ) p+=len+seplen;
			else p+=len+1;
		}
	}

	return cnt;
}

int sl_split_arg(arg_t *arg, char *sep, int mode)
{
	char *s=sl_arg_str(arg);
	if ( !s) return 0;
	
	char *p=s;
	char *e=p+strlen(s);
	int seplen=sep?strlen(sep):0;
	int len;
	
	while (p && p<e) {
		p=sl_split_token(p,sep,mode,&len);
		if (p ) {
			sl_arg_add(arg,p,len);
			if (mode==SL_SPM_STR ) p+=len+seplen;
			else p+=len+1;
		}
	}

	return arg->num;
	
}

arg_t* sl_split_copy(char *s, char *sep, int mode)
{
	arg_t *arg=sl_arg_new(s);

	sl_split_arg(arg,sep,mode);

	return arg;
}



int sl_str2uint(char *s, unsigned int *ret)
{
	char *p=s;
	unsigned int val=0;

	sl_trim(p);

	if ( isdigit(*p) ) {
		for (; isdigit(*p); p++ ) {
			val=val*10+(*p-'0');
		}
		if ( *p==0 ) {
			*ret=val;
			return 0;
		}		
	}

	return -1;

	
}

