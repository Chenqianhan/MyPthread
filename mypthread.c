// File:	mypthread.c

// List all group member's name: Qianhan Chen, Lu Wang
// username of iLab: lw491@ilab.cs.rutgers.edu
// iLab Server: iLab1

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE/
#ifndef MLFQ
#define SCHED 1        // Choose STCF
#else
#define SCHED 2        // Choose MLFQ
#endif

#define QUANTUM 25
#define ADDITION_TIME 5 //time slice for each queue is 25, 30, 35....
#define STACK_SIZE 4096 //I am not sure how much is appropriate
#define THREAD_CAP 200
#define PRI_LEVEL 8
#define READY 0
#define BLOCKED 1
#define EXIT 2
#define YIELD 3
//for mlfq priority boosting
#define REFRESH_TIME 20

static int tid;
static tcb *running_thread;
static int running_tid;
static tcb *thread_list[THREAD_CAP]; //Search thread's tcb by id.
static void *exit_value[THREAD_CAP];
static int exit_list[THREAD_CAP];
static struct itimerval *timer;
static struct itimerval *timeout;
static struct itimerval *cur;
static int is_init = 0;
static int thread_count;
static int period_count;
static Node *queues[PRI_LEVEL];
static struct itimerval *timer_list[PRI_LEVEL];

static void sched_stcf() {
        // Your own implementation of STCF
        // (feel free to modify arguments and return types)

        setitimer(ITIMER_VIRTUAL, timeout, NULL);
        // YOUR CODE HERE
        if(running_thread->status == EXIT){
		printf("Thread %d finished\n", running_thread->id);
                exit_list[running_thread->id] = 1;
		//Even when tcb is destoryed, those dynamic variable still there, which means pointers' address still there.
                exit_value[running_thread->id] = running_thread->value_ptr;
                //*exit_value[running_thread->id] = *running_thread->value_ptr;

                //The pointers mean dynamic allocation. 
                //Unlike variables that allocated statically, we need to free pointers one by one 
                free(thread_list[running_thread->id]->context);
               	free(thread_list[running_thread->id]->value_ptr);
                thread_count--;
                free(thread_list[running_thread->id]);
		//thread_list[running_thread->id] = NULL;
		running_thread = NULL;

        }else if(running_thread->status == BLOCKED){
		enqueue(running_thread);

	}else if(running_thread->status == YIELD){
		running_thread->status = READY;
		enqueue(running_thread);

	}else if(running_thread->status == READY){
                running_thread->elapse_time++;
                enqueue(running_thread);

        }
	
	//-------------------------------------------------------------
	if(running_thread == NULL){
		tcb *next = dequeue();
		if(next == NULL)
			return;

		running_thread = next;
		if(thread_count>1)
			setitimer(ITIMER_VIRTUAL, timer, NULL);
		setcontext(next->context);
	}else{
        	tcb *current = running_thread;
        	tcb *next = dequeue();
		if(next == NULL)
			return;

        	running_thread = next;
		//printf("Thread %d to Thread %d\n", current->id, next->id);
		if(thread_count>1)
			setitimer(ITIMER_VIRTUAL, timer, NULL);
        	swapcontext(current->context, next->context);
	}
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
        // Your own implementation of MLFQ
        // (feel free to modify arguments and return types)
	setitimer(ITIMER_VIRTUAL, timeout, NULL);
	
	//when it raise SIGVTALRM for 20 times, boosting priority
	if(++period_count == REFRESH_TIME){
		period_count = 0;
		boost_priority();
	}

	if(running_thread->status == READY){
		//It finish the time slice
		if(running_thread->priority < PRI_LEVEL-1){
			running_thread->priority++;
		}
		//Enqueue
		enqueue(running_thread);

	}else if(running_thread->status == BLOCKED){
		//Enqueue
		enqueue(running_thread);

	}else if(running_thread->status == EXIT){
		exit_list[running_thread->id] = 1;
                exit_value[running_thread->id] = running_thread->value_ptr;
               
                free(thread_list[running_thread->id]->context);
                free(thread_list[running_thread->id]->value_ptr);
                thread_count--;
                free(thread_list[running_thread->id]);
                running_thread = NULL;

	}else if(running_thread->status == YIELD){
		//It is the same as status == 1, because it didn't finish.
		running_thread->status == READY;
		enqueue(running_thread);
	}
	
	//----------------------------------------------------------------
	if(running_thread == NULL){
                tcb *next = dequeue();
                if(next == NULL)
                        return;

                running_thread = next;
                if(thread_count>1)
                        setitimer(ITIMER_VIRTUAL, timer_list[running_thread->priority], NULL);
                setcontext(next->context);
        }else{
                tcb *current = running_thread;
                tcb *next = dequeue();
                if(next == NULL)
                        return;

                running_thread = next;
                //printf("Thread %d to Thread %d\n", current->id, next->id);
                if(thread_count>1)
                        setitimer(ITIMER_VIRTUAL, timer_list[running_thread->priority], NULL);
                swapcontext(current->context, next->context);
        }

}

