/*
 * interpose.c
 * interpose on syscalls.  do ipc to server (kernel) 
 * this is ugly, riddled with #ifdefs because two different
 * interposition methods are used for gnu ld vs. solaris ld
 * (relinking three times instead of using a Mapfile)
 *
 * hacked up for uthreads
 * hacked up for linux's way of doing dlsym.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <dirent.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "uthread.h"
#include "uthread_sched.h"

//#if defined(__linux__) && defined(__i386)
#define mangle_name(linname, solname) ipc_##linname
#define INTERPOSE_NEED_DLOPEN
int     ipc_getdents(int fd, struct dirent *buf, const size_t count);
int     ipc_access(const char *file, int mode);
int     ipc_chdir(const char *file);
int     ipc_fchdir(int fd);
int     ipc_close(int fd);
int     ipc_open(const char *pathname, int flags, ...);
pid_t   ipc_fork(void);
off_t   ipc_lseek(int fd, off_t offset, int whence);
ssize_t ipc_write(int fd, const void *buf, size_t count);
ssize_t ipc_read(int fd, void* buf, const size_t count);
void    ipc_perror(const char *s);

/* need to define some pragmas to get these functions called. */
#pragma weak read=ipc_read
#pragma weak write=ipc_write
#pragma weak open=ipc_open
#pragma weak close=ipc_close
#pragma weak lseek=ipc_lseek
#pragma weak chdir=ipc_chdir
#pragma weak fchdir=ipc_fchdir
#pragma weak access=ipc_access
#pragma weak getdents=ipc_getdents
#pragma weak fork=ipc_fork
#pragma weak perror=ipc_perror

#pragma weak __read=ipc_read
#pragma weak __write=ipc_write
#pragma weak __open=ipc_open
#pragma weak __close=ipc_close
#pragma weak __lseek=ipc_lseek
#pragma weak __chdir=ipc_chdir
#pragma weak __fchdir=ipc_fchdir
#pragma weak __access=ipc_access
#pragma weak __getdents=ipc_getdents
#pragma weak __fork=ipc_fork
#pragma weak __perror=ipc_perror
/*
 int     getdents(int fd, struct dirent *buf, const size_t count) {
          return ipc_getdents( fd, buf, count ); }
*/
//#endif

#if defined(__SVR4) && defined(__sparc)
#define mangle_name(linname, solname) _##solname
#pragma weak read=_read
#pragma weak write=_write
#pragma weak open=__open
#pragma weak close=_close
#pragma weak lseek=_lseek
#pragma weak chdir=_chdir
#pragma weak fchdir=_fchdir
#pragma weak access=_access
#pragma weak getdents=_getdents
#pragma weak fork=__libc_fork
#pragma weak perror=_perror
#endif

#if defined _WTHREADS
#define mangle_thr_id   wthread_self()
#else
#define mangle_thr_id   0
#endif

void
mangle_name(perror, perror)
(const char *s)
{
	/* fprintf will reschedule by calling write */
	fprintf(stderr, "%s: %s", s, strerror(errno));
}

int
mangle_name(access, access)
(const char *file, int mode)
{
	typedef	int (*access_func)(const char *, int);
	access_func	real_access;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif   
    
	uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
    real_access = (access_func)dlsym(libhandle, "access");
#else
    real_access = (access_func)dlsym(RTLD_NEXT, "access");
#endif    
    
    ret = real_access(file, mode);
    
#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
    
    return ret;
}

int 
mangle_name(chdir, chdir)
(const char *file)
{
	typedef	int	(*chdir_func)(const char *file);
	chdir_func	real_chdir;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif   

    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_chdir = (chdir_func)dlsym(libhandle, "chdir");
#else
    real_chdir = (chdir_func)dlsym(RTLD_NEXT, "chdir");
#endif

    ret = real_chdir(file); 
    
#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
    
    return ret;
}

