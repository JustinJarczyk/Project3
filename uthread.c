#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "uthread.h"
#include "uthread_private.h"
#include "uthread_queue.h"
#include "uthread_bool.h"
#include "uthread_sched.h"


//#include "uthread_idle.c"




/* ---------- globals -- */

uthread_t	*ut_curthr = NULL;		/* current running thread */
uthread_t	uthreads[UTH_MAX_UTHREADS];	/* threads on the system */

static list_t		reap_queue;		/* dead threads */
static uthread_id_t	reaper_thr_id;		/* to wake reaper */


/* ---------- prototypes -- */

static void create_first_thr(void);

static uthread_id_t uthread_alloc(void);
static void uthread_destroy(uthread_t *thread);

static char *alloc_stack(void);
static void free_stack(char *stack);

static void reaper_init(void);
static void reaper(long a0, void *a1);
static void make_reapable(uthread_t *uth);



/* ---------- public code -- */

/*
 * uthread_init
 *
 * Called exactly once when the user process (for which you will be scheduling
 * threads) is started up. Perform all of your global data structure 
 * initializations and other goodies here.  It should go before all the
 * provided code.
 */
void
uthread_init(void)
{
    LOG_INIT("Entering uthread_init");
        LOG_INIT("   uthread_init : initializing array");
        // create thread array with no states
        for (int i = 0; i < UTH_MAX_UTHREADS ; i++)
        { 
            uthreads[i].ut_id = i;
            uthreads[i].ut_prio = -1;
            uthreads[i].ut_state = UT_NO_STATE;
            uthreads[i].ut_link.l_prev = NULL;
            uthreads[i].ut_link.l_next = NULL;
            
        }
      
	/* XXX: don't touch anything below here */

	/* these should go last, and in this order */
      LOG_INIT("   uthread_init : running uthread_sched_init()");
	uthread_sched_init();
        LOG_INIT("   uthread_init : running reaper_init()");
	reaper_init();
        LOG_INIT("   uthread_init :running create_first_thr()");
	create_first_thr();
        
        LOG_INIT("************* FINISHED INITIALIZATION *********************");
}



/*
 * uthread_create
 *
 * Create a uthread to execute the specified function <func> with argument
 * <arg> and initial priority <prio>. To do this, you should first find a
 * valid (unused) id for the thread using uthread_alloc (failing this, return
 * an error).  Next, create a context for the thread to execute on using
 * uthread_makecontext(), set up a uthread_t struct, make the thread runnable
 * and return the aforementioned thread id in <uidp>.  Return 0 on success, -1
 * on error.
 */


