#include <sched.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>


#include <unistd.h>

#include "uthread.h"

#include "uthread_private.h"

#include "uthread_ctx.h"
#include "uthread_queue.h"
#include "uthread_bool.h"
#include "uthread_sched.h"

//#include "uthread_idle.c"



/* ---------- globals -- */

/* Remove __attribute__((unused)) when you use this variable. */

static utqueue_t /*__attribute__((unused))*/ runq_table[UTH_MAXPRIO + 1];	/* priority runqueues */

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
    LOG("Entering uthread_yield");
    ut_curthr->ut_state = UT_RUNNABLE ;
    utqueue_enqueue(runq_table, ut_curthr);
    uthread_switch();
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_yield");
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
    LOG("Entering uthread_block");
    if(ut_curthr != NULL){
        ut_curthr->ut_state = UT_WAIT;
        uthread_switch() ;  
    }
    
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_block");
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
    LOG("Entering uthread_wake");
    assert(uthr != NULL);
    
    assert(uthr->ut_prio >= 0);
    assert(uthr->ut_prio <= UTH_MAXPRIO);

    LOG("   uthread_wake: checking state for UT_WAIT, setting it to UT_RUNNABLE and queueing it");
    if (uthr->ut_state == UT_WAIT){
        uthr->ut_state = UT_RUNNABLE;
        LOG("   uthread_wake: calling utqueue_enqueue(runq_table, uthr)");
        utqueue_enqueue(runq_table, uthr);
    }
    //NOT_YET_IMPLEMENTED("UTHREADS: uthread_wake");
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
    LOG("Entering uthread_setprio");
    int isvalid = false;
    uthread_t *t;
    
    for (int i = 0;i < UTH_MAX_UTHREADS; i++)
    { 
        if (uthreads[i].ut_id == id){
            isvalid = true;
            t = &uthreads[i];
            //return uthreads[i].ut_id;
        }
    }
    
    if (!isvalid){
        errno = EINVAL;
        return -1;
    }
    
    // TODO:// NOT SURE HERE [   if ((0 >= prio) && (prio <= UTH_MAXPRIO)){   ]
    if ((prio < 0) && (prio > UTH_MAXPRIO)){
        errno = EINVAL;
        return -1;
    }
    
    if(prio != t->ut_prio){
        //this is either a new thread or we are changing the priority of an existing thread
        if(t->ut_state == UT_RUNNABLE){
            if (t->ut_prio == -1){
                //if the current priority is -1, this is a new thread and so it isn't yet on a run queue.
                
                //t->ut_link->l_prev = NULL;
                //t->ut_link->l_next = NULL;
                
                t->ut_prio = prio;
                utqueue_enqueue(runq_table, t);
                return 0;
            } else {
                t->ut_prio = prio;
                utqueue_remove(runq_table, t);
            }
        }
    }
    
//	NOT_YET_IMPLEMENTED("UTHREADS: uthread_setprio");
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
    LOG("Entering uthread_switch");
    // weird issue, took contents of the uthread_idle() and placed below
    //uthread_idle();
    int pre_highest = 0;
    
    int isvalid = false;
    
    
    LOG("   uthread_switch: starting while loop");
    uthread_t *t;
    while(!isvalid){
        sched_yield();
        //sleep(1);
        pre_highest = 0;

        
        //LOG4("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        for (int i = 0; i < UTH_MAX_UTHREADS; i++)
        { 
            
            /*if (uthreads[i].ut_prio != -1){
                
                LOG5("*********************");
                LOG5MINT("THREAD ID: ",uthreads[i].ut_id);
                LOG5MINT("THREAD  PRIORITY ",uthreads[i].ut_prio);
                if (uthreads[i].ut_waiter != NULL){
                    LOG5MINT("=====>  Waiter ",uthreads[i].ut_waiter->ut_id);
                }
                LOG5MINT("THREAD (WAITING) STATE ",uthreads[i].ut_state);  
            }*/
            
            if ((uthreads[i].ut_prio >= pre_highest) && (uthreads[i].ut_state == UT_RUNNABLE)){
                t = &uthreads[i];
                pre_highest = uthreads[i].ut_prio;
                isvalid = true;
            }
        }
    }
    LOG("   uthread_switch: exiting while loop");
    LOGINT2("THIS IS THE HIGHEST PRIORITY JOB ID ", t->ut_id);
    utqueue_remove(runq_table, t);
    
    assert(t->ut_stack != NULL);
    
    
    uthread_t *old_thrd = ut_curthr;
    ut_curthr = t;
    //t->ut_state = UT_ON_CPU;
    ut_curthr->ut_state = UT_ON_CPU;
    
    LOG("   uthread_switch: swapping context");
    
        uthread_swapcontext(&old_thrd->ut_ctx, &ut_curthr->ut_ctx);
        
    LOG("   uthread_switch: finished swapping context");
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_switch");
}



/*
 * uthread_sched_init
 *
 * Setup the scheduler. This is called once from uthread_init().
 */
void
uthread_sched_init(void)
{
    LOG("Entering uthread_sched_init");
    utqueue_init(runq_table);
    //NOT_YET_IMPLEMENTED("UTHREADS: uthread_sched_init");
}
