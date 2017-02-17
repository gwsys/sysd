#include "inc.h"
#include "initlib.h"
#include "service.h"
#include "ubus.h"

static struct blob_buf b;

static LIST_HEAD(services);

int parse_args(char *line, char **args)
{
	char *p1,*p2;
	int i;
	int len=strlen(line);
	int cnt=0;
	int status=0;

	for (i=0;i<=len;i++) {
		if (status==0) { //space
			if (line[i]==0 || isspace(line[i])) continue;

			if (line[i]=='"' ) {
				status=2;
				p1=line+i+1;
				p2=line+i+1;
			} else {
				status=1;
				p1=line+i;
			}
			continue;
		}

		if (status==1 ) { //normal
			if ( line[i]==0 ) {
				args[cnt++]=p1;
				status=0;
			}
			else if ( isspace(line[i]) ) {
				line[i]=0;
				args[cnt++]=p1;
				status=0;
			}

			continue;
		}

		if (status==2  ) { //quota 
			
			if (line[i]=='\\' ) {
				status=3;
				continue;
			}
			else if (line[i]=='"'|| line[i]==0 ) {
				*p2=0;
				args[cnt++]=p1;
				status=0;
			}
			else {
				*p2++=line[i];
			}
		}

		if (status==3) { //slash in quota
			if (line[i]=='s') *p2++=' ';
			else if (line[i]=='t') *p2++='\t';
			else if (line[i]==0 ) {
				*p2=0;
				args[cnt++]=p1;
			}
			else {
				*p2++=line[i];
			}
			status=2;
		}
	}

	args[cnt]=NULL;
	
	return cnt;

	
}

int run_cmd(char **args, bool bg, char **envp)
{
	pid_t pid; 
	int fd=-1;
	int ret;

	if (iconfig->debug ) {
		char cmd[CBFSZ];
		cmd[0]=0;
		int i;
		for (i=0; args[i] ; i++ ) {
			strcat(cmd,args[i]);
			strcat(cmd," ");
		}

		dmsg("run cmd: %s",cmd);
	}

	if((pid = fork())<0){
		emsg("fork command process fail");
		return -1;
	}
	else if(pid == 0){
		if (bg ) setsid();
		
		fd=open("/dev/null",O_WRONLY);
		if ( fd>=0) {
			if (fd!=STDOUT_FILENO ) {
				dup2(fd,STDOUT_FILENO);
			}
			if (fd!=STDERR_FILENO ) {
				dup2(fd,STDERR_FILENO);
			}
			if (fd!=STDOUT_FILENO && fd!=STDERR_FILENO ) {
				close(fd);
			}
		}
		if ( envp ) execvpe(args[0],args,envp );
		else execvp(args[0],args);
		emsg("run command fail(%s)",strerror(errno));
		_exit(127);
    }

	return pid;
	
}

static time_t gettime() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec;
}

int svr_check_pid(pid_t pid)
{
	if (kill(pid,0) ) return -1;

	return 0;

}

int svr_check_pidfile(svrctrl *svr)
{
	char bf[128];
	FILE *fp;
	pid_t pid;
	if (svr->mode!=M_SERVICE || svr->pidfile[0]==0 ) return 0;

	fp=fopen(svr->pidfile,"r") ;
	if ( !fp ) {
		dmsg("service %s can't open pidfile %s",svr->name, svr->pidfile);
		return -1;
	}

	if ( ! fgets(bf,128,fp) ) {
		fclose(fp);
		dmsg("Can't read %s pid from file %s",svr->name, svr->pidfile);
		return -2;
	}

	bf[127]=0;
	fclose(fp);

	pid=atoi(bf);

	if (pid<=0) {
		dmsg("Can't get %s pid from file %s",svr->name, svr->pidfile);
		return -3;
	}

	if (svr_check_pid(pid)) {
		dmsg("service %s false pidfile %s pid %u",svr->name,svr->pidfile, pid);
		unlink(svr->pidfile);
		return -4;
	}


	dmsg("check service %s pid %u",svr->name, pid);

	return pid;
	
}

