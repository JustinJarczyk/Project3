/*
 *   FILE: test.c 
 *  DESCR: a simple test program for uthreads
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/errno.h>

#include "uthread_sched.h"
#include "uthread_sched.c"

#include "uthread.h"
#include "uthread.c"

#include "uthread_mtx.h"
#include "uthread_mtx.c"

#include "uthread_ctx.h"
#include "uthread_ctx.c"

#include "uthread_cond.h"
#include "uthread_cond.c"

#include "uthread_queue.h"
#include "uthread_queue.c"


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

#define TEST_NUM_FORKS 4

#define SBUFSZ 256

#define DEBUG(fmt, args...) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ## args)

#define test_assert(x) \
	do { \
		if (!(x)) { \
			DEBUG("FAILED: %s\n", #x); \
		} \
	} while (0)

#define test_error(x, err) \
	test_assert(-1 == x && err == errno)

//         
/* XXX: we're using sprintf and write to emulate printf - but, we're not 
 * really being as judicious as we should be about guarding write. */

static void thread_dummy_function(long a0, void* a1);
static void thread_add_ten_to_counter(long a0, void* a1);
static void thread_add_ten_to_counter_with_initial_wait(long a0, void* a1);
static void thread_fork_creation(long a0, void* a1);
static void test_setup();
static void test_reset();
static void test_cleanup();

static void test_queue();
static void test_main_thread();
static void test_single_thread();
static void test_multiple_threads();
static void test_locks_and_conditions();
static void test_priority();
static void test_detached_threads();
static void test_thread_bomb();
static void test_thread_fork_creation();

enum {
    TEST_QUEUE,
    TEST_MAIN_THREAD,
    TEST_SINGLE_THREAD,
    TEST_MULTIPLE_THREADS,
    TEST_LOCKS_AND_CONDS,
    TEST_PRIORITY,
    TEST_DETACHED_THREADS,
    TEST_THREAD_BOMB,
    TEST_FORK_CREATION,
};

int test_counter;
char test_counter_guard;
uthread_mtx_t test_mtx;
uthread_cond_t test_cond;
char test_pause;
uthread_mtx_t test_pause_mtx;
uthread_cond_t test_pause_cond;

int
main(int ac, char **av)
{
    //DEBUG("Running ./ta_test \n");
    int ntest;

    if (ac < 2) {
       fprintf(stderr, "Usage: %s <test_num>\n", av[0]);
       return -1;
    }
    
    //DEBUG("   ./ta_test : running uthread_init\n");
    uthread_init();   
    //DEBUG("   ./ta_test : running test_setup\n");
    test_setup();

    //DEBUG("   ./ta_test : atoi\n");
    ntest = atoi(av[1]);
    
    //DEBUG("   ./ta_test : switch case \n");
    switch (ntest) {
        case TEST_QUEUE:
            test_queue();
            break;
        case TEST_MAIN_THREAD:
			test_main_thread();
            break;
        case TEST_SINGLE_THREAD:
            test_single_thread();
            break;
        case TEST_MULTIPLE_THREADS:
            test_multiple_threads();
            break;
        case TEST_LOCKS_AND_CONDS:
            test_locks_and_conditions();
            break;
        case TEST_PRIORITY:
            test_priority();
            break;
        case TEST_DETACHED_THREADS:
            test_main_thread();
            test_detached_threads();
            break;
        case TEST_THREAD_BOMB:
            test_main_thread();
            test_thread_bomb();
            break;
        case TEST_FORK_CREATION:
            test_thread_fork_creation();
            break;
        default:
            fprintf(stderr, "Invalid test number.\n");
    }

    //DEBUG("Test cleanup\n");
    test_cleanup();
    //DEBUG("Finished Test cleanup\n");
    // -->> THIS IS WHERE THE SEG FAULT HAPPENS
    //DEBUG("running uthread_exit(0);\n");
    uthread_exit(0);
    DEBUG("Thread %i got to the end of main, Should have exited THIS IS BAD\n", uthread_self());
    
    return 0;
}

/*
* Initialize mutexes, conditions, counters, guard booleans, etc.
*/
static void test_setup(){
    uthread_mtx_init(&test_mtx);
    uthread_cond_init(&test_cond);
    uthread_mtx_init(&test_pause_mtx);
    uthread_cond_init(&test_pause_cond);
    test_reset();
}

/*
* Reset counters and guard booleans
*/ 
static void test_reset(){
    test_counter = 0;
    test_counter_guard = 0;
    test_pause = 0;
}

/*
* In case anything needs to be cleaned up before exiting
*/
static void test_cleanup(){
    
}