int
uthread_create(uthread_id_t *uidp, uthread_func_t func,
	       long arg1, void *arg2, int prio)
{
    LOG_CREATE("Entering uthread_create");

    assert(uidp != NULL);
    LOG_CREATE("  uthread_create: checking prio");   
    LOG_CREATE_INT( prio);
    // TODO:// Unsure
    if ((prio < 0) || (prio > UTH_MAXPRIO)){
        errno = EINVAL;
        return -1;
    }
    LOG_CREATE("  uthread_create: finished checking prio");
    
    LOG_CREATE("  uthread_create: uthread_alloc");
    uthread_id_t u;
    
    int thread_count = 0;
    for (int i = 0; i < UTH_MAX_UTHREADS; i++)
    {
        if(uthreads[i].ut_state != UT_NO_STATE) thread_count++;
    }
    
    LOG9MINT("Thread count ", thread_count);
    LOG9MINT("UTH_MAX_UTHREADS ", UTH_MAX_UTHREADS);
    if (thread_count == UTH_MAX_UTHREADS){
        errno = EAGAIN;
        return -1;
    }
    
    if ((u = uthread_alloc()) == -1){
        // success
        errno = EAGAIN;
        return -1;
    }
    LOG_CREATE("  uthread_create: finished uthread alloc");
    
    uthread_t *t;
    
    LOG_CREATE("  uthread_create: getting the newly alloc thread from uthreads");
    for (int i = 0; i < UTH_MAX_UTHREADS; i++)
    { 
        if (uthreads[i].ut_id == u){
            t = &uthreads[i];
            break;
        }
    }
    LOG_CREATE("  uthread_create: finished getting the newly alloc thread from uthreads");
    
    LOG_CREATE("  uthread_create: allocating stack for new thread");
    if((t->ut_stack = alloc_stack()) == NULL){
        errno = EAGAIN;
        return - 1;   
    }
    
    LOG_CREATE("  uthread_create: creating the context");
    //uthread_ctx_t *ctx = (uthread_ctx_t *) malloc(sizeof(uthread_ctx_t));
    LOG_CREATE("  uthread_create: calling uthread_makecontext");
    
    

    uthread_makecontext(&t->ut_ctx, t->ut_stack, UTH_STACK_SIZE, func, arg1, arg2);

    
    LOG_CREATE("  uthread_create: setting the thread to runnable");
    t->ut_state = UT_RUNNABLE;
    
    
    //uthread_setprio(u, prio);
    //uidp = &u;
    
    LOG_CREATE("  uthread_create: setting the priority of thread");
    uthread_setprio(t->ut_id, prio);
    
    //uidp = &t->ut_id;
    
    memcpy ( uidp, &u, sizeof ( uthread_id_t ));
    LOG6MINT("ID", *uidp);
    LOGINT2("create uthread", u);
    /*
    LOG_CREATE("*** uthread array ***");
    for (int i = 0; i < UTH_MAX_UTHREADS; i++)
    { 
        if(uthreads[i].ut_state != UT_NO_STATE){
            LOGINT((int)uthreads[i].ut_id);
        }
    }
    */
    
    LOG_CREATE("returning 0;");
    //NOT_YET_IMPLEMENTED("UTHREADS: uthread_create");
    return 0;
}



/*
 * uthread_exit
 *
 * Terminate the current thread.  Should set all the related flags and
 * such in the uthread_t. 
 *
 * If this is not a detached thread, and there is someone
 * waiting to join with it, you should wake up that thread.
 *
 * If the thread is detached, it should be put onto the reaper's dead
 * thread queue and wakeup the reaper thread by calling make_reapable().
 */
void
uthread_exit(int status)
{
    
    LOG7("Entering uthread_exit");
    ut_curthr->ut_state = UT_ZOMBIE;
    ut_curthr->ut_has_exited = true;
    ut_curthr->ut_exit =status;
    
    LOG7("   uthread_exit: checking if (ut_curthr->ut_detached == true )");
    LOG7("   uthread_exit: else if (ut_curthr->ut_waiter != NULL) ");
    if (ut_curthr->ut_detached == true ){
        LOG7("   uthread_exit: calling make_reapable(ut_curthr)");
        make_reapable(ut_curthr);
        
        LOG7("   uthread_exit: finished make_reapable(ut_curthr)");
    }
    else{
        if (ut_curthr->ut_waiter != NULL) 
        {
            LOG7("   uthread_exit: calling uthread_wake");
             uthread_wake(ut_curthr->ut_waiter);   
             LOG7("   uthread_exit: finished uthread_wake");
             //uthread_detach(ut_curthr->ut_id);
        }
        //ut_curthr->ut_detached = 1;
        //make_reapable(ut_curthr);  
    }
    
    
    
    //NOT_YET_IMPLEMENTED("UTHREADS: uthread_exit");
    LOG7("   uthread_exit: calling uthread_switch()");
    
    if (ut_curthr->ut_id > 1){
        
        uthread_switch();
        exit(0);
    } else {
        //uthread_switch();
        // just exit out of the main thread
        exit(0);
    }
    
    PANIC("returned to a dead thread");
}



