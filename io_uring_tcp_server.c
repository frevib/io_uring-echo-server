#include "liburing.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>

int main(int argc, char *argv[]) {

    // some variables
    int portno = strtol(argv[1], NULL, 10);
    struct sockaddr_in serv_addr, client_addr;
    int client_len = sizeof(client_addr);

    // setup socket
    int sock_listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    const int val = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;


    // bind socket and listen for connections
    if (bind(sock_listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error binding socket\n");
        exit(-1);
    }
	if (listen(sock_listen_fd, 128) < 0) {
        perror("Error listening\n");
        exit(-1);
    }
    printf("listening for connections on port: %d\n", portno);


    // initialize io_uring
    struct io_uring ring;
    io_uring_queue_init(32, &ring, 0);

    // add first io_uring sqe, check when there will be data available on sock_listen_fd
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_poll_add(sqe, sock_listen_fd, POLLIN);

    // tell kernel we have put a sqe on the submission ring
    io_uring_submit(&ring);

    // if incoming data then check if it is a new connection (listen_fd == the_fd_that_was_in_the_cqe)

    const int LISTEN = 0, ECHO_RECV = 1, ECHO_SEND = 2, ECHO = 3;
    struct io_uring_cqe *cqe;
    

    while (1)
    {
        int ret = io_uring_wait_cqe(&ring, &cqe);

        if (ret != 0)
        {
            perror("Error io_uring_wait_cqe");
        }

        // new data is available on one of the sockets
        if ((cqe->res & POLLIN) == POLLIN)
        {
            printf("new connection, res: %d\n", cqe->res);

            // io_uring_prep_accept(sqe, sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len, 0);
            // while loop until all connections are emptied using accept
            int sock_conn_fd = accept4(sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len, SOCK_NONBLOCK);
            printf("new connection on fd: %d\n", sock_conn_fd);
            if (sock_conn_fd == -1)
            {
                perror("Accept socket error\n");
            }

            io_uring_cqe_seen(&ring, cqe);

            sqe = io_uring_get_sqe(&ring);
            io_uring_prep_poll_add(sqe, sock_listen_fd, POLLIN);

            // struct io_uring_sqe *sqe_conn = io_uring_get_sqe(&ring);
            // io_uring_prep_poll_add(sqe_conn, sock_conn_fd, POLLIN);
            io_uring_submit(&ring);
        }
    }
    
}