void svr_send_event(svrctrl *svr, char *event, char *msg)
{
	char *envp[4];
	char bf1[NBFSZ],bf2[NBFSZ],bf3[CBFSZ];
	dmsg("send event to %s %s:%s", svr->name,event,msg);
/*
	blob_buf_init(&b, 0);

	blobmsg_add_string(&b,"name",svr->name);
	blobmsg_add_string(&b,"event",event);
	if (msg) blobmsg_add_string(&b,"msg",msg);

	ubus_send_event(ctx, svr->name, b.head);	
*/
	if (svr->notify[0] ) {
		xsnprintf(bf1,NBFSZ,"SYSTEMD_EVENT_OBJ=%s",svr->name);
		xsnprintf(bf2,NBFSZ,"SYSTEMD_EVENT_TYPE=%s",event);
		envp[0]=bf1;
		envp[1]=bf2;
		if (msg ){
			xsnprintf(bf3,CBFSZ,"SYSTEMD_EVENT_MSG=%s",msg);
			envp[2]=bf3;
			envp[3]=NULL;
		} else {
			envp[2]=NULL;
		}

		dmsg("service %s run notify cmd",svr->name);
		run_cmd(svr->nargs,true,envp);
		
	}

	return ;

}

void svr_pause(svrctrl *svr)
{
	int ret;

	dmsg("pause service %s",svr->name);
	
	if (svr->pid>0 ){
		dmsg("kill %s pid %u",svr->name,svr->pid);
		kill(svr->pid,SIGTERM ) ;
		svr->pid=0;
	}

	if (svr->mode==M_SERVICE ) {
		if (svr->runpid>0 ){
			dmsg("kill background service %s runpid %u",svr->name, svr->runpid);
			kill(svr->runpid, SIGTERM );
			svr->runpid=0;
		} else {
			ret=svr_check_pidfile(svr);
			if (ret>0 ) {
				dmsg("kill background service %s runpid %u",svr->name, ret);				
				kill(ret, SIGTERM );			
			}
		}
	}

	uloop_process_delete(&svr->proc);
	uloop_timeout_cancel(&svr->timeout);

	svr->status=S_PAUSE;

	imsg("pause service %s(%d)",svr->name,svr->mode);

}




void svr_close(svrctrl *svr) 
{
	int ret;

	dmsg("close service %s",svr->name);
	
	if (svr->pid>0 ){
		dmsg("kill %s pid %u",svr->name,svr->pid);
		kill(svr->pid,SIGTERM ) ;
	}

	if (svr->mode==M_SERVICE ) {
		if (svr->runpid>0 ){
			dmsg("kill background service %s runpid %u",svr->name, svr->runpid);
			kill(svr->runpid, SIGTERM );
		} else {
			ret=svr_check_pidfile(svr);
			if (ret>0 ) {
				dmsg("kill background service %s runpid %u",svr->name, ret);				
				kill(ret, SIGTERM );			
			}
		}
	}

	uloop_process_delete(&svr->proc);
	uloop_timeout_cancel(&svr->timeout);

	list_del(&svr->list);

	imsg("close service %s(%d)",svr->name,svr->mode);
	xfree(svr);

	
}

int svr_close_byname(char *name)
{
	svrctrl *svr;

	list_for_each_entry(svr,&services,list) {
		if (strcmp(svr->name,name)==0 ) {
			svr_close(svr);
			return 0;
		}
	}

	return -1;
}

void svr_close_all()
{
	svrctrl *svr, *tmp;

	dmsg("close all services");

	list_for_each_entry_safe(svr,tmp,&services,list) {
		svr_close(svr);
	}
}

int svr_resume(svrctrl *svr)
{
	if (svr->status!=S_PAUSE ) return -1;
	
	int ret=run_cmd(svr->cargs,false,NULL);

	if (ret>0 ) {
		svr->pid=ret;
		svr->last_run=gettime();
		if (svr->mode==M_SERVICE) {
			svr->status=S_START;
		} else {
			svr->status=S_RUN;
		}

		svr->proc.pid=ret;		

		uloop_process_add(&svr->proc);

		imsg("resume service %s",svr->name);
		return 0;
	}

	svr->status=S_FAIL;

	uloop_timeout_set(&svr->timeout, svr->fail_sleep*1000);

	emsg("start service %s fail",svr->name);

	return -1;


}


int svr_start(svrctrl *svr)
{
	int ret=run_cmd(svr->cargs,false,NULL);

	if (ret>0 ) {
		svr->pid=ret;
		svr->last_run=gettime();
		if (svr->mode==M_SERVICE) {
			svr->status=S_START;
		} else {
			svr->status=S_RUN;
		}

		svr->proc.pid=ret;		

		uloop_process_add(&svr->proc);

		imsg("start service %s",svr->name);
		return 0;
	}

	svr->status=S_FAIL;

	uloop_timeout_set(&svr->timeout, svr->fail_sleep*1000);

	emsg("start service %s fail",svr->name);

	return -1;
}

