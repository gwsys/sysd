#include "inc.h"

#include "initlib.h"


struct init_config iconfig_data={
	.progname=NULL,
	.pidfile=NULL,
	.lockfile=NULL,
	.detach=1,
	.syslog=1,
	.conlog=0,
	.shutdown=1,
	.reload=1,
	.usr1=1,
	.usr2=1,
	.debug=0
};

struct init_config *iconfig=&iconfig_data;


int log_init()
{
	openlog(iconfig->progname, LOG_NDELAY|LOG_PID, LOG_LOCAL0);
	return 0;
}


void log_free()
{
	closelog();
	return ;
}

void sigint_handler(int i)
{
	imsg( "catched SIGINT signal (%d)", i);
	iconfig->shutdown=1;
}

void sigterm_handler(int i)
{
	imsg( "catched SIGTERM signal (%d)", i);
	iconfig->shutdown=1;
}

void sighup_handler(int i)
{
	imsg("catched SIGHUP signal(%d)",i);
	iconfig->reload=1;
}

void sigusr1_handler(int i)
{
	imsg("catched SIGUSR1 signal(%d)",i);
	iconfig->usr1=1;
	
}

void sigusr2_handler(int i)
{
	imsg("catched SIGUSR2 signal(%d)",i);
	iconfig->usr2=1;
	
}



void signal_init()
{

	if( iconfig->reload && signal(SIGHUP, &sighup_handler)==SIG_ERR ) {
		emsg("error installing SIGHUP handler");			
    }
	iconfig->reload=0;	

	if( iconfig->shutdown ) {
		if (signal(SIGTERM,&sigterm_handler)==SIG_ERR )
			emsg( "error installing SIGTERM handler ");
		
		if (signal(SIGINT, &sigint_handler)==SIG_ERR ) 
			emsg( "error installing SIGINT handler ");

	}
	iconfig->shutdown=0;

	if( iconfig->usr1 && signal(SIGUSR1, &sigusr1_handler)==SIG_ERR ) {
		emsg("error installing SIGUSR1 handler");			
    }	
	iconfig->usr1=0;	

	if( iconfig->usr2 && signal(SIGUSR2, &sigusr2_handler)==SIG_ERR ) {
		emsg("error installing SIGUSR2 handler");			
    }
	iconfig->usr2=0;	


	return ;

}


void write_pidfile()
{
	FILE *fp;

	fp=fopen(iconfig->pidfile,"w+") ;

	if (fp ) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	} else {
		emsg("write pid file(%s) fail(%s)",iconfig->pidfile,strerror(errno));
	}

	return ;
}


int detach()
{
	int ret ;

	ret=fork();

	if (ret==0) {
		setsid();
	}
	else if  (ret>0) {
		exit(0);
	}
	else {
		printf("daemon fail!\n");
		return -1;
	}

	return 0;


}

int selflock(int unlock)
{
	int fd=-1;
	
	if ( unlock  ) {
		if ( fd != -1 ) {
			flock(fd, LOCK_UN );
			close(fd);
			unlink(iconfig->lockfile);
			fd=-1;
		}
		return 0;
	}

	if ((fd=open(iconfig->lockfile,O_CREAT|O_RDWR, 0666))==-1) {
		emsg("open lock file(%s) fail(%s)", iconfig->lockfile,strerror(errno));
		return -1;
	}

	if (flock(fd,LOCK_EX|LOCK_NB)!=0) {
		imsg("%s is already running!\n",iconfig->progname);
		close(fd);
		fd=-1;
		return -1;
	}

	return 0;

}



int init(char *name)
{
	char bf[PBFSZ];

	if (! iconfig->progname ) {
		xstrncpy(iconfig->namebf,name,NBFSZ);
		iconfig->progname=iconfig->namebf;
	}

	if ( ! iconfig->pidfile ){
		xsnprintf(iconfig->pidbf,PBFSZ,"/var/run/%s.pid",iconfig->progname);
		iconfig->pidfile=iconfig->pidbf;
	}

	if ( ! iconfig->lockfile ) {
		xsnprintf(iconfig->lkbf,PBFSZ,"/var/lock/%s.lock",iconfig->progname);
		iconfig->lockfile=iconfig->lkbf;
	}


	if ( iconfig->detach && detach() ) return -1;

	if ( iconfig->syslog && log_init() ) return -1;

	if ( iconfig->detach && iconfig->conlog ) iconfig->conlog=0;

	if (strcmp(iconfig->lockfile,"null") && selflock(0)) return -1;	

	if (strcmp(iconfig->pidfile,"null") ) write_pidfile();

	signal_init();

	dmsg("%s initial finish,memory %u",name, memuse());

	return 0;

}

void finit()
{
	dmsg("%s release, exit...memory %u",iconfig->progname ,memuse());

	if (strcmp(iconfig->pidfile,"null") ) unlink(iconfig->pidfile);

	selflock(1);	

	if (iconfig->detach ) log_free();

}


