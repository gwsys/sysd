#ifndef _SERVICE_H
#define _SERVICE_H

typedef struct service_control{
	struct list_head list;
	struct uloop_process proc;
	struct uloop_timeout timeout;
	char name[NBFSZ];
	char cmd[CBFSZ];
	char *cargs[64];
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
} svrctrl;

enum svr_stat_atr{
	S_RUN,
	S_START,
	S_FAIL,
	S_PAUSE,
	S_STOP,
};


enum svr_mode_attr{
	M_NORMAL,
	M_SERVICE,
	M_ONESHOT
};


int handle_rsp(struct ubus_context *ctx, struct ubus_request_data *req, bool success, char *msg);
int handle_cmd_open(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb,bool rsp);
int handle_cmd_close(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp);
int handle_cmd_resume(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp);
int handle_cmd_pause(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp);
int handle_cmd_get(struct ubus_context *ctx, struct ubus_request_data *req, struct blob_attr **tb, bool rsp);
 
#endif



