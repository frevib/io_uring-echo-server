all:
	gcc -Wall -O2 -D_GNU_SOURCE -o io_uring_tcp_server.out io_uring_tcp_server.c -I./src/include/ -L./src/ -luring