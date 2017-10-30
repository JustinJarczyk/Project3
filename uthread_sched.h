
#ifndef __uthread_sched_h__
#define __uthread_sched_h__

#include <sys/types.h>
#include "uthread.h"

int uthread_setprio(uthread_id_t id, int prio);
void uthread_yield(void);
void uthread_block(void);
void uthread_wake(uthread_t *uthr);

#endif /* __uthread_sched_h__ */
