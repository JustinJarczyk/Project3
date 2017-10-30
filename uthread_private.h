
#ifndef __uthread_private_h__
#define __uthread_private_h__


/*
 * initialize the scheduler.
 * called from uthread_init()
 */
void uthread_sched_init(void);


/*
 * swap out the currently running thread and wait
 * until there is another runnable thread and start
 * running it
 */
void uthread_switch(void);


/*
 * "idle" the "cpu".
 * see comment above uthread_switch()
 */
void uthread_idle(void);


#endif /* __uthread_private_h__ */