/*
 * uthread_join
 *
 * Wait for the given thread to finish executing. If the thread has not
 * finished executing, the calling thread needs to block until this event
 * happens.
 *
 * Error conditions include (but are not limited to):
 * o the thread described by <uid> does not exist
 * o two threads attempting to join the same thread, etc..
 * Return an appropriate error code (found in manpage for pthread_join) in 
 * these situations (and more).
 *
 * Note that if a thread finishes executing and is never uthread_join()'ed
 * (or uthread_detach()'ed) it remains in the state UT_ZOMBIE and is never 
 * cleaned up. 
 *
 * When you have successfully joined with the thread, set its ut_detached
 * flag to true, and then wake the reaper so it can cleanup the thread by
 * calling make_reapable
 */
int
uthread_join(uthread_id_t uid, int *return_value)
{
    LOG("Entering uthread_join");
 
    LOG9MINT("attempting to join to id = ", uid);
    
    int is_valid = false;
    
    uthread_t *t;
    
    LOG("   uthread_join : ");
    for (int i = 0; i < UTH_MAX_UTHREADS ; i++)
    { 
        if ((uthreads[i].ut_id == uid) && (uthreads[i].ut_state != UT_NO_STATE)){
            
            t = &uthreads[i];
            is_valid = true;
            break;
        }
    }

    
    
    if (!is_valid){
        LOG9("   uthread_join : FAILED is_valid");
        errno = ESRCH;
        LOG6MINT("Not Valid Thread ID = ",uid);
        return -1;
    }
    
    if (t->ut_waiter != NULL){
        LOG("   uthread_join : FAILED if (t->ut_waiter != NULL){ ");
        errno = EINVAL ;
        LOG6MINT("Not Valid Thread ID 2 = ",uid);
        return -1;
    }
    
    if (t->ut_detached == true){
        LOG("   uthread_join : FAILED if (t->ut_detached == true){");
        errno = EINVAL;
        LOG6MINT("Not Valid Thread ID 3 = ",uid);
        return -1;
    }
    
    // added in to stop random joins
    if (t->ut_id == reaper_thr_id){
        errno = ESRCH;
        return -1;
    }
    // calling thread is trying to join itself
    if (ut_curthr->ut_id == uid){
        LOG("   uthread_join : FAILED if (ut_curthr->ut_id == uid){");
        errno = EDEADLK;
        LOG6MINT("Not Valid Thread ID 4 = ",uid);
        return -1;
    }
    
    LOG("   uthread_join : if (ut_curthr->ut_state != UT_ZOMBIE)");
    if (t->ut_state != UT_ZOMBIE){
        //ut_curthr->ut_waiter = t;
        t->ut_waiter = ut_curthr;
        uthread_block();
        LOG6MINT("Not Valid Thread ID 5 = ",uid);
    }
    
    
    t->ut_detached = 1;
    make_reapable(t);
    
    LOG("   uthread_join : Entering uthread_block()");    
    LOG("   uthread_join : finished uthread_block()");     
    
    
     if (return_value != NULL){
         memcpy ( return_value, &t->ut_exit, sizeof ( uthread_id_t ));
         //return_value = &t->ut_exit;
     }
    

    return 0;
    /*
     * If the target thread has terminated or when it it finally does, now you clean up the target thread by simply setting its ut_detached entry to 1 (ready to be reaped). 
If the return_value int pointer is not NULL, assign the ut_exit value of the target thread to *return_value.
Now arrange for the target to be reaped by calling make_reapable with a pointer to the target thread's entry in uthreads. and return 0 (for success)

     
    
    NOT_YET_IMPLEMENTED("WORKING ON THIS NOW NEED TO FINSIH THIS PROCEDURE ");
    
    NOT_YET_IMPLEMENTED("UTHREADS: uthread_join");
    return 0;
     */
}



/*
 * uthread_detach
 *
 * Detach the given thread. Thus, when this thread's function has finished
 * executing, no other thread need (or should) call uthread_join() to perform
 * the necessary cleanup.
 *
 * There is also the special case if the thread has already exited and then
 * is detached (i.e. was already in the state UT_ZOMBIE when uthread_deatch()
 * is called). In this case it is necessary to call make_reapable on the
 * appropriate thread.
 *
 * There are also some errors to check for, see the man page for
 * pthread_detach (basically just invalid threads, etc).
 * 
 */
