#ifndef NET_HANDLER_H
#define NET_HANDLER_H

#include <netinet/in.h>

int  nh_server_init(int port);
int  nh_send_to(int sock, const char *msg, struct sockaddr_in *client_addr);
int  nh_recv_from(int sock, char *buf, int buf_len, struct sockaddr_in *client_addr);
void nh_close(int sock);

#endif // NET_HANDLER_H
