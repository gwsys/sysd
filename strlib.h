#ifndef _STRLIB_H
#define _STRLIB_H

char* sl_ltrim( char* s );
char* sl_rtrim( char* s );
char* sl_trim( char* s );

#define SL_ARG_BFINC 32
#define SL_SPM_SPACE 0
#define SL_SPM_CHAR 1
#define SL_SPM_CHARSET 2
#define SL_SPM_STR 3

typedef struct argument{
	int num;
	int bflen;
	int strlen;
	int max;	
	char **token;
	char bf[SL_ARG_BFINC];
} arg_t;



arg_t* sl_arg_new(char *s);
void sl_arg_destroy(arg_t *arg);
void sl_arg_check(arg_t *arg);
void sl_arg_add(arg_t *arg, char *s, int len);
char* sl_arg_str(arg_t *arg) ;

char* sl_split_token(char *s, char *sep, int mode, int *len);
int sl_split_str(char *s, char *sep, int mode,char **arg, int max);
int sl_split_arg(arg_t *arg, char *sep, int mode);
arg_t* sl_split_copy(char *s, char *sep, int mode);


int sl_str2uint(char *s, unsigned int *ret);


#endif


