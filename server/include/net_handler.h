#ifndef NET_HANDLER_H
#define NET_HANDLER_H

int  nh_server_init(int port);
int  nh_server_accept(int server_fd);
int  nh_client_connect(const char *ip, int port);
int  nh_send_line(int fd, const char *msg);
int  nh_recv_line(int fd, char *buf, int buf_len);
void nh_close(int fd);

#endif // NET_HANDLER_H
