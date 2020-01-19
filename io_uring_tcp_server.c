#include "liburing.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>

#define MAX_CONNECTIONS 1024
#define BACKLOG 128

void add_poll(struct io_uring* ring, int fd, int type);
void add_socket_read(struct io_uring* ring, int fd, struct iovec* iovecs, int type);
void add_socket_write(struct io_uring* ring, int fd, struct iovec* iovecs, int type);

int POLL_LISTEN = 0, POLL_NEW_CONNECTION = 1, READ = 2, WRITE = 3;

typedef struct conn_info
{
    int fd;
    int type;
} conn_info;
conn_info conns[MAX_CONNECTIONS];

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please give a port number: ./io_uring_echo_server [port]\n");
        exit(0);
    } 

    // some variables we need
    int portno = strtol(argv[1], NULL, 10);
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);


    // create connection_info structs
    struct iovec iovecs[1024];
    char bufs[1024][1024];

    for (int i = 0; i < MAX_CONNECTIONS - 1; i++) {
        memset(&iovecs[i], 0, sizeof(iovecs[i]));
        memset(&bufs[i], 0, sizeof(bufs[i]));
        iovecs[i].iov_base = bufs[i];
        iovecs[i].iov_len = sizeof(bufs[i]);
    }


    // setup socket
    int sock_listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    const int val = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;


    
    if (bind(sock_listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error binding socket..\n");
        exit(-1);
    }
	if (listen(sock_listen_fd, BACKLOG) < 0) {
        perror("Error listening..\n");
        exit(-1);
    }
    printf("io_uring echo server listening for connections on port: %d\n", portno);


    // initialize io_uring
    struct io_uring ring;
    io_uring_queue_init(32, &ring, 0);


    // add first io_uring sqe, check when there will be data available on sock_listen_fd
    struct io_uring_sqe *sqe_init = io_uring_get_sqe(&ring);
    io_uring_prep_poll_add(sqe_init, sock_listen_fd, POLLIN);
    conn_info conn_i = 
    {
        .fd = sock_listen_fd, 
        .type = POLL_LISTEN
    };
    io_uring_sqe_set_data(sqe_init, &conn_i);


    // tell kernel we have put a sqe on the submission ring
    io_uring_submit(&ring);

    int type;
    
    while (1)
    {
        struct io_uring_cqe *cqe;

        // wait for new cqe to become available
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret != 0)
        {
            perror("Error io_uring_wait_cqe\n");
            exit(-1);
        }

        struct conn_info *user_data = (struct conn_info *)io_uring_cqe_get_data(cqe);
        type = user_data->type;

        if (type == POLL_LISTEN)
        {
            io_uring_cqe_seen(&ring, cqe);

            // io_uring_prep_accept(sqe, sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len, 0);
            // while loop until all connections are emptied using accept
            int sock_conn_fd;
            while ((sock_conn_fd = accept4(sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len, SOCK_NONBLOCK)) != -1) {
                add_poll(&ring, sock_conn_fd, POLL_NEW_CONNECTION);
            }
            
            // re-add poll sqe for listing socket and add new poll for newly connected socket
            add_poll(&ring, sock_listen_fd, POLL_LISTEN);
        }
        else if (type == POLL_NEW_CONNECTION)
        {
            io_uring_cqe_seen(&ring, cqe);
            add_socket_read(&ring, user_data->fd, iovecs, READ);
        }
        else if (type == READ)
        {
            if (cqe->res == 0) {
                shutdown(user_data->fd, 2);
                io_uring_cqe_seen(&ring, cqe);
            } else {
                // write to socket sqe
                io_uring_cqe_seen(&ring, cqe);
                add_socket_write(&ring, user_data->fd, iovecs, WRITE);
            }
        }
        else if (type == WRITE)
        {
            // read from socket completed, re-add poll sqe
            io_uring_cqe_seen(&ring, cqe);            
            add_poll(&ring, user_data->fd, POLL_NEW_CONNECTION);
        }

        io_uring_submit(&ring);
    }
  
}

void add_poll(struct io_uring* ring, int fd, int type) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_poll_add(sqe, fd, POLLIN);

    conn_info *conn_i_conn = calloc(1, sizeof(conn_info));
    conn_i_conn->fd = fd;
    conn_i_conn->type = type;

    io_uring_sqe_set_data(sqe, conn_i_conn);
}


void add_socket_read(struct io_uring* ring, int fd, struct iovec* iovecs, int type) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_readv(sqe, fd, &iovecs[fd], 1, 0);

    conn_info *conn_i = calloc(1, sizeof(conn_info));
    conn_i->fd = fd;
    conn_i->type = type;

    io_uring_sqe_set_data(sqe, conn_i);
}

void add_socket_write(struct io_uring* ring, int fd, struct iovec* iovecs, int type) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_writev(sqe, fd, &iovecs[fd], 1, 0);

    conn_info *conn_i = calloc(1, sizeof(conn_info));
    conn_i->fd = fd;
    conn_i->type = type;

    io_uring_sqe_set_data(sqe, conn_i);
}