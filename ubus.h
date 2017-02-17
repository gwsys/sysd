#ifndef _UBUS_H
#define _UBUS_H

enum req_open_attr{
	R_OPEN_NAME,
	R_OPEN_CMD,
	R_OPEN_PIDFILE,
	R_OPEN_MODE,
	R_OPEN_NOTIFY,
	R_OPEN_MIN_RUN,
	R_OPEN_MAX_FAIL,
	R_OPEN_FAIL_SLEEP,
	R_OPEN_CHECK_TV,
	R_OPEN_WAIT,
	__R_OPEN_MAX	

};

enum req_close_attr{
	R_CLOSE_NAME,
	__R_CLOSE_MAX	

};

enum req_pause_attr{
	R_PAUSE_NAME,
	__R_PAUSE_MAX	

};

enum req_resume_attr{
	R_RESUME_NAME,
	R_RESUME_CMD,
	__R_RESUME_MAX	

};

enum req_get_attr{
	R_GET_NAME,
	__R_GET_MAX	

};



int ubus_init();
void ubus_finit();



#endif



