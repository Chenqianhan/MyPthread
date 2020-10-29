// File:	mypthread_t.h

// List all group member's name: Qianhan Chen, Lu Wang
// username of iLab: lw491@ilab.cs.rutgers.edu
// iLab Server: iLab1

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>

typedef uint mypthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	mypthread_t id;
	//Even we free a tcb, any pointer pointer to there is still there
	//we need this variable to identify wild pointer.
	int is_init;
	// thread status
	// 0: ready, 1:blocked, 2:exit, 3:yield
	int status;
	// thread context
	ucontext_t *context;
	
	int priority;
	int elapse_time;
	void *value_ptr;
	struct itimerval *allotment;
	// YOUR CODE HERE
} tcb;

//Node of queue
typedef struct Node {
	tcb * thread;
	int priority;
	struct Node * next;
} Node;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */
	uint mutex_id;
	mypthread_t user_id;
	int is_locked;
	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE


/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

//------------------Utility Functions------------------
void mypthread_init();
void signal_handler(int sig);
tcb * dequeue();
void enqueue(tcb * thread);
void scheduler_init();
void boost_priority();
//------------------Variables--------------------------
#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