int 
mangle_name(fchdir, fchdir)
(int fd)
{
	typedef	int	(*fchdir_func)(int fd);
	fchdir_func	real_fchdir;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif   

    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_fchdir = (fchdir_func)dlsym(libhandle, "chdir");
#else
    real_fchdir = (fchdir_func)dlsym(RTLD_NEXT, "chdir");
#endif    
    
    ret = real_fchdir(fd); 

#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
    
    return ret; 
}

pid_t
mangle_name(fork, _libc_fork)
(void)
{
	typedef pid_t (*fork_func)(void);
	fork_func	real_fork;
	int			child;

#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_fork = (fork_func)dlsym(libhandle, "fork");
#else
    real_fork = (fork_func)dlsym(RTLD_NEXT, "fork");
#endif    
	
    if ((child = real_fork()) == 0)
	{
		/* child process -- we need to
		 * reset everything or just let it
		 * go the way it is...
		 * XXX figure out the right thing to do.
		 */
	}

#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
	
    return child;
}

ssize_t
mangle_name(write, write)
(int fd, const void *buf, size_t count)
{
	typedef	int	(*write_func)(int fd, const void *buf, size_t count);
    write_func real_write;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif
   
    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_write = (write_func)dlsym(libhandle, "write");
#else
    real_write = (write_func)dlsym(RTLD_NEXT, "write");
#endif    

    ret = real_write(fd,buf,count);
	
#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
    
    return ret;
}

off_t
mangle_name(lseek, lseek)
(int fd, off_t offset, int whence)
{
	typedef int	(*lseek_func)(int fd, off_t offset, int whence);
	lseek_func	real_lseek;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif
	
    uthread_yield();
    
#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_lseek = (lseek_func)dlsym(libhandle, "lseek");
#else
	real_lseek = (lseek_func)dlsym(RTLD_NEXT, "lseek");
#endif
   
    ret = real_lseek(fd, offset, whence);
    
#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
	
    return ret;
}

int
mangle_name(getdents, getdents)
(int fd, struct dirent *buf, size_t count)
{
	typedef int	(*getdents_func)(int fd, struct dirent *buf, size_t count);
	getdents_func	real_getdents;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif

    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_getdents = (getdents_func)dlsym(libhandle, "getdents");
#else
	real_getdents = (getdents_func)dlsym(RTLD_NEXT, "getdents");
#endif
	
    ret = real_getdents(fd, buf, count);

#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
    
    return ret; 
}

int
mangle_name(close, close)
(int fd)
{
	typedef int	(*close_func)(int fd);
	close_func	real_close;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif
    
    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_close = (close_func)dlsym(libhandle, "close");
#else   
	real_close = (close_func)dlsym(RTLD_NEXT, "close");
#endif

    ret = real_close(fd);

//#if defined(__linux__) && defined(__i386)
    dlclose(libhandle);
//#endif    
	
    return ret;
}

int
mangle_name(open, _open)
(const char *pathname, int flags, ...)
{
	typedef int	(*open_func)(const char *pathname, int flags, ...);
	open_func	real_open;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif

    uthread_yield();

#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_open = (open_func)dlsym(libhandle, "open");
#else   
	real_open = (open_func)dlsym(RTLD_NEXT, "open");
#endif
	
	/* HACK HACK HACK HACK
	 * i'm sure we could look at flags and see if there
	 * should be a third arg, and then use varargs to
	 * get at it... but that's alot of work.
	 */
    ret = real_open(pathname, flags, 0666);
  
#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
	
    return ret;
}

ssize_t
mangle_name(read, read)
(const int fd, void* buf, const size_t count)
{
	typedef int	(*read_func)(const int fd, void *buf, const size_t count);
	read_func	real_read;
    int ret;
#ifdef INTERPOSE_NEED_DLOPEN
    void* libhandle;
#endif
    
    uthread_yield();
    
#ifdef INTERPOSE_NEED_DLOPEN
    libhandle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	real_read = (read_func)dlsym(libhandle, "read");
#else   
	real_read = (read_func)dlsym(RTLD_NEXT, "read");
#endif
	
    ret = real_read(fd, buf, count);  

#ifdef INTERPOSE_NEED_DLOPEN
    dlclose(libhandle);
#endif    
	
    return ret;
}