static void schedule() {
        // Every time when timer interrup happens, your thread library
        // should be contexted switched from thread context to this
        // schedule function

        // Invoke different actual scheduling algorithms
        // according to policy (STCF or MLFQ)
        //if (sched == STCF)
	if (SCHED == 1)
                        sched_stcf();
        //else if (sched == MLFQ)
	else if(SCHED == 2)
                        sched_mlfq();

        // YOUR CODE HERE
}

//static mypthread_mutex_t; //Only one lock. But we can have multiple mutexes by a mutex array.
//---------------Thread-------------------
/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
	//printf("Thread Creating");
	if(!is_init){
		mypthread_init();
	}
	//Pause the timer to ensure thread_create is atomic.
	setitimer(ITIMER_VIRTUAL, timeout, cur);
	
	// create Thread Control Block
	tcb *new_thread = malloc(sizeof(tcb));
	thread_count++;
	//make pointer id point to thread. Then value it as tid. Then tid++;
	new_thread->id = tid;
	new_thread->is_init = 1;
	new_thread->status = READY;
	new_thread->elapse_time = 0;
	new_thread->priority = 0;
	new_thread->value_ptr = NULL;
	*thread = tid;
	thread_list[new_thread->id] = new_thread;
	tid++;

	new_thread->context = malloc(sizeof(ucontext_t));
	// create and initialize the context of this thread
	getcontext(new_thread->context);
	//We don't need a subsequent context, cuz those test functions end with pthread_exit(); 
	new_thread->context->uc_link = NULL;
	new_thread->context->uc_stack.ss_sp = malloc(STACK_SIZE);
	new_thread->context->uc_stack.ss_size = STACK_SIZE;
	new_thread->context->uc_stack.ss_flags = 0;

	makecontext(new_thread->context, (void*)function, 1, arg);
	printf("Thread %d created\n", new_thread->id);
	//Enqueue the thread
	enqueue(new_thread);
	
	setitimer(ITIMER_VIRTUAL, cur, NULL); //continue timer.
	return 0;
};

//Initialize everything necessary
void mypthread_init(){
	is_init = 1;
	thread_count = 1;
	//Initialize timer
	timer = malloc(sizeof(struct itimerval));
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 25;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 25;
	timeout = malloc(sizeof(struct itimerval));
        timeout->it_value.tv_sec = 0;
        timeout->it_value.tv_usec = 0;
        timeout->it_interval.tv_sec = 0;
        timeout->it_interval.tv_usec = 0;
        cur = malloc(sizeof(struct itimerval));

	//Initialize a pointer dict.
        for(int i=0; i<THREAD_CAP; i++){
                thread_list[i]=NULL;
		exit_value[i]=NULL;
        }

	for(int i=0;i<PRI_LEVEL;i++){
		queues[i]=NULL;
	}
	
	scheduler_init();

	//Create the first thread for main and make it as a running thread.
	tcb *main_thread = malloc(sizeof(tcb));
        thread_list[tid] = main_thread;
        //main_thread->id = main_thread_id;
	tid++;

	main_thread->status = READY;
	main_thread->is_init = 1;
	main_thread->elapse_time = 0;
	main_thread->priority = 0;
	main_thread->value_ptr = NULL;
        main_thread->context = malloc(sizeof(ucontext_t));
        getcontext(main_thread->context);

	//Enqueue main_thread
	enqueue(main_thread);
	
	//running_tid = main_thread->id;
	running_thread = main_thread;
	//Activate handler and timer
	signal(SIGVTALRM, signal_handler);
	setitimer(ITIMER_VIRTUAL, timer, NULL);
	
	return;
};


/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context
	running_thread->status = YIELD;
	//raise(SIGVTALRM);
	schedule();
	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	running_thread->status = EXIT;
	//raise(SIGVTALRM);
	schedule();
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread
	running_thread->status = BLOCKED;
	//thread_list[running_tid]->status = 1;
	while(!exit_list[thread]){
		//raise(SIGVTALRM);
		schedule();
	}
	
	//thread_list[running_tid]->status = 0;
	running_thread->status = READY;
	if(exit_value[thread]!=NULL){
		*value_ptr = exit_value[thread]; 
	}
	// YOUR CODE HERE
	return 0;
};

