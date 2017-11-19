# Project overview.

uthreads is a user-level threading package that is designed to behave similar to pthreads . This package contains support of the creation, deletion, and joining of threads and functions that deal with mutexes and condition variables.


# Assumptions made, if any.

1.	Compiling project on Ubuntu 16.04: To solve this problem, I first modified the makefile. I later installed cscope and other dependencies, enabling me to search through the source code. To correctly compile testing scripts, some libraries and source files were added. After adding all of these elements, existing code was modified in order for the program to work.
2.	Setting up a testing environment (OS): To set up the testing environment, I must determine if the occurring errors came from the OS or the template code. 
3.	Testing: During the testing process, I realized that most of the code needed to be debugged. I have to simplify code from multiple logging statements. When running into these issues, I need to be aware of which components to debug.
4.	Getting the scheduler to schedule threads in the correct order: At times, the scheduler did not properly read priorities. I also struggled with setting up the main thread and a reaper thread.
5.	Setting the states of a thread to be reaped properly: To understand what the reaper was doing, I had to debug it.
6.	Testing ‘wait’ functionality for threads: In other words, getting the parent threads to properly wait for child threads. I have to understand which thread was a waiter and the interaction between those states.
7.	Understanding how all the functions tied in together: To understand the functionality, I tracked down the codes order of execution.


# Any Design decisions you had to make or issues you faced, in brief.
## Summary (include any specific new learnings, if possible).

Methods and their Functionalities

Allocation and freeing of stacks
-	static char *alloc_stack(void)
-	void free_stack(char *stack)

Creates machine
-	Void uthread_makecontext(uthread_ctx_t *ctx, char *stack, int stacksz, void (*func)(), long arg1,void *arg2);

Save current CPU context and executes new context; saved context will resume later
-	Void uthread_swapcontext(uthread_ctx_t *oldctx, uthread_ctx_t *newctx)

Methods that control the creation/deletion/joining/detaching of threads
-	Void uthread_init();
-	Int uthread_create(uthread_id_t *uidp, uthread_func_t func, long arg1, void *arg2, int prio);
-	Void uthread_exit(int status);
-	Void uthread_join(uthread_id_t uid, int *return_value);
-	Int uthread_detach(uthread_id_t uid);
-	Uthread_id_t uthread_alloc();
-	Void uthread_destroy(uthread_t *uth);

Methods that control the state of the threads
-	void uthread_yield()
-	Void uthread_block()
-	Void uthread_wake(uthread_t *uthr);
-	Void uthread_setprio(uthread_id_t id, int prio);
-	Void uthead_switch();
-	Void uthread_sched_init();

Methods for locking & scheduling mechanisms
-	Void uthread_mtx_init( uthread_mtx_t *mtx);
-	Void uthread_mtx_lock( uthread_mtx_t *mtx);
-	Int uthread_mtx_trylock(uthread_mtx_t *mtx);
-	Void uthread_mtx_unlock(uthread_mtx_t *mtx);
-	Void uthread_cond_init(uthread_cond_t *cond)
-	Void uthread_cond_wait(uthread_cond_t *cond, uthread_mtx_t *mtx)
-	Void uthread_cond_broadcast(uthread_cond_t *cond);
-	Void uthread_cond_signal(uthread_cond_t *cond);
