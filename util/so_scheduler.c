#include <stdio.h>
#include <semaphore.h>

#include "so_scheduler.h"
#include "prio_queue.h"
#include "linked_list.h"
#include "utils.h"

typedef enum {
    NEW,
    READY,
    WAITING,
    RUNNING,
    TERMINATED
} status_t;

typedef struct {
    tid_t tid;
    unsigned int priority;
    so_handler *start_routine;
    status_t status;
    sem_t run;
} thread_t;

typedef struct {
    thread_t *thread;
    unsigned int start_now;
} thread_start_info_t;

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
    scheduler.ready = pq_create(sizeof(thread_t), SO_MAX_PRIO + 1);
    if (!scheduler.ready) return SCHED_INIT_ERR;
    scheduler.waiting = ll_create(sizeof(thread_t));
    if (!scheduler.waiting) return SCHED_INIT_ERR;
    scheduler.thread_ids = ll_create(sizeof(tid_t));
    if (!scheduler.thread_ids) return SCHED_INIT_ERR;
    fprintf(stderr, "SO INIT\n");
    return 0;
}

void *start_thread(void *info) {
    unsigned int start_now = ((thread_start_info_t *)info)->start_now;
    thread_t *thread = ((thread_start_info_t *)info)->thread;
    if (!thread) {
        fprintf(stderr, "st now %d\n", start_now);
    }
    if (!start_now) {
        fprintf(stderr, "start thread waiting\n");
        /* Waits for the semaphore before starting */
        sem_wait(&thread->run);
        fprintf(stderr, "start thread stopped waiting\n");
    }
    thread->start_routine(thread->priority);
    free(info);
    return 0;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
    /* Check params */
    if (!func || priority > SO_MAX_PRIO) {
        return INVALID_TID;
    }

    /* Initialize thread struct */
    thread_t *new_thread = malloc(sizeof(*new_thread));
    new_thread->priority = priority;
    new_thread->status = NEW;
    new_thread->start_routine = func;
    sem_init(&new_thread->run, 0, 0);   

    fprintf(stderr, "finished initializing the thread\n");

    unsigned int start_now = scheduler.running_thread ? 0 : 1;
    thread_start_info_t *thread_info = malloc(sizeof(*thread_info));
    thread_info->start_now = start_now;
    thread_info->thread = new_thread;

    /* Start thread */
    if (pthread_create(&new_thread->tid, NULL, start_thread, thread_info)) {
        fprintf(stderr, "[pthread_create] error %d\n", errno);
    }

    fprintf(stderr, "finished starting the thread\n");
    scheduler.running_thread = new_thread;
    ll_add_nth_node(scheduler.thread_ids, 0, &new_thread->tid);

    // if (scheduler.running_thread->priority < new_thread->priority) {
    //     // DEOCAMDATA E PRIMUL THREAD
    //     sem_post(&new_thread->run);
    //     fprintf(stderr, "start the fucking thread\n");
    // }

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
    int n_threads = ll_get_size(scheduler.thread_ids);
    for (int i = 0; i < n_threads; i++) {
        fprintf(stderr, "Thread join\n");
        ll_node_t *node = ll_remove_nth_node(scheduler.thread_ids, 0);
        tid_t thread_id = *((tid_t *)node->data);
        free(node->data);
        free(node);
        fprintf(stderr, "thread %p\n", thread_id);
        if (pthread_join(thread_id, NULL)) {
            fprintf(stderr, "[pthread_join()] error %d\n", errno);
        }
    }

    /* Free scheduler internals */
    pq_free(scheduler.ready);
    ll_free(&scheduler.waiting);
    ll_free(&scheduler.thread_ids);
}