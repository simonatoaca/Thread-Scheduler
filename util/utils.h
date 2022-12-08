#ifndef __UTILS_H_
#define __UTILS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define SCHED_INIT_ERR -1
#define THREAD_FORK_ERR -2
#define WAIT_ERR -3
#define WAIT_SUCCESS 0
#define SIGNAL_ERR -4
#define NO_THREADS 0

/* useful macro for handling error codes */
#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(errno);                                                       \
		}                                                                      \
	} while (0)

#endif /* __UTILS_H_ */
