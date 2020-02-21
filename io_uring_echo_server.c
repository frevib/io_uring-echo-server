#include <liburing.h>
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
#define MAX_MESSAGE_LEN 2048

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, int flags);
void add_poll(struct io_uring* ring, int fd, int type);
void add_socket_read(struct io_uring* ring, int fd, size_t size, unsigned flags);
void add_socket_write(struct io_uring* ring, int fd, size_t size, unsigned flags);

enum {
    ACCEPT,
    POLL_LISTEN,
    POLL_NEW_CONNECTION,
    READ,
    WRITE,
};

typedef struct conn_info
{
    int fd;
    int type;
} conn_info;

conn_info conns[MAX_CONNECTIONS];
char bufs[MAX_CONNECTIONS][MAX_MESSAGE_LEN];

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Please give a port number: ./io_uring_echo_server [port]\n");
        exit(0);
    }

    // some variables we need
    int portno = strtol(argv[1], NULL, 10);
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);


    // setup socket
    int sock_listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    const int val = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;


    // bind and listen
    if (bind(sock_listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error binding socket..\n");
        exit(1);
    }
    if (listen(sock_listen_fd, BACKLOG) < 0)
    {
        perror("Error listening..\n");
        exit(1);
    }
    printf("io_uring echo server listening for connections on port: %d\n", portno);


    // initialize io_uring
    struct io_uring_params params;
    struct io_uring ring;
    memset(&params, 0, sizeof(params));
    params.features = (1U << 5);

    io_uring_queue_init_params(BACKLOG, &ring, &params);
    if (!(params.features & (1U << 5))) {
		fprintf(stdout, "IORING_FEAT_FAST_POLL not there, skipping\n");
		return 0;
	}

    
    // io_uring_queue_init(BACKLOG, &ring, 0);


    // add first io_uring poll sqe, to check when there will be data available on sock_listen_fd
    struct io_uring_sqe *sqe_init = io_uring_get_sqe(&ring);
    io_uring_prep_poll_add(sqe_init, sock_listen_fd, POLLIN);
    conn_info conn_i =
    {
        .fd = sock_listen_fd,
        .type = POLL_LISTEN
    };
    io_uring_sqe_set_data(sqe_init, &conn_i);

    while (1)
    {
        // printf("begin of while loop..\n");
        struct io_uring_cqe *cqe;
        int ret;

        // tell kernel we have put a sqe on the submission ring
        io_uring_submit(&ring);

        // wait for new cqe to become available
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret != 0)
        {
            perror("Error io_uring_wait_cqe\n");
            exit(1);
        }

        // check how many cqe's are on the cqe ring, and put these cqe's in an array
        struct io_uring_cqe *cqes[BACKLOG];
        int cqe_count = io_uring_peek_batch_cqe(&ring, cqes, sizeof(cqes) / sizeof(cqes[0]));

        // go through all the cqe's
        for (int i = 0; i < cqe_count; ++i)
        {
            struct io_uring_cqe *cqe = cqes[i];
            struct conn_info *user_data = (struct conn_info *)io_uring_cqe_get_data(cqe);
            int type = user_data->type;

            if (type == POLL_LISTEN)
            {
                io_uring_cqe_seen(&ring, cqe);

                // io_uring_prep_accept(sqe, sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len, 0);
                // while loop until all connections are emptied using accept
                int sock_conn_fd;
                while ((sock_conn_fd = accept(sock_listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len)) != -1)
                {
                    //  add read sqe for newly connected socket
                    add_socket_read(&ring, sock_conn_fd, MAX_MESSAGE_LEN, 0);

                }

                // re-add poll sqe for listing socket
                add_poll(&ring, sock_listen_fd, POLL_LISTEN);
            }
            else if (type == READ)
            {
                int bytes_read = cqe->res;
                if (bytes_read <= 0)
                {
                    // no bytes available on socket, client must be disconnected
                    io_uring_cqe_seen(&ring, cqe);
                    shutdown(user_data->fd, SHUT_RDWR);
                }
                else
                {
                    // bytes have been read into iovec, add write to socket sqe
                    io_uring_cqe_seen(&ring, cqe);
                    add_socket_write(&ring, user_data->fd, bytes_read, 0);
                }
            }
            else if (type == WRITE)
            {
                // write to socket completed, re-add poll sqe
                io_uring_cqe_seen(&ring, cqe);
                add_socket_read(&ring, user_data->fd, MAX_MESSAGE_LEN, 0);
            }
        }
    }
}

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, int flags) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    // conn_info *conn_i = &conns[fd];
	// struct conn_info conn_i = {
	// 	.fd = fd,
	// 	.type = ACCEPT,
	// };

	io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);

    conn_info *conn_i = &conns[fd];
    conn_i->fd = fd;
    conn_i->type = ACCEPT;

    io_uring_sqe_set_data(sqe, conn_i);
}

void add_poll(struct io_uring *ring, int fd, int type)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_poll_add(sqe, fd, POLLIN);

    conn_info *conn_i = &conns[fd];
    conn_i->fd = fd;
    conn_i->type = type;

    io_uring_sqe_set_data(sqe, conn_i);
}

void add_socket_read(struct io_uring *ring, int fd, size_t size, unsigned flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	io_uring_prep_recv(sqe, fd, &bufs[fd], size, MSG_NOSIGNAL);
    io_uring_sqe_set_flags(sqe, flags);

    conn_info *conn_i = &conns[fd];
    conn_i->fd = fd;
    conn_i->type = READ;

    io_uring_sqe_set_data(sqe, conn_i);
}

void add_socket_write(struct io_uring *ring, int fd, size_t size, unsigned flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, &bufs[fd], size, MSG_NOSIGNAL);
    io_uring_sqe_set_flags(sqe, flags);

    conn_info *conn_i = &conns[fd];
    conn_i->fd = fd;
    conn_i->type = WRITE;

    io_uring_sqe_set_data(sqe, conn_i);
}
