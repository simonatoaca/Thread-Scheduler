#include <stdio.h>
#include <semaphore.h>

#include "so_scheduler.h"
#include "prio_queue.h"
#include "linked_list.h"
#include "utils.h"

typedef enum {
	ALIVE,
	TERMINATED
} status_t;

typedef struct {
	tid_t tid;
	unsigned int priority;
	so_handler *start_routine;
	int time_remaining;
	status_t status;
	sem_t run;
	sem_t planned;
} thread_t;

typedef struct {
	queue_t *waiting_list;
} io_device_t;

typedef struct {
	unsigned int is_running;    /* Scheduler state */
	unsigned int time_quantum;  /* Max time quantum for each thread */
	unsigned int io_number;     /* Max io devices supported */
	thread_t *running_thread;   /* The thread currently running */
	prio_queue_t *ready;        /* The queue for round robin implementation */
	io_device_t *io_devices;    /* The supported io devices */
	linked_list_t *threads;     /* All threads - used for pthread_join */
	sem_t has_finished;         /* Used for waiting for all threads to terminate */
} scheduler_t;

static scheduler_t scheduler;

int so_init(unsigned int time_quantum, unsigned int io)
{
	/* Check params */
	if (!time_quantum || io > SO_MAX_NUM_EVENTS || scheduler.is_running)
		return SCHED_INIT_ERR;

	/* Initialize scheduler */
	scheduler.is_running = 1;
	scheduler.time_quantum = time_quantum;
	scheduler.io_number = io;
	scheduler.ready = pq_create(SO_MAX_PRIO + 1);

	if (!scheduler.ready)
		return SCHED_INIT_ERR;

	scheduler.threads = ll_create();

	if (!scheduler.threads)
		return SCHED_INIT_ERR;

	scheduler.io_devices = calloc(io, sizeof(io_device_t));

	if (!scheduler.io_devices)
		return SCHED_INIT_ERR;

	for (unsigned int i = 0; i < io; i++) {
		scheduler.io_devices[i].waiting_list = q_create();
		if (!scheduler.io_devices[i].waiting_list) {
			so_end();
			exit(ENOMEM);
		}
	}

	sem_init(&scheduler.has_finished, 0, 0);

	return 0;
}

void run_next_thread(void)
{
	/* Check if the last thread ended -> the scheduler can also end */
	if (pq_is_empty(scheduler.ready) && scheduler.running_thread->status == TERMINATED) {
		sem_post(&scheduler.has_finished);
		return;
	}

	/* Get next thread planned */
	ll_node_t *thread_node = pq_dequeue(scheduler.ready);
	thread_t *new_thread = thread_node->info;

	free(thread_node);

	/* Run the planned thread */
	scheduler.running_thread = new_thread;
	scheduler.running_thread->status = ALIVE;
	scheduler.running_thread->time_remaining = scheduler.time_quantum;
	sem_post(&new_thread->run);
}

void plan_thread(thread_t *thread)
{
	if (!scheduler.running_thread) {
		/* This is the first forked thread */
		scheduler.running_thread = thread;
	} else if (scheduler.running_thread->priority < thread->priority) {
		/* Plan current thread and then set the greater prio thread as the current one */
		thread_t *current_thread = scheduler.running_thread;

		plan_thread(current_thread);
		sem_wait(&current_thread->planned);

		scheduler.running_thread = thread;
	} else {
		pq_enqueue(scheduler.ready, thread, thread->priority);
	}

	/* Signal that the thread was planned */
	sem_post(&thread->planned);
}

void *start_thread(void *info)
{
	thread_t *thread = (thread_t *)info;

	plan_thread(thread);

	sem_wait(&thread->run);

	thread->start_routine(thread->priority);

	thread->status = TERMINATED;
	run_next_thread();
	return NULL;
}

