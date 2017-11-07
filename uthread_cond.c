#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "uthread.h"
#include "uthread_mtx.h"
#include "uthread_cond.h"
#include "uthread_queue.h"
#include "uthread_sched.h"


/*
 * uthread_cond_init
 *
 * initialize the given condition variable
 */
void
uthread_cond_init(uthread_cond_t *cond)
{
    utqueue_init(&cond->uc_waiters);
    
    //LOG("Entering uthread_cond_init");
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_cond_init");
}


/*
 * uthread_cond_wait
 *
 * Should behave just like a stripped down version of pthread_cond_wait.
 * Block on the given condition variable.  The caller should lock the
 * mutex and it should be locked again after the broadcast.
 */
void
uthread_cond_wait(uthread_cond_t *cond, uthread_mtx_t *mtx)
{
    LOG("Entering uthread_cond_wait");
    assert(mtx -> m_owner == ut_curthr);
    ut_curthr -> ut_state = UT_WAIT;
    utqueue_enqueue(&cond -> uc_waiters, ut_curthr);
    uthread_mtx_unlock(mtx);
    uthread_block();
    uthread_mtx_lock(mtx);
	
    
    //NOT_YET_IMPLEMENTED("UTHREADS: uthread_cond_wait");
}


/*
 * uthread_cond_broadcast
 *
 * Wakeup all the threads waiting on this condition variable.
 * Note there may be no threads waiting.
 */
void
uthread_cond_broadcast(uthread_cond_t *cond)
{
    LOG("Entering uthread_cond_broadcast");
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_cond_broadcast");
    uthread_t *tempthr;
    while (!utqueue_empty(&cond -> uc_waiters)) {
        tempthr = utqueue_dequeue(&cond -> uc_waiters);
        uthread_wake(tempthr);
    }
}



/*
 * uthread_cond_signal
 *
 * wakeup just one thread waiting on the condition variable.
 * Note there may be no threads waiting.
 */
void
uthread_cond_signal(uthread_cond_t *cond)
{
    LOG("Entering uthread_cond_signal");
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_cond_signal");
    uthread_t *tempthr;
    if (!utqueue_empty(&cond -> uc_waiters)) {
        tempthr = utqueue_dequeue(&cond -> uc_waiters);
        uthread_wake(tempthr);
    }
}