svrctrl *svr_find(char *name)
{
	svrctrl *svr;

	list_for_each_entry(svr,&services,list) {
		if (strcmp(svr->name,name)==0 ) {
			return svr;
		}
	}

	return NULL;

}

char *svr_status(svrctrl *svr)
{
	int status;

	if ( ! svr ) return "stop";
	
	status=svr->status;
	if (status==S_START ) return "start";
	else if (status==S_RUN ) return "run";
	else if (status==S_FAIL ) return "fail";
	else if (status==S_PAUSE ) return "pause";

	return "unknown";
}

char *svr_mode(svrctrl *svr)
{
	int mode;

	if ( ! svr ) return "none";
	
	mode=svr->status;
	if (mode==M_NORMAL ) return "normal";
	else if (mode=M_SERVICE) return "service";
	else if (mode==M_ONESHOT) return "oneshot";

	return "unknown";
}


int svr_get(svrctrl *svr, struct blob_buf *b)
{
	char notify[CBFSZ];
	char *nargs[64];
	char pidfile[PBFSZ];
	int mode;
	int min_run;
	int max_fail;
	int fail_sleep;	
	int check_tv;
	pid_t pid;
	pid_t runpid;
	int status;
	time_t last_run;
	time_t last_fail;
	int fail;


	if ( ! svr ) return -1;
	blobmsg_add_string(b,"name",svr->name);
	if (svr->cmd[0]) blobmsg_add_string(b,"cmd",svr->cmd); 
	if (svr->notify[0]) blobmsg_add_string(b,"notify",svr->name); 	
	blobmsg_add_string(b,"mode",svr_mode(svr));
	blobmsg_add_u32(b,"min_run",svr->min_run);	
	blobmsg_add_u32(b,"max_fail",svr->max_fail);	
	blobmsg_add_u32(b,"fail_sleep",svr->fail_sleep);	
	blobmsg_add_u32(b,"check_tv",svr->check_tv);
	blobmsg_add_u32(b,"pid",svr->pid);
	blobmsg_add_u32(b,"runpid",svr->runpid);
	blobmsg_add_string(b,"mode",svr_mode(svr));
	blobmsg_add_string(b,"status",svr_status(svr));	
	blobmsg_add_u32(b,"last_run",svr->last_run);
	blobmsg_add_u32(b,"last_fail",svr->last_fail);
	blobmsg_add_u32(b,"fail",svr->fail);
	return 0;


}




void svr_timeout_handler(struct uloop_timeout *t)
{
	int ret;
	svrctrl *svr=container_of(t, svrctrl , timeout);

	if ( svr->status==S_FAIL ) {
		svr_start(svr);
	}
	else if (svr->status==S_START && svr->mode==M_SERVICE ) {

		ret=svr_check_pidfile(svr);

		if (ret>0){
			svr->runpid=ret;
			svr->status=S_RUN;
			dmsg("background service %s run successful",svr->name);
			uloop_timeout_set(&svr->timeout,svr->check_tv*1000);
		} else {
			emsg("background service %s run fail",svr->name);
			svr_send_event(svr,"error",NULL);
			svr_close(svr);
		}

	}
	else if (svr->status==S_RUN && svr->mode==M_SERVICE ) {
		if (svr->runpid >0 ) {
			if (svr_check_pid(svr->runpid) ) {
				svr->runpid=0;
				svr->status=S_FAIL;
				svr->last_fail=gettime();
				emsg("background service %s fail",svr->name);
				svr_send_event(svr,"fail",NULL);
				uloop_timeout_set(&svr->timeout,svr->fail_sleep*1000);
			} else {
				dmsg("check background service %s success",svr->name);
				uloop_timeout_set(&svr->timeout,svr->check_tv*1000);
			}
		}
	}

	return ;
	
}


