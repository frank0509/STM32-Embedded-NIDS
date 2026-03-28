#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include "lwip/api.h"

void http_server_init(void);
void DynWebPage(struct netconn *conn);
void send_motor_status(struct netconn *conn);
void get_motor_status(struct netconn *conn, char *request_body);

#endif /* __HTTPSERVER_NETCONN_H__ */
