#ifndef _UNP_H_
#define _UNP_H_

int unp_read   (struct file *fp, char *buf, int len);
int unp_write  (struct file *fp, const char *buf, int len);
int unp_ioctl  (struct file *fp, int cmd, int inout, int arglen, caddr_t data);
int unp_select (struct file *fp, int select_cmd, int io_mode, fd_set *ignored, u_long *also_ignored);
int unp_close  (struct file *fp);
int unp_socket (int domain, int type, int protocol, struct unix_socket *sock);
int unp_bind   (int s, const struct sockaddr *name, int namelen);
int unp_listen (int s, int backlog);
int unp_accept (int s, struct sockaddr *name, int *namelen);
int unp_connect(int s, const struct sockaddr *name, int namelen);
int unp_send   (int s, const void *buf, int len, int flags);
int unp_recv   (int s, void *buf, int len, int flags);
int unp_shutdown(int s, int how);
int unp_setsockopt(int s, int level, int name, const void *val, int valsize);
int unp_getsockopt(int s, int level, int name, void *val, int *valsize);
int unp_getsockname(int fdes, struct sockaddr *asa, int *alen);
int unp_getpeername(int fdes, struct sockaddr *asa, int *alen);
struct ix_unix_name *find_unix_name(const char *path);
struct sock_stream *init_stream(void);
struct sock_stream *find_stream(struct file *f, int read_stream);
struct sock_stream *get_stream(struct file *f, int read_stream);
void release_stream(struct sock_stream *ss);
int stream_read(struct file *f, char *buf, int len);
int stream_write(struct file *f, const char *buf, int len);

#endif
