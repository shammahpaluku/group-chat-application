#ifndef NET_HANDLER_H
#define NET_HANDLER_H

#include <netinet/in.h>

int  nh_client_init(const char *server_ip, int port, struct sockaddr_in *server_addr);
int  nh_send_to(int sock, const char *msg, struct sockaddr_in *server_addr);
int  nh_recv_from(int sock, char *buf, int buf_len);
void nh_close(int sock);

#endif // NET_HANDLER_H
