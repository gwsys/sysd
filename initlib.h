#ifndef _INITLIB_H
#define _INITLIB_H


struct init_config{
	char *progname;
	char *pidfile;
	char *lockfile;
	int lock;
	int detach;
	int syslog;
	int conlog;
	int shutdown;
	int reload;
	int usr1;
	int usr2;
	int debug;
	char namebf[NBFSZ];
	char pidbf[PBFSZ];
	char lkbf[PBFSZ];
};

extern struct init_config *iconfig;


#define imsg(msg_fmt, ...)      \
{   \
	if ( iconfig->syslog ) syslog(LOG_INFO, msg_fmt, ##__VA_ARGS__);  \
	if ( iconfig->conlog )   printf(msg_fmt"\n",##__VA_ARGS__);      \
}


#define emsg(msg_fmt, ...)      \
{     \
	if ( iconfig->syslog ) syslog(LOG_ERR, msg_fmt, ##__VA_ARGS__);    \
	if ( iconfig->conlog ) printf(msg_fmt"\n",##__VA_ARGS__);    \
}

#define errinfo(msg_fmt, ...) \
{     \
	if ( iconfig->syslog )syslog(LOG_ERR, msg_fmt"(%s)", ##__VA_ARGS__,strerror(errno));    \
	if ( iconfig->conlog ) printf(msg_fmt"(%s)\n",##__VA_ARGS__,strerror(errno));    \
}


#define dmsg(msg_fmt, ...)      \
{       \
	if ( iconfig->debug && ( iconfig->syslog || iconfig->conlog  ) ) { \
		struct timeval tv;  \
		unsigned int sec, usec;   \
		gettimeofday(&tv, NULL);   \
		sec=(unsigned int)tv.tv_sec; \
		usec=(unsigned int) tv.tv_usec; \
		if (iconfig->syslog ) syslog(LOG_DEBUG, "%010u.%06u "msg_fmt,sec,usec, ##__VA_ARGS__);   \
		if ( iconfig->conlog ) printf( "%010u.%06u "msg_fmt"\n",sec,usec, ##__VA_ARGS__);    \
	} \
}

int init();
void finit();


#endif

