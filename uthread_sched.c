
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>

#include "uthread.h"
#include "uthread_private.h"
#include "uthread_ctx.h"
#include "uthread_queue.h"
#include "uthread_bool.h"
#include "uthread_sched.h"


/* ---------- globals -- */

/* Remove __attribute__((unused)) when you use this variable. */

static utqueue_t __attribute__((unused)) runq_table[UTH_MAXPRIO + 1];	/* priority runqueues */

/* ----------- public code -- */


/*
 * uthread_yield
 *
 * Causes the currently running thread to yield use of the processor to
 * another thread. The thread is still runnable however, so it should
 * be in the UT_RUNNABLE state and schedulable by the scheduler. When this
 * function returns, the thread should be executing again. A bit more clearly,
 * when this function is called, the current thread stops executing for some
 * period of time (allowing another thread to execute). Then, when the time
 * is right (ie when a call to uthread_switch() results in this thread
 * being swapped in), the function returns.
 */
void
uthread_yield(void)
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_yield");
}



/*
 * uthread_block
 *
 * Put the current thread to sleep, pending an appropriate call to 
 * uthread_wake().
 */
void
uthread_block(void) 
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_block");
}


/*
 * uthread_wake
 *
 * Wakes up the supplied thread (schedules it to be run again).  The
 * thread may already be runnable or (well, if uthreads allowed for
 * multiple cpus) already on cpu, so make sure to only mess with it if
 * it is actually in a wait state.
 */
void
uthread_wake(uthread_t *uthr)
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_wake");
}


/*
 * uthread_setprio
 *
 * Changes the priority of the indicated thread.  Note that if the thread
 * is in the UT_RUNNABLE state (it's runnable but not on cpu) you should
 * change the list it's waiting on so the effect of this call is
 * immediate.
 */
int
uthread_setprio(uthread_id_t id, int prio)
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_setprio");
	return 0;
}



/* ----------- private code -- */


/*
 * uthread_switch()
 *
 * This is where all the magic is.  Wait until there is a runnable thread, and
 * then switch to it using uthread_swapcontext().  Make sure you pick the
 * highest priority runnable thread to switch to. Also don't forget to take
 * care of setting the ON_CPU thread state and the current thread. Note that
 * it is okay to switch back to the calling thread if it is the highest
 * priority runnable thread.
 *
 * Every time uthread_switch() is called, uthread_idle() should be called at
 * least once.  In addition, when there are no runnable threads, you should
 * repeatedly call uthread_idle() until there are runnable threads.  Threads
 * with numerically higher priorities run first. For example, a thread with
 * priority 8 will run before one with priority 3.
 * */
void
uthread_switch(void)
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_switch");
}



/*
 * uthread_sched_init
 *
 * Setup the scheduler. This is called once from uthread_init().
 */
void
uthread_sched_init(void)
{
	NOT_YET_IMPLEMENTED("UTHREADS: uthread_sched_init");
}
