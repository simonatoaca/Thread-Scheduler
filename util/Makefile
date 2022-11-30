CC = gcc
CFLAGS = -fPIC -Wall -Wextra -g

.PHONY: build
build: libscheduler.so

libscheduler.so: so_scheduler.o prio_queue.o queue.o linked_list.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

so_scheduler.o: so_scheduler.c
	$(CC) $(CFLAGS) -o $@ -c $<

prio_queue.o: prio_queue.c
	$(CC) $(CFLAGS) -o $@ -c $<

queue.o: queue.c
	$(CC) $(CFLAGS) -o $@ -c $<

linked_list.o: linked_list.c
	$(CC) $(CFLAGS) -o $@ -c $<


.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so