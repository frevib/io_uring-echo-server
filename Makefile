CCFLAGS ?= -Wall -O2 -D_GNU_SOURCE -luring

all: liburing io_uring_echo_server

liburing:
	+$(MAKE) -C liburing-liburing-0.3/src

io_uring_echo_server:
	$(CC)  io_uring_echo_server.c -o ./io_uring_echo_server -I./liburing-liburing-0.3/src/include/ -L./liburing-liburing-0.3/src/

