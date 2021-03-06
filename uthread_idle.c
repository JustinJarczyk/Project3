
#include <sched.h>

#include "uthread.h"
#include "uthread_private.h"


/*
 * uthread_idle
 *
 * right now we just call linux's yield() function.
 * note that we cannot make system calls here, since it
 * is called from uthread_switch().
 */
void
uthread_idle(void)
{
  //#if defined(__linux__) && defined(__i386)	
    sched_yield();
    //#else
    /* solaris */
    //yield();
    //#endif    
}