void svr_proc_handler(struct uloop_process *c, int flag)
{
	time_t now;
	svrctrl *svr=container_of(c, svrctrl , proc);
	int ret;

	dmsg("service %s process %u down",svr->name, svr->pid);
	
	if (svr->status==S_START) {
		svr->pid=0;
		dmsg("wait 5s to check background service %s",svr->name);
		uloop_timeout_set(&svr->timeout,5000);
		return ;
	}

	if (svr->mode==M_ONESHOT ) {
		svr_send_event(svr,"finish",NULL);
		svr_close(svr);
		return ;
	}

	now=gettime();

	if ( svr->min_run ) {
		if (now-svr->last_run < svr->min_run ) svr->fail++;
		else svr->fail=0;

		if (svr->max_fail && svr->fail ==svr->max_fail ) {
			imsg("service %s run fail for %d times",svr->name,svr->max_fail);
			svr_send_event(svr,"fail",NULL);
		}
	} else {
		svr_send_event(svr,"fail",NULL);
	}

	svr->status=S_FAIL;
	svr->last_fail=now;
	svr->pid=0;

	uloop_timeout_set(&svr->timeout,svr->fail_sleep*1000);

	return ;

}




int handle_rsp(struct ubus_context *ctx, struct ubus_request_data *req, bool success, char *msg)
{
	blob_buf_init(&b, 0);
	if (success) blobmsg_add_string(&b, "ret", "success");
	else blobmsg_add_string(&b, "ret", "fail");
	if (msg) blobmsg_add_string(&b, "msg", msg);
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int handle_cmd_open(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb,bool rsp)
{
	svrctrl* svr;
	char *value,*cmd;
	int wait,val;
	
	char *name=tb[R_OPEN_NAME]?blobmsg_get_string(tb[R_OPEN_NAME]):NULL;
	if ( !name ) {
		emsg("no name");
		if (rsp) handle_rsp(ctx,req,false,"invalid name parameter");
		return 0;
	}
	if ( name && svr_find(name) )  {
		emsg("service %s already exists",name);
		if (rsp) handle_rsp(ctx,req,false,"service already exists");
		return 0;
	}

	wait=tb[R_OPEN_WAIT]?blobmsg_get_bool(tb[R_OPEN_CMD]):false;
	cmd=tb[R_OPEN_CMD]?blobmsg_get_string(tb[R_OPEN_CMD]):NULL;
	if ( ! wait	&& ( !cmd || strlen(cmd)==0 ) ) {
		emsg("invalid command");
		if (rsp) handle_rsp(ctx,req,false,"invalid command parameter"); 	
		return 0;
	}

	svr=(svrctrl*)xmalloc_and_clean(sizeof(svrctrl));
	xstrncpy(svr->name,name,NBFSZ);
	if (cmd )xstrncpy(svr->cmd,cmd,CBFSZ);	
	
	if ( tb[R_OPEN_NOTIFY] ) xstrncpy(svr->notify,blobmsg_get_string(tb[R_OPEN_NOTIFY]),CBFSZ);
	if ( tb[R_OPEN_PIDFILE] ) xstrncpy(svr->pidfile,blobmsg_get_string(tb[R_OPEN_PIDFILE]),PBFSZ);
	
	value=tb[R_OPEN_MODE]?blobmsg_get_string(tb[R_OPEN_MODE]):NULL;
			
	if (value && strcmp(value,"service")==0) svr->mode=M_SERVICE;
	else if ( value && strcmp(value,"oneshot")==0 ) svr->mode=M_ONESHOT;
	else svr->mode=M_NORMAL;
	
	if ( svr->mode==M_SERVICE && svr->pidfile[0]==0 ) {
		emsg("start %s fail: invalid pidfile parameters",svr->name) ;
		if (rsp )handle_rsp(ctx,req,false,"invalid pidfile parameter");
		goto release_svr;
	}
	
	if (tb[R_OPEN_MIN_RUN] ) {
		val=blobmsg_get_u32(tb[R_OPEN_MIN_RUN]);
		if (val==0 || val>=60 ) svr->min_run=val;
	}
		
	if (tb[R_OPEN_MAX_FAIL] ) {
		val=blobmsg_get_u32(tb[R_OPEN_MAX_FAIL]);
		if (val>=0 ) svr->max_fail=val;
	}
	
	svr->fail_sleep=15;
	if (tb[R_OPEN_FAIL_SLEEP] ) {
		val=blobmsg_get_u32(tb[R_OPEN_FAIL_SLEEP]);
		if (val>=10 ) svr->fail_sleep=val;
	}
	
	svr->check_tv=60;	
	if (tb[R_OPEN_CHECK_TV] ) {
		val=blobmsg_get_u32(tb[R_OPEN_CHECK_TV]);
		if (val>=30) svr->check_tv=val;
	}
				
	if (svr->cmd[0] ) parse_args(svr->cmd,svr->cargs);
	if (svr->notify[0] ) parse_args(svr->notify,svr->nargs);
	
	INIT_LIST_HEAD(&svr->list);
	INIT_LIST_HEAD(&svr->proc.list);
	INIT_LIST_HEAD(&svr->timeout.list);
			
	svr->proc.cb=svr_proc_handler;
	svr->timeout.cb=svr_timeout_handler;
		
	list_add(&svr->list,&services);
		
	if ( !wait ) {
		svr_start(svr);
	} else {
		svr->status=S_PAUSE;
		dmsg("add service %s, wait to resume",svr->name);
	}
	
	if (rsp) handle_rsp(ctx,req,true,NULL);
	
	return 0;
	
release_svr:
	xfree(svr);	
	return 0;
}

int handle_cmd_close(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp)
{
	svrctrl* svr;
	
	char *name=tb[R_CLOSE_NAME]?blobmsg_get_string(tb[R_CLOSE_NAME]):NULL;
	if ( !name ) {
		emsg("no name");
		if (rsp) handle_rsp(ctx,req,false,"invalid name parameter");
		return 0;
	}
	if ( !(svr=svr_find(name)) )  {
		emsg("service %s doesn't exists",name);
		if (rsp) handle_rsp(ctx,req,false,"service doesn't exists");
		return 0;
	}

	svr_close(svr);

	if (rsp) handle_rsp(ctx,req,true,NULL);

	return 0;
}

int handle_cmd_resume(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp)
{
	svrctrl* svr;
	
	char *name=tb[R_RESUME_NAME]?blobmsg_get_string(tb[R_RESUME_NAME]):NULL;
	if ( !name ) {
		emsg("no name");
		if (rsp) handle_rsp(ctx,req,false,"invalid name parameter");
		return 0;
	}
	if ( !(svr=svr_find(name)) )  {
		emsg("service %s doesn't exists",name);
		if (rsp) handle_rsp(ctx,req,false,"service doesn't exists");
		return 0;
	}

	svr_resume(svr);

	if (rsp) handle_rsp(ctx,req,true,NULL);

	return 0;

}

int handle_cmd_pause(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp)
{
	svrctrl* svr;
	
	char *name=tb[R_PAUSE_NAME]?blobmsg_get_string(tb[R_PAUSE_NAME]):NULL;
	if ( !name ) {
		emsg("no name");
		if (rsp) handle_rsp(ctx,req,false,"invalid name parameter");
		return 0;
	}
	if ( !(svr=svr_find(name)) )  {
		emsg("service %s doesn't exists",name);
		if (rsp) handle_rsp(ctx,req,false,"service doesn't exists");
		return 0;
	}

	svr_pause(svr);

	if (rsp) handle_rsp(ctx,req,true,NULL);

	return 0;
}

int handle_cmd_get(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb,bool rsp)
{
	svrctrl* svr;
	void *tbl,*array;	

	blob_buf_init(&b, 0);

	char *name=tb[R_PAUSE_NAME]?blobmsg_get_string(tb[R_PAUSE_NAME]):NULL;
	if ( name ) {
		if ( !(svr=svr_find(name)) )  {
			emsg("service %s doesn't exists",name);
			if (rsp) handle_rsp(ctx,req,false,"service doesn't exists");
			return 0;
		}


		tbl=blobmsg_open_table(&b, "param");
		
		if (svr_get(svr,&b)==0 ) {
			blobmsg_close_table(&b,tbl);
			blobmsg_add_string(&b, "ret", "success");
			ubus_send_reply(ctx, req, b.head);		
		} else {
			blobmsg_close_table(&b,tbl);
			handle_rsp(ctx,req,false,NULL);
		}
		return 0;
	}

	array=blobmsg_open_array(&b,"param");	
	list_for_each_entry(svr,&services,list) {
		tbl=blobmsg_open_table(&b,NULL);
		svr_get(svr,&b);
		blobmsg_close_table(&b,tbl);
	}
	blobmsg_close_array(&b,array);

	blobmsg_add_string(&b, "ret", "success");
	ubus_send_reply(ctx, req, b.head);		

	
	return 0;
}



