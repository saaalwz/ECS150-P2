# Target library
lib := libuthread.a

all: $(lib)

## TODO
CFLAGS = -Wall -Wextra -Werror -c -g

%.o: %.c context.h
	gcc $(CFLAGS) $<

%.o: %.c preempt.h
	gcc $(CFLAGS) $<

%.o: %.c queue.h
	gcc $(CFLAGS) $<

%.o: %.c uthread.h
	gcc $(CFLAGS) $<

libuthread.a: context.o preempt.o queue.o uthread.o
	ar rcs libuthread.a $^

clean:
	rm -f *.o *.a