/*
* Test the queue. Not written by students, but still worthwhile to test.
* Threads are created on the spot, they are not actual running threads.
*/
static void test_queue(){
    //DEBUG("Running test_queue()\n");
    utqueue_t queue;
    uthread_t threads[10];
    uthread_t* deqthr;
    int num_threads = 10;
    int i;

    utqueue_init(&queue);
    
    /* Initialize thread link fields since these are dummy threads */
    for (i = 0; i < num_threads; i++){
        threads[i].ut_link.l_next = NULL;
        threads[i].ut_link.l_prev = NULL;
    }

    for (i = 0; i < num_threads; i++){
        threads[i].ut_link.l_next = NULL;
        threads[i].ut_link.l_prev = NULL;
        utqueue_enqueue(&queue, &threads[i]);        
    }
    test_assert(!utqueue_empty(&queue));

    for (i = 0; i < num_threads; i++){
        deqthr = utqueue_dequeue(&queue);
        test_assert(deqthr == &threads[i]);
    }
    test_assert(utqueue_empty(&queue));
    DEBUG("Passed test_queue()\n");
}

/*
* Test simple methods on the main thread
*/
static void test_main_thread(){
    DEBUG("Running test_main_thread()\n");
    
    //LOG("***** test_assert(uthread_self() == 1); ******");
    test_assert(uthread_self() == 1);
    //LOG("***** uthread_setprio(uthread_self(), 7); ******");
    uthread_setprio(uthread_self(), 7);
    //LOG("**** uthread_yield(); *******");
    uthread_yield();
    //LOG("***** uthread_setprio(uthread_self(), 0); ******");
    uthread_setprio(uthread_self(), 0);
    //LOG("***** uthread_yield(); ******");
    uthread_yield();
    //LOG("***** est_error(uthread_detach(-1), ESRCH); ******");
    test_error(uthread_detach(-1), ESRCH);

    DEBUG("Passed test_main_thread()\n");
}

/*
* Test the creation and execution of a single thread.
* Test various errors associated with joining
*/ 
static void test_single_thread(){
    DEBUG("Running test_single_thread()\n");
    uthread_id_t thr;
    int expected_exit_value = 25;
    int exit_value;
    
    //DEBUG("Test Reset\n");
    test_reset();
    //DEBUG("Running test_assert(0 == uthread_create(&thr, thread_dummy_function, expected_exit_value, NULL, 0));\n");
    test_assert(0 == uthread_create(&thr, thread_dummy_function, expected_exit_value, NULL, 0));
    //DEBUG("test_assert(0 == uthread_join(thr, &exit_value));\n");
    test_assert(0 == uthread_join(thr, &exit_value));
    //DEBUG("test_assert(exit_value == expected_exit_value);\n");
    test_assert(exit_value == expected_exit_value);
    
    //DEBUG("test_error(uthread_join(-1,&exit_value), ESRCH);\n");
    test_error(uthread_join(-1,&exit_value), ESRCH);
    //DEBUG("test_error(uthread_join(UTH_MAX_UTHREADS+1,&exit_value), ESRCH);\n");
    test_error(uthread_join(UTH_MAX_UTHREADS+1,&exit_value), ESRCH);
    
    DEBUG("Passed test_single_thread()\n");
}

/*
* Same as single thread, except now there's more.
*/
static void test_multiple_threads(){
    DEBUG("Running test_multiple_threads()\n");
    uthread_id_t thr[8];
    int num_threads = 8;
    int thread_number, exit_value;
    
    test_reset();
    for(thread_number = 0; thread_number < num_threads; thread_number++){
        test_assert(0 == uthread_create(&thr[thread_number], thread_dummy_function, thread_number, NULL, 0));
    }
    for (thread_number = 0; thread_number < num_threads; thread_number++){
        exit_value = 0;
        test_assert(0 == uthread_join(thr[thread_number], &exit_value));
        test_assert(exit_value == thread_number);
    }
    
    DEBUG("Passed test_multiple_threads()\n");
}

/*
* Uses add_ten_to_counter method to test locks and conditions. Assumes
* threads are working properly at this point.
*/
static void test_locks_and_conditions(){
    DEBUG("Running test_locks_and_conditions()\n");
    uthread_id_t thr[8];
    int num_thr = 8;
    int thread_number, exit_value;
    
    test_reset();
    for(thread_number = 0; thread_number < num_thr; thread_number++){
        test_assert(0 == uthread_create(&thr[thread_number], thread_add_ten_to_counter, thread_number, NULL, 0));
    }
    for (thread_number = 0; thread_number < num_thr; thread_number++){
        exit_value = 0;
        test_assert(0 == uthread_join(thr[thread_number], &exit_value));
        test_assert(exit_value == thread_number);
    }
    test_assert(test_counter == 10*num_thr);
    DEBUG("Passed test_locks_and_conditions()\n");
}

