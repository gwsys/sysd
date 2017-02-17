#include "inc.h"
#include "initlib.h"
#include "ubus.h"


int sysinit()
{
	uloop_init();

	if ( ubus_init() ) return -1;

	return 0;
}

void sysfinit()
{

	ubus_finit();

	uloop_done();
}

void parse_cmd(int argc,char **argv)
{
	int i;
	
	for (i=1;i<argc; i++ ) {
		if (strcmp(argv[i],"-d" )==0 ) {
			iconfig->debug=1;
		}
		else if (strcmp(argv[i],"-D")==0 ) {
			iconfig->detach=0;
		}
		else if (strcmp(argv[i], "-s")==0 ) {
			iconfig->syslog=0;
		}
		else if (strcmp(argv[i], "-c")==0 ) {
			iconfig->conlog=1;
		}
		
	}
}


main(int argc, char *argv[])
{

	parse_cmd(argc,argv);
	iconfig->shutdown=0;

	if (init("systemd")==0 && sysinit()==0 ) {
		uloop_run();
		dmsg("exists...");
	}

	sysfinit();
	finit();
	return 0;

}