void preempt_thread(thread_t *thread)
{
	plan_thread(thread);
	sem_wait(&thread->planned);

	run_next_thread();

	sem_wait(&thread->run);
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	/* Check params */
	if (!func || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/* Initialize thread struct */
	thread_t *new_thread = calloc(1, sizeof(*new_thread));

	if (!new_thread) {
		fprintf(stderr, "Failed thread allocation\n");
		so_end();
		exit(ENOMEM);
	}

	new_thread->priority = priority;
	new_thread->start_routine = func;
	new_thread->status = ALIVE;
	new_thread->time_remaining = scheduler.time_quantum;

	sem_init(&new_thread->run, 0, 0);
	sem_init(&new_thread->planned, 0, 0);

	ll_add_node(scheduler.threads, new_thread);

	thread_t *old_thread = scheduler.running_thread;

	/* Start thread */
	if (pthread_create(&new_thread->tid, NULL, start_thread, new_thread)) {
		fprintf(stderr, "pthread_create error %d\n", errno);
		so_end();
		exit(errno);
	}

	sem_wait(&new_thread->planned);

	/* Decrement time quantum for the thread that called so fork */
	if (old_thread)
		old_thread->time_remaining--;

	/*  Start the forked thread now if it is the case.
	 *  Also handle preemption.
	 */
	if (scheduler.running_thread == new_thread) {
		sem_post(&scheduler.running_thread->run);

		/* Preemption of the old thread */
		if (old_thread)
			sem_wait(&old_thread->run);
	} else {
		/* Preempt the thread that called so fork if time quantum expired */
		if (old_thread && old_thread->time_remaining <= 0)
			preempt_thread(old_thread);
	}

	return new_thread->tid;
}

int so_wait(unsigned int io)
{
	if (io >= scheduler.io_number)
		return WAIT_ERR;

	scheduler.running_thread->time_remaining--;

	/* Add the calling thread to the waiting list */
	thread_t *current_thread = scheduler.running_thread;

	q_enqueue(scheduler.io_devices[io].waiting_list, current_thread);

	/* Preemption */
	run_next_thread();
	sem_wait(&current_thread->run);

	return WAIT_SUCCESS;
}

int so_signal(unsigned int io)
{
	if (io >= scheduler.io_number)
		return SIGNAL_ERR;

	if (q_is_empty(scheduler.io_devices[io].waiting_list))
		return NO_THREADS;

	scheduler.running_thread->time_remaining--;

	thread_t *current_thread = scheduler.running_thread;

	int woken_threads = 0;

	/* Wake all threads that were waiting for io */
	while (!q_is_empty(scheduler.io_devices[io].waiting_list)) {
		ll_node_t *node = q_dequeue(scheduler.io_devices[io].waiting_list);
		thread_t *thread = node->info;

		woken_threads++;

		plan_thread(thread);

		free(node);
	}

	/* A thread with a higher prio entered the sched */
	if (scheduler.running_thread != current_thread) {
		sem_post(&scheduler.running_thread->run);
		sem_wait(&current_thread->run);
	} else if (current_thread->time_remaining <= 0) {
		preempt_thread(current_thread);
	}

	return woken_threads;
}

void so_exec(void)
{
	scheduler.running_thread->time_remaining--;

	thread_t *current_thread = scheduler.running_thread;

	if (current_thread->time_remaining <= 0)
		preempt_thread(current_thread);
}

void free_thread(ll_node_t *thread_node)
{
	if (!thread_node)
		return;

	thread_t *thread = thread_node->info;

	sem_destroy(&thread->run);
	sem_destroy(&thread->planned);

	free(thread);
	free(thread_node);
}

void so_end(void)
{
	scheduler.is_running = 0;

	/* Wait for all threads to terminate */
	if (!ll_is_empty(scheduler.threads)) {
		sem_wait(&scheduler.has_finished);
		ll_node_t *curr_node = scheduler.threads->head;

		while (curr_node) {
			thread_t *thread = curr_node->info;

			if (pthread_join(thread->tid, NULL))
				fprintf(stderr, "pthread_join() error %d\n", errno);
			curr_node = curr_node->next;
		}
	}

	/* Free scheduler internals */
	pq_free(scheduler.ready, free_thread);
	ll_free(scheduler.threads, free_thread);

	for (unsigned int i = 0; i < scheduler.io_number; i++)
		q_free(scheduler.io_devices[i].waiting_list, free_thread);

	free(scheduler.io_devices);
	sem_destroy(&scheduler.has_finished);
}

