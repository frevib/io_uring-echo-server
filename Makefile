CCFLAGS ?= -Wall -O2 -D_GNU_SOURCE -luring

all_targets = liburing io_uring_echo_server

all: clean $(all_targets)

clean:
	rm -f $(all_targets)

liburing:
	+$(MAKE) -C liburing-liburing-0.3/src

io_uring_echo_server:
	$(CC) io_uring_echo_server.c -o ./io_uring_echo_server -I./liburing-liburing-0.3/src/include/ -L./liburing-liburing-0.3/src/  -Wall -O2 -D_GNU_SOURCE -luring

