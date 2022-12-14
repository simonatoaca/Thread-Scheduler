## Thread Scheduler - SO Homework 2
### Copyright 2022 Toaca Alexandra Simona

### Implementation
- The basic idea behind the scheduler is that every thread gets its
**semaphore** and it is initially in a **waiting state**
(before the routine is even called).
- This is done by using an auxiliary function (```start_thread```)
which is used in ```pthread_create```. When the thread ends, the
next one is started

> void *start_thread(void *info)
> {
>	thread_t *thread = (thread_t *)info;
>
>	plan_thread(thread);
>
>	sem_wait(&thread->run);
>
>	thread->start_routine(thread->priority);
>
>	thread->status = TERMINATED;
>	run_next_thread();
>	return NULL;
>}

- The scheduler plans every thread that enters the program (decides
whether to enqueue it or start it immediately) - function ```plan_thread```
- ```plan_thread``` signals to a semaphore (placed before the next action is taken)
that the thread has been planned
- To start the thread immediately, the one that is currently running is
**preempted** (the semaphore is put again in a waiting state - sem_wait)
and the new thread is signaled to start (sem_post)
- Preemption is also done when the time quantum of the current thread expires,
in which case the next thread is run (next in the queue)- ```run_next_thread```
- There is a structure which holds queues for the threads that are waiting
for a certain event. Every queue coresponds to the id of the event.
```io_devices[id].waiting_list```
- a third type of semaphore is the one that waits for all threads to
finish their job before calling ```pthread_join``` in ```so_end()```

### What I have learned
- Working with threads. New concept, new way of thinking, hard to debug.
Before introducing the semaphore which waits for all threads to terminate,
```pthread join``` in ```so_end``` was ending threads prematurely.
- I read extra on mutexes, condition variables and semaphores. Then decided
that semaphores where the right fit for this homework.

### Comments
- Debugging when multiple threads are runnning is tedious and difficult
- I believe this homework is good is terms of understading how a scheduler
works, but it is hard to do when one has never worked with threads before

### How to use
- the makefile generates the dynamic library ```so_scheduler.so``` which
is then linked with the test program.