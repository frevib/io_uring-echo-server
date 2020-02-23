CCFLAGS ?= -Wall -O2 -D_GNU_SOURCE -luring

all_targets = liburing io_uring_echo_server

.PHONY all: clean $(all_targets)

clean:
	rm -f $(all_targets)

liburing:
	+$(MAKE) -C ./liburing-0.3

io_uring_echo_server:
	$(CC) io_uring_echo_server.c -o ./io_uring_echo_server -I./liburing-0.3/src/include/ -L./liburing-0.3/src/ $(CCFLAGS)

