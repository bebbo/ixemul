#ifndef _IXPROTOS_H_
#define _IXPROTOS_H_

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

struct WBStartup;

/* Miscellaneous prototypes */
void	ix_panic(const char *msg, ...);
void	panic(const char *msg, ...);
void	ix_warning(const char *msg, ...);
BPTR   *__load_seg(char *name, char **args);
void	__wait_sync_packet(struct StandardPacket *sp);
void	__wait_select_packet(struct StandardPacket *sp);
BPTR	__plock(const char *file_name, int (*last_func)(), void *last_arg);
void	close_libraries(void);
int	__ioerr_to_errno(int ioerr);
void	kfree(void *mem);
void   *kmalloc(size_t size);
void   *krealloc(void *mem, size_t size);
void	vfork_own_malloc(void);
void	__free_seg(BPTR *seg);
void	__ix_remove_sigwinch(void);
void	__ix_install_sigwinch(void);
void	ix_lock_base(void);
void	ix_unlock_base(void);
void	resetfpu(void);
void	all_free(void);
void	__ix_cli_parse(struct Process *this_proc, long alen, char *_aptr, int *argc, char ***argv);
void	_psignal(struct Task *t, int sig);
void	__Close(BPTR fh);
int	__get_file(struct file *f);
void	__release_file(struct file *f);
int	__tioctl(struct file *f, unsigned int cmd, unsigned int inout, unsigned int arglen, unsigned int arg);
BPTR	__lock(char *name, int mode);
BPTR	__llock(char *name, int mode);
int	__unlock(BPTR lock);
int	ix_sleep(caddr_t chan, char *wmesg);
void	setrun(struct Task *t);
int	falloc(struct file **resultfp, int *resultfd);
int	ufalloc(int want, int *result);
void	ix_wakeup(u_int waitchan);
int	__make_link(char *path, BPTR targ, int mode);
void	__init_stdinouterr(void);
void	init_buddy(void);
void	configure_context_switch(void);
void	siginit(struct user *p);
void	__init_std_packet(struct StandardPacket *sp);
int	issig(struct user *p);
void	_psignalgrp(struct Process *proc, int signal);
void   *b_alloc(int size, unsigned pool);
void    b_free(void *fb, int size);
int	__fstat(struct file *f);
void	sendsig(struct user *p, sig_t catcher, int sig, int mask, unsigned code, void *addr);
void    force_task_switch(void);
void	trapsignal(struct Task *t, int sig, unsigned code, void *addr);
char   *convert_dir(struct file *f, char *name, int omask);
void	timevaladd(struct timeval *t1, const struct timeval *t2);
int     __fioctl(struct file *f, unsigned int cmd, unsigned int inout, unsigned int arglen, unsigned int arg);
int	is_ixconfig(char *);
void	psig(struct user *p, int sig);
int	init_inet_daemon(int *argc, char ***argv);
void	shutdown_inet_daemon(void);
void	initstack(void);
void	freestack(void);
int	sigprocmask(int how, const sigset_t *mask, sigset_t *omask);
int	kill(pid_t pid, int signo);
void    set_dir_name_from_lock(BPTR lock);
void	_clean_longjmp(jmp_buf, int);
gid_t	getegid(void);
uid_t	geteuid(void);
struct Task *pfind(pid_t p);
void	proc_reparent(struct Process *child, struct Process *parent);
int	set_socket_stdio(int sock);
void    send_death_msg(struct user *mu);
void	stopped_process_handler(void);
int     sigblock(int);
int     sigsetmask(int);
sig_t	signal(int, sig_t);
pid_t	waitpid(pid_t, int *, int);
int	_getsockopt __P((struct file *, int, int, void *, int *));
void	_set_socket_params __P((struct file *, int, int, int));
struct ixemul_base *ix_init(struct ixemul_base *ixbase);
int     itimerdecr(struct itimerval *itp, int usec);
void    addupc(u_int, struct uprof *, int);
char  **dupvec(char **);
char   *_findenv(char **env, const char *name, int *offset);
long    fill_stat_mode(struct stat *stb, struct FileInfoBlock *fib);
int	getpriority(int, int);
int	setpriority(int, int, int);
void	__ix_close_muFS(struct user *ix_u);
ino_t	retrieve_ino(BPTR lock, struct InfoData *info, struct FileInfoBlock *fib);
int	is_pseudoterminal(char *name);
int	ioctl (int fd, unsigned long cmd, ...);
char   *basename(char *tmp);
int     filenamecmp(const char *fname);
UWORD   __amiga2unixid(UWORD id);
UWORD   __unix2amigaid(UWORD id);
void	shmexit(struct user *p);
int	shmfork(struct user *parent, struct user *child);
int     ipcperm(struct ucred *cred, struct ipc_perm *perm, int mode);
ino_t   get_unique_id(BPTR lock, void *fh);
struct ixemul_base *ix_init_glue(struct ixemul_base *ixbase);
void    seminit(void);
void	semexit(struct Task *p);
int     a2u(char *buf, char *src);

struct IORequest *ix_create_extio(struct MsgPort *ioReplyPort, long size);
struct MsgPort *ix_create_port(unsigned char *name, long pri);
struct Task *ix_create_task (unsigned char *name, long pri, void *initPC, u_long stackSize);
void    ix_delete_extio(struct IORequest *ioExt);
void    ix_delete_port (struct MsgPort *port);
void    ix_delete_task(struct Task *tc);

void    ixnewlist(struct ixlist *list);
void    ixaddtail(struct ixlist *list, struct ixnode *node);
void    ixaddhead(struct ixlist *list, struct ixnode *node);
void    ixremove(struct ixlist *list, struct ixnode *node);
void    ixinsert(struct ixlist *list, struct ixnode *node, struct ixnode *after);
struct ixnode *ixremhead(struct ixlist *list);

#endif
