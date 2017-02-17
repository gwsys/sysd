#ifndef _INC_H
#define _INC_H

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <signal.h>
#include <syslog.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


#include <glob.h>

#include <libubox/blobmsg_json.h>
#include <libubox/list.h>
#include <libubus.h>
#include <libubox/avl.h>
#include <libubox/avl-cmp.h>






#define SBFSZ 64
#define BFSZ 1024
#define HBFSZ 4096
#define NBFSZ 128
#define PBFSZ 256
#define CBFSZ 512


#endif