int
uthread_detach(uthread_id_t uid)
{   
    LOG7("Entering uthread_detach");
    int isvalid = false;
    uthread_t *u;
    
    LOG7("   uthread_detach: getting the thread to detach");
    for(int i=0; i <UTH_MAX_UTHREADS; i++ ){
        if(uthreads[i].ut_id == uid){
            LOG7("   uthread_detach: setting [u = &uthreads[i]; and breaking out of loop]");
            u = &uthreads[i];
            isvalid = true;
            break;
        }
    }
    
    LOG7("   uthread_detach: checking if (!isvalid)");
    if (!isvalid){
        LOG7("   uthread_detach: not valid");
        errno = ESRCH;
        return -1;
    }
    
    LOG7("   uthread_detach: setting ut_detached to true");
    u->ut_detached = true;
    LOG7("   uthread_detach: checking if [if(u->ut_state == UT_ZOMBIE)]");
    
    if(u->ut_state == UT_ZOMBIE){
        LOG7("   uthread_detach: calling make_reapable");
        make_reapable(u);
    }
    
    LOG7("   uthread_detach: leaving");   
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_detach");
    return 0;
}



/*
 * uthread_self
 *
 * Returns the id of the currently running thread.
 */
uthread_id_t
uthread_self(void)
{
	assert(ut_curthr != NULL);
	return ut_curthr->ut_id;
}




/* ------------- private code -- */



/*
 * uthread_alloc
 *
 * find a free uthread_t, returns the id.
 * Remove __attribute__((unused)) when you call this function.
 */
static uthread_id_t uthread_alloc(void)
{
    LOG("Entering uthread_alloc");
/*    Find the first unused entry in the uthreads array. An entry is unused if its state is UT_NO_STATE.
        Return the ut_id value of that entry.
        If no such free entry is found, return -1. */
    
    
    //NOT_YET_IMPLEMENTED("UTHREADS: __attribute__");
    
    for (int i = 0;i < UTH_MAX_UTHREADS; i++)
    { 
        if (uthreads[i].ut_state == UT_NO_STATE){
            return uthreads[i].ut_id;
        }
    }
    
    return -1;
}

/*
 * uthread_destroy
 *
 * Cleans up resources associated with a thread (since it's now finished
 * executing). This is called implicitly whenever a detached thread finishes 
 * executing or whenever non-detached thread is uthread_join()'d.
 */
static void
uthread_destroy(uthread_t *uth)
{
    LOG("Entering uthread_destroy");
    assert(uth != NULL);
    free_stack(uth->ut_stack);
    uth->ut_state = UT_NO_STATE;
	//NOT_YET_IMPLEMENTED("UTHREADS: uthread_destroy");
}






/****************************************************************************
 * You do not have to modify any code below this line
 ****************************************************************************/


/*
 * reaper_init
 *
 * startup the reaper thread
 */
static void
reaper_init(void)
{
	list_init(&reap_queue);
	uthread_create(&reaper_thr_id, reaper, 0, NULL, UTH_MAXPRIO);
        LOGMINT3("REAPER ID : ", reaper_thr_id);
	assert(reaper_thr_id != -1);
}



/*
 * reaper
 *
 * this is responsible for going through all the threads on the dead
 * threads list (which should all be in the ZOMBIE state) and then
 * cleaning up all the threads that have been detached/joined with
 * already.
 *
 * in addition, when there are no more runnable threads (besides the
 * reaper itself) it will call exit() to stop the program.
 */
