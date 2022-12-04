#include <stdio.h>
#include <semaphore.h>

#include "so_scheduler.h"
#include "prio_queue.h"
#include "linked_list.h"
#include "utils.h"

typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} status_t;

typedef struct {
    tid_t tid;
    unsigned int priority;
    so_handler *start_routine;
    int time_remaining;
    unsigned int preempted;
    status_t status;
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
    linked_list_t *threads;     /* All threads - used for pthread_join */
    sem_t has_finished;         /* Used for waiting for all threads to terminate */
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
    scheduler.threads = ll_create();
    if (!scheduler.threads) return SCHED_INIT_ERR;

    sem_init(&scheduler.has_finished, 0, 0);

    return 0;
}

void run_next_thread() {
    /* Check if the last thread ended -> the scheduler can also end */
    if (pq_is_empty(scheduler.ready) && scheduler.running_thread->status == TERMINATED) {
        if (sem_post(&scheduler.has_finished)) {
            fprintf(stderr, "sem post at %s error %d\n", __LINE__, errno);
        }
        return;
    }

    /* Get next thread planned */
    ll_node_t *thread_node = pq_dequeue(scheduler.ready);
    thread_t *new_thread = thread_node->info;
    free(thread_node);
    
    fprintf(stderr, "Thread %p run next\n", new_thread->tid);

    /* Run the planned thread */
    scheduler.running_thread = new_thread;
    scheduler.running_thread->preempted = 0;
    scheduler.running_thread->status = RUNNING;
    scheduler.running_thread->time_remaining = scheduler.time_quantum;
    if (sem_post(&new_thread->run)) {
        fprintf(stderr, "sem post at %s error %d\n", __LINE__, errno);
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
    thread->status = TERMINATED;
    run_next_thread();
}

void plan_thread(thread_t *thread) {
    if (!scheduler.running_thread) {
        fprintf(stderr, "This is the first thread/is the only one %p\n", thread->tid);
        scheduler.running_thread = thread;
    } else if (scheduler.running_thread->priority < thread->priority) {
        fprintf(stderr, "This has a greater prio %p\n", thread->tid);

        /* Plan current thread and then set the greater prio thread as the current one */
        thread_t *current_thread = scheduler.running_thread;
        plan_thread(current_thread);
        sem_wait(&current_thread->planned);

        scheduler.running_thread = thread;
    } else {
        fprintf(stderr, "This is enqueued %p prio %d\n", thread->tid, thread->priority);
        pq_enqueue(scheduler.ready, thread, thread->priority);
    }

    /* Signal that the thread was planned */
    if (sem_post(&thread->planned)) {
        fprintf(stderr, "sem post at %s error %d\n", __LINE__, errno);
    }
}

tid_t so_fork(so_handler *func, unsigned int priority) {
    fprintf(stderr, "SO FORK %p\n", pthread_self());
    /* Check params */
    if (!func || priority > SO_MAX_PRIO) {
        return INVALID_TID;
    }

    /* Initialize thread struct */
    thread_t *new_thread = calloc(1, sizeof(*new_thread));
    new_thread->priority = priority;
    new_thread->preempted = 0;
    new_thread->start_routine = func;
    new_thread->status = READY;

    sem_init(&new_thread->run, 0, 0);
    sem_init(&new_thread->planned, 0, 0);

    ll_add_node(scheduler.threads, new_thread);

    thread_t *old_thread = scheduler.running_thread;

    /* Decrement time quantum for the current thread*/
    if (scheduler.running_thread) {
        scheduler.running_thread->time_remaining--;
    }

    /* Start thread */
    if (pthread_create(&new_thread->tid, NULL, start_thread, new_thread)) {
        fprintf(stderr, "pthread_create error %d\n", errno);
    }

    if (sem_wait(&new_thread->planned)) {
        fprintf(stderr, "sem post at %s error %d\n", __LINE__, errno);
    }

    /*  Start the forked thread now if it is the case.
        Also handle preemption.
    */
    if (scheduler.running_thread == new_thread) {
        fprintf(stderr, "Start thread cuz it's first\n");
        sem_post(&scheduler.running_thread->run);

        /* Preemption of the old thread */
        if (old_thread) {
            sem_wait(&old_thread->run);
        }
    }
    
    fprintf(stderr, "SO FORK %p RETURNED\n", pthread_self());
    return new_thread->tid;
}

int so_wait(unsigned int io) {
    return 0;
}

int so_signal(unsigned int io) {
    return 0;
}

void so_exec(void) {
    fprintf(stderr, "SO_EXEC %p\n", scheduler.running_thread->tid);
    if (scheduler.running_thread) {
        scheduler.running_thread->time_remaining--;
    }

    if (scheduler.running_thread && scheduler.running_thread->time_remaining <= 0) {
        fprintf(stderr, "%p PREEMPTED\n", scheduler.running_thread->tid);
        scheduler.running_thread->preempted = 1;
        scheduler.running_thread->status = READY;

        thread_t *current_thread = scheduler.running_thread;

        plan_thread(current_thread);
        sem_wait(&current_thread->planned);

        run_next_thread();

        sem_wait(&current_thread->run);
    }
}

void free_thread(ll_node_t *thread_node) {
    if (!thread_node)
        return;
    free(thread_node->info);
    free(thread_node);
}

void so_end(void) {
    scheduler.is_running = 0;

    /* Wait for all threads to terminate */
    if (!ll_is_empty(scheduler.threads)) {
        sem_wait(&scheduler.has_finished);
        ll_node_t *curr_node = scheduler.threads->head;
        while (curr_node) {
            thread_t *thread = curr_node->info;
            fprintf(stderr, "thread join %p\n\n", thread->tid);
            if (pthread_join(thread->tid, NULL)) {
                fprintf(stderr, "pthread_join() error %d\n", errno);
            }
            curr_node = curr_node->next;
        }
    }

    /* Free scheduler internals */
    pq_free(scheduler.ready, free_thread);
    ll_free(scheduler.waiting, free_thread);
    ll_free(scheduler.threads, free_thread);
}