all:
	gcc -Wall -O2 -D_GNU_SOURCE io_uring_echo_server.c -o ./io_uring_echo_server -I./src/include/ -L./src/ -luring