static void
reaper(long a0, void *a1)
{
	while(1)
	{
		uthread_t	*thread;
		int		th;

		/* block.  someone will wake me up when it is time */
		uthread_block();

		/* go through dead threads, find detached and
		 * call uthread_destroy() on them
		 */
		list_iterate_begin(&reap_queue, thread, uthread_t, ut_link)
		{
			assert(thread->ut_state == UT_ZOMBIE);

			list_remove(&thread->ut_link);
			uthread_destroy(thread);
		}
		list_iterate_end();

		/* check and see if there are still runnable threads */
		for (th = 0; th < UTH_MAX_UTHREADS; th++)
		{
			if (th != reaper_thr_id &&
			    uthreads[th].ut_state != UT_NO_STATE)
			{
				break;
			}
		}

		if (th == UTH_MAX_UTHREADS)
		{
			/* we leak the reaper's stack */
			fprintf(stderr, "uthreads: no more threads.\n");
			fprintf(stderr, "uthreads: bye!\n");
			exit(0);
		}
	}
}



/*
 * Turns the main context (the 'main' method that initialized
 * this process) into a regular uthread that can be switched
 * into and out of. Must be called from the main context (i.e.,
 * by uthread_init()).
 */
static void
create_first_thr(void)
{
    LOG_INIT("Entering create_first_thr");
	uthread_t	hack;
	uthread_id_t	main_thr;

	/*
	 * OK, this is a little bit of magic.  Right now, we are not
	 * really running in a uthread, but rather just this context that is
	 * the one that the program starts in.
	 *
	 * We would like to be running inside a real uthread, because then 
	 * we can block, reschedule, and so forth.
	 *
	 * So: First, allocate something to switch away from ("hack"), so 
	 *  the switch routine doesn't panic.
	 * Then, allocate the actual uthread we're switching into.
	 * Clone the current context into our new saved context.
	 * Switch into it.
	 * Return.
	 */

        LOG_INIT("  create_first_thr : ");
	memset(&hack, 0, sizeof(uthread_t));
	ut_curthr = &hack;
	ut_curthr->ut_state = UT_ON_CPU;
        
        LOG_INIT("  create_first_thr : uthread_create");
	uthread_create(&main_thr,
		       (uthread_func_t)0, 0, NULL, UTH_MAXPRIO);
	assert(main_thr != -1);
	
        
        LOG_INIT("  create_first_thr : uthread_detach(main_thr);");
        uthread_detach(main_thr);

	/* grab the current context, so when we switch to the main
	 * thread, we start *right here*.  thus, we only want to call 
	 * uthread_switch() if the current thread is the hacky temporary
	 * thread, not a real one.
	 */
        
        LOG_INIT("  create_first_thr : calling uthread_getcontext");
	uthread_getcontext(&uthreads[main_thr].ut_ctx);

	if (ut_curthr == &hack)
	{
            LOG_INIT("  create_first_thr : calling uthread_switch();");
		uthread_switch();
	}
	else
	{
            
            LOG_INIT("  create_first_thr : assert(uthread_self() == main_thr);");
            LOGINT2("create first", main_thr);
		/* this should be the 'main_thr' */
		assert(uthread_self() == main_thr);
	}

}

/*
 * Adds the given thread to the reaper's queue, and wakes up the reaper.
 * Called when a thread is completely dead (is detached and exited).
 *
 * Remove __attribute__((unused)) when you call these functions.
 */

/*
void make_reap_wrap(uthread_t *uth)
{
    
    make_reapable(uth);
}
*/
static void
/*__attribute__((unused))*/ make_reapable(uthread_t *uth)
{
	assert(uth->ut_detached);
	assert(uth->ut_state == UT_ZOMBIE);
	list_insert_tail(&reap_queue, &uth->ut_link);
        LOGMINT3("CALLING REAPER_ON_THREAD",reaper_thr_id);
	uthread_wake(&uthreads[reaper_thr_id]);
}



static char
__attribute__((unused)) *alloc_stack(void)
{
	return (char *)malloc(UTH_STACK_SIZE);
}

static void
__attribute__((unused)) free_stack(char *stack)
{
	free(stack);
}



