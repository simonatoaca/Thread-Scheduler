#include <stdio.h>
#include <semaphore.h>

#include "so_scheduler.h"
#include "prio_queue.h"
#include "linked_list.h"
#include "utils.h"

typedef struct {
    tid_t tid;
    unsigned int priority;
    so_handler *start_routine;
    int time_remaining;
    unsigned int preempted;
    sem_t run;
    sem_t planned;
} thread_t;

typedef struct {
    unsigned int is_running;    /* Scheduler state */
    unsigned int time_quantum;  /* Max time quantum for each thread */
    unsigned int io_number;     /* Max io devices supported */
    thread_t *running_thread;   /* The thread currently running */
    prio_queue_t *ready;        /* The queue for round robin implementation */
    linked_list_t *waiting;     /* The threads waiting for a signal */
    linked_list_t *thread_ids;  /* Thread ids - used for pthread_join */
} scheduler_t;

static scheduler_t scheduler;

int so_init(unsigned int time_quantum, unsigned int io) {
    /* Check params */
    if (!time_quantum || io > SO_MAX_NUM_EVENTS || scheduler.is_running) {
        return SCHED_INIT_ERR;
    }

    /* Initialize scheduler */
    scheduler.is_running = 1;
    scheduler.time_quantum = time_quantum;
    scheduler.io_number = io;
    scheduler.ready = pq_create(SO_MAX_PRIO + 1);
    if (!scheduler.ready) return SCHED_INIT_ERR;
    scheduler.waiting = ll_create();
    if (!scheduler.waiting) return SCHED_INIT_ERR;
    scheduler.thread_ids = ll_create();
    if (!scheduler.thread_ids) return SCHED_INIT_ERR;
    return 0;
}

void run_next_thread(thread_t *thread) {
    if (scheduler.running_thread == thread) {
        fprintf(stderr, "Start current thread %p\n", thread->tid);
        sem_post(&thread->run);
    } else {
        pq_enqueue(scheduler.ready, thread, thread->priority);
        thread_t *new_thread = pq_dequeue(scheduler.ready);
        fprintf(stderr, "Thread %p run next\n", new_thread->tid);
        sem_post(&new_thread->run);
        scheduler.running_thread = new_thread;
    }
}

void *start_thread(void *info) {
    thread_t *thread = (thread_t *)info;
    
    plan_thread(thread);

    fprintf(stderr, "Thread %p waiting\n", thread->tid);
    sem_wait(&thread->run);
    fprintf(stderr, "Thread %p stopped waiting\n", thread->tid);

    thread->start_routine(thread->priority);
    fprintf(stderr, "Thread %p finished\n", thread->tid);
}

void plan_thread(thread_t *thread) {
    if (!scheduler.running_thread) {
        fprintf(stderr, "This is the first thread %p\n", thread->tid);
        scheduler.running_thread = thread;
    } else if (scheduler.running_thread->priority < thread->priority) {
        fprintf(stderr, "This has a greater prio %p\n", thread->tid);
        pq_enqueue(scheduler.ready, scheduler.running_thread, scheduler.running_thread->priority);
        scheduler.running_thread = thread;
    } else {
        fprintf(stderr, "This is enqueued %p\n", thread->tid);
        pq_enqueue(scheduler.ready, thread, thread->priority);
    }
    sem_post(&thread->planned);
}

tid_t so_fork(so_handler *func, unsigned int priority) {
    fprintf(stderr, "SO FORK %p\n", pthread_self());
    /* Check params */
    if (!func || priority > SO_MAX_PRIO) {
        return INVALID_TID;
    }

    /* Initialize thread struct */
    thread_t *new_thread = malloc(sizeof(*new_thread));
    new_thread->priority = priority;
    new_thread->preempted = 0;
    new_thread->start_routine = func;

    sem_init(&new_thread->run, 0, 0);
    sem_init(&new_thread->planned, 0, 0);

    /* Start thread */
    if (pthread_create(&new_thread->tid, NULL, start_thread, new_thread)) {
        fprintf(stderr, "pthread_create error %d\n", errno);
    }

    ll_add_node(scheduler.thread_ids, new_thread->tid);

    sem_wait(&new_thread->planned);

    run_next_thread(new_thread);

    return new_thread->tid;
}

int so_wait(unsigned int io) {
    return 0;
}

int so_signal(unsigned int io) {
    return 0;
}

void so_exec(void) {

}

void so_end(void) {
    scheduler.is_running = 0;

    /* Wait for all threads to terminate */
    if (!ll_is_empty(scheduler.thread_ids)) {
        ll_node_t *curr_node = scheduler.thread_ids->head;
        while (curr_node) {
            tid_t thread_id = *((tid_t *)curr_node->info);
            //fprintf(stderr, "thread join %p\n\n", thread_id);
            if (pthread_join(thread_id, NULL)) {
                fprintf(stderr, "[pthread_join()] error %d\n", errno);
            }
            curr_node = curr_node->next;
        }
        free(scheduler.running_thread);
    }

    /* Free scheduler internals */
    pq_free(scheduler.ready);
    ll_free(scheduler.waiting, free);
    ll_free(scheduler.thread_ids, free);
}