/*
* Same as previous method, now each thread gets a different priority
*/
static void test_priority(){
    DEBUG("Running test_priority()\n");
    uthread_id_t thr[8];
    int num_thr = 8;
    int thread_number, exit_value;
    
    test_reset();
    for(thread_number = 0; thread_number < num_thr; thread_number++){
        test_assert(0 == uthread_create(&thr[thread_number],
                    thread_add_ten_to_counter, thread_number, NULL, thread_number%(UTH_MAXPRIO)));
    }
    uthread_setprio(thr[0],UTH_MAXPRIO);
    for (thread_number = 0; thread_number < num_thr; thread_number++){
        exit_value = 0;
        test_assert(0 == uthread_join(thr[thread_number], &exit_value));
        test_assert(exit_value == thread_number);
    }
    test_assert(test_counter == 10*num_thr);
    DEBUG("Passed test_priority()\n");
}

/*
* Can't forget about detaching threads. Uses add_ten_to_counter to make
* sure that they are working since there is no way to wait for them other
* than a condition or waiting for the counter to increment up to the 
* expected value.
* 
* The main thread has to have a lower priority than the new threads so that 
* when the main thread yields, other threads will get to go. uthread_switch
* says its okay to switch from the curthr to curthr if it 
*/
static void test_detached_threads(){
    DEBUG("Running test_detached_threads()\n");
    uthread_id_t thr[8];
    int num_thr = 8;
    int thread_number, exit_value;
    
    test_reset();
    uthread_setprio(uthread_self(), 0);
    for(thread_number = 0; thread_number < num_thr; thread_number++){
        test_assert(0 == uthread_create(&thr[thread_number],
                    thread_add_ten_to_counter_with_initial_wait, thread_number, NULL, 1));
        uthread_detach(thr[thread_number]);
    }
    
    /* Tell all the threads to go! */
    test_pause = 0;
    uthread_cond_broadcast(&test_pause_cond);
    
    for (thread_number = 0; thread_number < num_thr; thread_number++){
        test_error(uthread_join(thr[thread_number], &exit_value), EINVAL);
    }
	
    while(test_counter < 10*num_thr) {
        DEBUG("Waiting for detached threads to finish... (spinning)\n");
        uthread_yield();
    }
    test_assert(test_counter == 10*num_thr);
    DEBUG("Passed test_detached_threads()\n");
}

/*
* Get up, get up, get up, drop the bombshell \
* Get up, get up, this is outta control \
* Get up, get up, get up, drop the bombshell \
* Get up, get up, get gone
*   - Powerman 5000
* 
* This tests everything
*   - Multiple threads
*   - Creating more threads than are available
*   - Joining threads (and erroneous joins)
*   - Detaching threads
*   - (Broad)casting signals
*   - Using mutexes
*   - Priorties
*/
static void test_thread_bomb(){
    DEBUG("Running test_thread_bomb()\n");
    int i,j, exit_value;
    int num_thr = UTH_MAX_UTHREADS;
    //one init thread, one reaper thread already exist
    int num_thr_avail = UTH_MAX_UTHREADS-2; 
    uthread_id_t thr[UTH_MAX_UTHREADS];
    memset(thr,0,UTH_MAX_UTHREADS);
    
    test_reset();
    test_pause = 1;
    
    /* Create a bunch of threads, detach half of them, check for expected errors */
    for(i = 0; i < num_thr; i++){
		if (i < num_thr_avail) {
			test_assert(0 == uthread_create(&thr[i], thread_add_ten_to_counter_with_initial_wait, i, NULL, 0));
		} else {
			test_error(uthread_create(&thr[i], thread_add_ten_to_counter_with_initial_wait, i, NULL, 0), EAGAIN);
		}

        if (i < num_thr_avail && i % 2 == 0) {
            uthread_detach(thr[i]);
		}
    }
    
    /* Screw with priorities a lot */
    for (j = 0; j < 10; j++){
        for(i = 0; i < num_thr_avail; i++){
            uthread_setprio(thr[i],(i*j)%(UTH_MAXPRIO+1));
        }
    }
    
    /* Tell all the threads to go! */
    test_pause = 0;
    uthread_cond_broadcast(&test_pause_cond);
    
    /* Try to join the threads and check for expected return values */
    for (i = 0; i < num_thr; i++){
        exit_value = 0;
        if (i >= num_thr_avail){
            /* If uthread_create failed correctly, it should not have set
             * thr[i], and we will attempt to join with some garbage tid */
            int retval = uthread_join(thr[i], &exit_value);
            if (errno == ESRCH) {
                test_error(retval, ESRCH);
            } else {
                test_error(retval, EINVAL);
            }
            continue;
        }
        if (i % 2 == 1){
            test_assert(0 == uthread_join(thr[i], &exit_value));
            test_assert(exit_value == i);
        }
        else {
            /* Accept both EINVAL and ESRCH, since it depends on whether the
             * detached thread has exited which errno is appropriate */
            int retval = uthread_join(thr[i], &exit_value);
            if (errno == ESRCH) {
                test_error(retval, ESRCH);
            } else {
                test_error(retval, EINVAL);
            }
        }
    }
    
    /* Wait for all the threads to finish incrementing */
    while(test_counter < 10*num_thr_avail) {
        DEBUG("Waiting for detached threads to finish... (spinning)\n");
        uthread_yield();
    }
    /* Make sure that the final counter value is as expected (should already have happend) */
    test_assert(test_counter == 10*num_thr_avail);
    DEBUG("Passed test_thread_bomb()\n");
}