//---------------Mutex--------------------
/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex
	static uint mid;
	mutex->mutex_id = mid++;
	mutex->user_id = -1; //It means it is not occupied.
	mutex->is_locked = 0;
	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread
	if(mutex == NULL)
		return -1;
	
	running_thread->status = BLOCKED;
	while(mutex->is_locked){
		if(running_thread->id == mutex->user_id)
			break;
		if(mutex->user_id == -1) //It is locked but no thread is using it.
			break;

		//raise(SIGVTALRM);
		schedule();
	}

	running_thread->status = READY;
	//Get a mutex has to be atomic
	setitimer(ITIMER_VIRTUAL, timeout, cur);
	
	mutex->is_locked = 1;
	mutex->user_id = running_thread->id;

	setitimer(ITIMER_VIRTUAL, cur, NULL);
        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.
	if(mutex == NULL)
		return -1;
	if(mutex->is_locked == 0)
		return -1;
	if(mutex->user_id != running_thread->id)
		return -1;
	
	setitimer(ITIMER_VIRTUAL, timeout, cur);

	mutex->user_id = -1;
	mutex->is_locked = 0;
	
	setitimer(ITIMER_VIRTUAL, cur, NULL);
	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init
	if(mutex == NULL)
		return -1;
	if(mutex->is_locked)
		return -1;
	//We can free mutex, because mutex is not allocated dynamically.
	//free(mutex);
	
	//There is not dynamic memory in mutex so we don't need to do anything

	return 0;
};

void signal_handler(int signum){
	schedule();
}

void scheduler_init(){
	if (SCHED == 1){
		queues[0] = malloc(sizeof(Node));
		queues[0]->thread = NULL;
		queues[0]->priority = 1000;
		queues[0]->next = NULL;
		//printf("%d\n",queues[0]->priority);
        //else if (sched == MLFQ)
	}else if(SCHED == 2){
		int slice = QUANTUM;
		for(int i=0;i<PRI_LEVEL;i++){
			queues[i] = malloc(sizeof(Node));
			queues[i]->thread = NULL;
			queues[i]->priority = i;
			queues[i]->next = NULL;
			
			timer_list[i] = malloc(sizeof(struct itimerval));
			timer_list[i]->it_value.tv_sec = 0;
			timer_list[i]->it_value.tv_usec = slice;
			timer_list[i]->it_interval.tv_sec = 0;
			timer_list[i]->it_interval.tv_usec = slice;
			slice += ADDITION_TIME;
		}
	}

}

void enqueue(tcb * thread) {
	if (SCHED == 1){
		Node *node = malloc(sizeof(Node));
		node->thread = thread;
		node->priority = thread->elapse_time;
		//printf("%d\n",queues[0]->priority);
		Node *prev = queues[0];
		//Node *pointer = prev->next;
		while(prev->next != NULL){
			if(node->priority < prev->priority){
				prev = prev->next;
				//pointer = pointer->next;
			}else{
				break;
			}
		} //Until pointer is null or node->priority > pointer->priority
		if(prev->next!=NULL)
			node->next = prev->next;
		
		prev->next = node;
	}else if(SCHED == 2){
		Node *node = malloc(sizeof(Node));
		node->thread = thread;
		node->priority = thread->priority;
		
		if(queues[node->priority]->next != NULL)
			 node->next = queues[node->priority]->next;

		queues[node->priority]->next = node;
	}
}

tcb * dequeue() {
	if(thread_count<1){
		//If there is only one thread. It means queue is empty;
		return NULL;
	}
	
	if(SCHED == 1){
		if(queues[0]->next == NULL)
			return NULL;
		
		Node *node = queues[0];
		while(node->next->next != NULL){
			node = node->next;
		}

		tcb *thread = node->next->thread;
		//free() only invalid the memory but not pointer itself;
		free(node->next);
		node->next = NULL;

		return thread;
	}else if(SCHED == 2){
		for(int i = 0; i<PRI_LEVEL;i++){
			int isEmpty = 1;
			Node *prev = queues[i];
			Node *pointer = queues[i];
			while(pointer->next!=NULL){
				prev = pointer;
				pointer = pointer->next;
				isEmpty = 0;
			}

			if(!isEmpty){
				tcb *thread = pointer->thread;
				prev->next = NULL;
				free(pointer);
				return thread;
			}
		}
		return NULL;
	}
}

void boost_priority(){
	//from 1 to PRI_LEVEL-1, which means task of higher priority will be scheduled first
	for(int i=1;i<PRI_LEVEL;i++){
		//From queue's tail, it might be not fair but runs faster
		while(queues[i]->next != NULL){
			//Dequeue
			Node *temp = queues[i]->next;
			queues[i]->next = temp->next;
			
			//Enqueue queues[0]
			temp->next = queues[0]->next;
			queues[0]->next = temp;
			temp->thread->elapse_time = 0;
		}
	}
}
// Feel free to add any other functions you need

// YOUR CODE HERE


