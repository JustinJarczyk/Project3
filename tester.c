/*
 *   FILE: tester.c 
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "uthread.h"
#include "uthread.c"
#include "uthread_mtx.h"
#include "uthread_mtx.c"
#include "uthread_cond.h"
#include "uthread_cond.c"
#include "uthread_sched.h"
#include "uthread_sched.c"

/*
 * Added these into the tester 
#include "interpose.c"
#include "uthread_cond.c"
#include "uthread_idle.c"
#include "uthread_queue.c"
#include "uthread.c"
#include "uthread_ctx.c"
#include "uthread_mtx.c"
#include "uthread_sched.c"
*/

#define	NUM_THREADS 3

#define SBUFSZ 256

uthread_id_t	thr[NUM_THREADS];
uthread_mtx_t	mtx;
uthread_cond_t	cond;
        
/* XXX: we're using sprintf and write to emulate printf - but, we're not 
 * really being as judicious as we should be about guarding write. */

static void
tester(long a0, void *a1)
{
    int	i = 0, ret;
    char pbuffer[SBUFSZ];
    
    while (i < 10)
    {
        sprintf(pbuffer, "thread %i: hello! (%i)\n", uthread_self(), i++);  
        ret = write(STDOUT_FILENO, pbuffer, strlen(pbuffer));
        if (ret < 0) 
        {
            perror("uthreads_test");
            /* XXX: we should really cleanup here */
            exit(1);
        }
		
        uthread_mtx_lock(&mtx);
        uthread_cond_signal(&cond);
        uthread_cond_wait(&cond, &mtx);
        uthread_mtx_unlock(&mtx);
    }

    sprintf(pbuffer, "thread %i exiting.\n", uthread_self());  
    ret = write(STDOUT_FILENO, pbuffer, strlen(pbuffer));
    if (ret < 0) 
    {
        perror("uthreads_test");
        /* XXX: we should really cleanup here */
        exit(1);
    }

    uthread_exit(a0);
}

int
main(int ac, char **av)
{
    int	i;

    uthread_init();

    uthread_mtx_init(&mtx);
    uthread_cond_init(&cond);

    for (i = 0; i < NUM_THREADS; i++)
    {
        uthread_create(&thr[i], tester, i, NULL, 0);
    }
    uthread_setprio(thr[0], 2);


    for (i = 0; i < NUM_THREADS; i++)
    {
        char pbuffer[SBUFSZ];
        int	tmp, ret;

        uthread_join(thr[i], &tmp);
    
        sprintf(pbuffer, "joined with thread %i, exited %i.\n", thr[i], tmp);  
        ret = write(STDOUT_FILENO, pbuffer, strlen(pbuffer));
        if (ret < 0) 
        {
            perror("uthreads_test");
            return EXIT_FAILURE;
        }   

        uthread_mtx_lock(&mtx);
        uthread_cond_signal(&cond);
        uthread_mtx_unlock(&mtx);
    }

    uthread_exit(0);

    return 0;
}
