#include "inc.h"
#include "initlib.h"
#include "service.h"
#include "ubus.h"


static struct blob_buf b;



static const struct blobmsg_policy open_policy[] = {
	[R_OPEN_NAME] ={ .name = "name", .type=BLOBMSG_TYPE_STRING },
	[R_OPEN_CMD] ={ .name = "cmd", .type=BLOBMSG_TYPE_STRING },
	[R_OPEN_PIDFILE] ={ .name = "pidfile", .type=BLOBMSG_TYPE_STRING },
	[R_OPEN_MODE] ={ .name = "mode", .type=BLOBMSG_TYPE_STRING },
	[R_OPEN_NOTIFY] ={ .name = "notify", .type=BLOBMSG_TYPE_STRING },	
	[R_OPEN_MIN_RUN] ={ .name = "min_run", .type=BLOBMSG_TYPE_INT32 },
	[R_OPEN_MAX_FAIL] ={ .name = "max_fail", .type=BLOBMSG_TYPE_INT32 },
	[R_OPEN_FAIL_SLEEP] ={ .name = "fail_sleep", .type=BLOBMSG_TYPE_INT32 },
	[R_OPEN_CHECK_TV] ={ .name = "check_tv", .type=BLOBMSG_TYPE_INT32 },	
	[R_OPEN_WAIT] ={ .name = "wait", .type=BLOBMSG_TYPE_BOOL },		
};

static const struct blobmsg_policy close_policy[] = {
	[R_CLOSE_NAME] ={ .name = "name", .type=BLOBMSG_TYPE_STRING },
};

static const struct blobmsg_policy pause_policy[] = {
	[R_PAUSE_NAME] ={ .name = "name", .type=BLOBMSG_TYPE_STRING },
};


static const struct blobmsg_policy resume_policy[] = {
	[R_RESUME_NAME] ={ .name = "name", .type=BLOBMSG_TYPE_STRING },
	[R_RESUME_CMD] ={ .name = "cmd", .type=BLOBMSG_TYPE_STRING },		
};

static const struct blobmsg_policy get_policy[] = {
	[R_GET_NAME] ={ .name = "name", .type=BLOBMSG_TYPE_STRING },
};


static int handle_req(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__R_OPEN_MAX];
	char bf[PBFSZ];

	dmsg("handle request %s",method);

	if ( strcmp(method,"open")==0 ) {
		blobmsg_parse(open_policy, ARRAY_SIZE(open_policy), tb, blob_data(msg), blob_len(msg));
		handle_cmd_open(ctx,req,tb,true);
	}
	else if (strcmp(method,"resume")==0 ) {
		blobmsg_parse(resume_policy, ARRAY_SIZE(resume_policy), tb, blob_data(msg), blob_len(msg)); 
		handle_cmd_resume(ctx,req,tb,true);
	}
	else if (strcmp(method,"pause")==0 ) {
		blobmsg_parse(pause_policy, ARRAY_SIZE(pause_policy), tb, blob_data(msg), blob_len(msg));
		handle_cmd_pause(ctx,req,tb,true);
	}
	else if (strcmp(method,"close")==0 ) {
		blobmsg_parse(close_policy, ARRAY_SIZE(close_policy), tb, blob_data(msg), blob_len(msg)); 
		handle_cmd_close(ctx,req,tb,true);
	}
	else if (strcmp(method,"get")==0 ) {
		blobmsg_parse(get_policy, ARRAY_SIZE(get_policy), tb, blob_data(msg), blob_len(msg)); 
		handle_cmd_get(ctx,req,tb,true);
	}
	else if (strcmp(method,"exit")==0 ) {
		uloop_cancelled=true;
	}		
	else {
		handle_rsp(ctx,req,false,"unknown request");
		return 0;
	}

	
	return 0;

}

static void handle_event(struct ubus_context *ctx, struct ubus_event_handler *ev,
			  const char *type, struct blob_attr *msg)
{
	struct blob_attr *tb[__R_OPEN_MAX];
	char *method;
	
	dmsg("handle event %s",type);

	if (strncmp(type,"systemd-",8) ) {
		emsg("invalid event type %s",type);
		return ;
	}
	method=type+8;

	if ( strcmp(method,"open")==0 ) {
		blobmsg_parse(open_policy, ARRAY_SIZE(open_policy), tb, blob_data(msg), blob_len(msg));
		handle_cmd_open(ctx,NULL,tb,false);
	}
	else if (strcmp(method,"resume")==0 ) {
		blobmsg_parse(resume_policy, ARRAY_SIZE(resume_policy), tb, blob_data(msg), blob_len(msg)); 
		handle_cmd_resume(ctx,NULL,tb,false);
	}
	else if (strcmp(method,"pause")==0 ) {
		blobmsg_parse(pause_policy, ARRAY_SIZE(pause_policy), tb, blob_data(msg), blob_len(msg));
		handle_cmd_pause(ctx,NULL,tb,false);
	}
	else if (strcmp(method,"close")==0 ) {
		blobmsg_parse(close_policy, ARRAY_SIZE(close_policy), tb, blob_data(msg), blob_len(msg)); 
		handle_cmd_close(ctx,NULL,tb,false);
	}
	else if (strcmp(method,"exit")==0 ) {
		uloop_cancelled=true;
	}		
	else {
		imsg("unknown event");
	}

	
	return ;


}



static const struct ubus_method systemd_methods[] = {
	UBUS_METHOD("open", handle_req, open_policy),
	UBUS_METHOD("close", handle_req, close_policy),	
	UBUS_METHOD("resume", handle_req, resume_policy),
	UBUS_METHOD("pause", handle_req, pause_policy),
	UBUS_METHOD("get", handle_req, get_policy),
	UBUS_METHOD_NOARG("exit", handle_req),
};

static struct ubus_object_type systemd_object_type =
	UBUS_OBJECT_TYPE("system", systemd_methods);



static struct ubus_object systemd_object = {
	.name = "systemd",
	.type = &systemd_object_type,
	.methods = systemd_methods,
	.n_methods = ARRAY_SIZE(systemd_methods),
};

static struct ubus_context *ctx=NULL;
static struct ubus_event_handler listener;	
static bool objreg=false, eventreg=false;



int ubus_init()
{
	int ret;

	dmsg("ubus_init");
	
	
	ctx = ubus_connect(NULL);
	if (!ctx) {
		emsg("Fail to connect to ubus");
		return -1;
	}

	ubus_add_uloop(ctx);

	dmsg ("add ctx to uloop");

	ret=ubus_add_object(ctx, &systemd_object);
	if (ret) {
		emsg("Failed to add object: %s", ubus_strerror(ret));
		return -2;
	}
	objreg=true;


	memset(&listener, 0, sizeof(listener));
	listener.cb = handle_event;

	ret= ubus_register_event_handler(ctx, &listener, "systemd-*");	
	if ( ret) {
		emsg("Failed to listen event");
		return -3;
	}
	eventreg=true;
	

	dmsg("add systemd object");


	return 0;

}

void ubus_finit()
{

	if ( objreg) ubus_remove_object(ctx, &systemd_object);

	if (eventreg) ubus_unregister_event_handler(ctx,&listener);

	if (ctx) ubus_free(ctx);


}