/*
* Test the idea of threads creating their own threads and so on.
* @see thread_fork_creation()
*/ 
static void test_thread_fork_creation(){
    DEBUG("Running test_thread_fork_creation()\n");
    uthread_id_t thr;
    int exit_value, i;

    test_reset();
    
    test_assert(0 == uthread_create(&thr, thread_fork_creation, 0, NULL, 0));
    
	test_assert(0 == uthread_join(thr, &exit_value));
    test_assert(exit_value == 0);
    
    exit_value = 1;
    for (i = 0; i < TEST_NUM_FORKS; i++)
        exit_value = exit_value * 2;
    
    test_assert(test_counter == exit_value);
    DEBUG("Passed test_thread_fork_creation()\n");
}


/*
* Dummy function for threads to use. Simply exits. Nothing exciting.
*/
static void
thread_dummy_function(long a0, void* a1){
    (void) a1;
    
    
    uthread_exit(a0);
    DEBUG("Thread %i got to end of thread_dummy_function. Should have exited. THIS IS BAD\n", uthread_self());
}

/*
* Increments the global counter 10 times by 1. It locks and waits on a 
* condition for a signal. Simple test program.
*/ 
static void 
thread_add_ten_to_counter(long a0, void* a1){
    (void) a1;
    int i;
    for (i = 0; i < 10; i++){
        uthread_mtx_lock(&test_mtx);
        while(test_counter_guard)
            uthread_cond_wait(&test_cond, &test_mtx);
        test_counter_guard = 1;
        test_counter = test_counter + 1;
        uthread_mtx_unlock(&test_mtx);
        test_counter_guard = 0;
        uthread_cond_signal(&test_cond);
    }
    uthread_exit(a0);
}

/*
* Increments the global counter 10 times by 1. It locks and waits on a 
* condition for a signal. Instead of just immediately incrementing
* like the previous version, this waits for the go-ahead from some other
* thread. It should be used by test functions so that all threads can be
* created before any threads finished. Once the threads are created,
* set the pause guard to 0 and broadcast the pause condition signal
*/ 
static void 
thread_add_ten_to_counter_with_initial_wait(long a0, void* a1){
    (void) a1;
    int i;
    uthread_mtx_lock(&test_pause_mtx);
    while(test_pause)
        uthread_cond_wait(&test_pause_cond, &test_pause_mtx);
    uthread_mtx_unlock(&test_pause_mtx);

    for (i = 0; i < 10; i++){
        uthread_mtx_lock(&test_mtx);
        while(test_counter_guard)
            uthread_cond_wait(&test_cond, &test_mtx);
        test_counter_guard = 1;
        test_counter = test_counter + 1;
        uthread_mtx_unlock(&test_mtx);
        test_counter_guard = 0;
        uthread_cond_signal(&test_cond);
    }
	uthread_exit(a0);
}

/*
* A thread that creates two threads that create two more threads...
* 
* Rather than having a single thread create all the threads and wait on all the
* joins, this method creates two new threads that will use this method and wait
* for them to return
* 
* The maximum depth is TEST_NUM_FORKS (4) (16 total threads)
* 
* The leaf threads increment the counter by 1 and then return. The end value
* of the counter should be TEST_NUM_FORKS^2
* 
* a0 is the initial depth (set to 0 for expected results);
*/
static void thread_fork_creation(long a0, void* a1){
    (void) a1;
    uthread_id_t thr1, thr2;
    int exit_value=-1;
    int retval=-1;
    if (a0 >= TEST_NUM_FORKS){
        uthread_mtx_lock(&test_mtx);
        test_counter = test_counter + 1;
        uthread_mtx_unlock(&test_mtx);
        uthread_exit(a0);
    }


    retval = uthread_create(&thr1, thread_fork_creation, a0+1, NULL, 0);
    test_assert(retval == 0);
    retval = uthread_create(&thr2, thread_fork_creation, a0+1, NULL, 0);
    
    retval = uthread_join(thr1, &exit_value);
    test_assert(exit_value == a0+1);
    test_assert(retval == 0);
    
    retval = uthread_join(thr2, &exit_value);
    test_assert(exit_value == a0+1);
    test_assert(retval == 0);

	uthread_exit(a0);
}
