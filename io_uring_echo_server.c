#include <liburing.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>

#define MAX_CONNECTIONS 4096
#define BACKLOG 512
#define MAX_MESSAGE_LEN 2048
#define IORING_FEAT_FAST_POLL (1U << 5)

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, unsigned flags);
void add_socket_read(struct io_uring* ring, int fd, size_t size, unsigned flags);
void add_socket_write(struct io_uring* ring, int fd, __u16 bid, size_t size, unsigned flags);
void add_provide_buf(struct io_uring *ring, __u16 bid);

enum {
    ACCEPT,
    READ,
    WRITE,
    PROV_BUF,
};

typedef struct conn_info
{
    __u32 fd;
    __u16 type;
    __u16 bid;
} conn_info;

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

    if (io_uring_queue_init_params(4096, &ring, &params) < 0)
    {
        perror("io_uring_init_failed...\n");
        exit(1);
    }

    if (!(params.features & IORING_FEAT_FAST_POLL))
    {
        printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }


    // prepare buffer selection

    // check if buffer selection is supported
    struct io_uring_probe *p;
    p = io_uring_get_probe_ring(&ring);
    if (!p || !io_uring_opcode_supported(p, IORING_OP_PROVIDE_BUFFERS))
    {
        fprintf(stdout, "Buffer select not supported, skipping\n");
        return 0;
    }
    free(p);

    // register buffers for buffer selection
    int buffers = 1000;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_provide_buffers(sqe, bufs, MAX_MESSAGE_LEN, buffers, 1337, 0);

    // request buffer selection is also async??
    io_uring_submit(&ring);

    io_uring_wait_cqe(&ring, &cqe);
    if (cqe->res < 0)
    {
        fprintf(stderr, "cqe->res=%d\n", cqe->res);
        return 1;
    }
    io_uring_cqe_seen(&ring, cqe);


    // add first accept sqe to monitor for new incoming connections
    add_accept(&ring, sock_listen_fd, (struct sockaddr *)&client_addr, &client_len, 0);

    // start event loop
    while (1)
    {
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

        // check how many cqe's are on the cqe ring at this moment
        struct io_uring_cqe *cqes[BACKLOG];
        int cqe_count = io_uring_peek_batch_cqe(&ring, cqes, sizeof(cqes) / sizeof(cqes[0]));

        // go through all the cqe's
        for (int i = 0; i < cqe_count; ++i)
        {
            struct io_uring_cqe *cqe = cqes[i];
            struct conn_info conn_i;
			memcpy(&conn_i, &cqe->user_data, sizeof(conn_i));
            // printf("cqe->res: %d\n", cqe->res);

            int type = conn_i.type;

            if (cqe->res == -ENOBUFS) {
                printf("bufs empty...\n");
                // io_uring_prep_provide_buffers(sqe, bufs[i], MAX_MESSAGE_LEN, 1, 1337, i);
            }

            if (type == PROV_BUF)
            {
                if (cqe->res < 0)
                {
                    fprintf(stderr, "cqe->res=%d\n", cqe->res);
                    return 1;
                }
                io_uring_cqe_seen(&ring, cqe);
            }
            else if (type == ACCEPT)
            {
                int sock_conn_fd = cqe->res;
                io_uring_cqe_seen(&ring, cqe);

                // new connected client; read data from socket and re-add accept to monitor for new connections
                add_socket_read(&ring, sock_conn_fd, MAX_MESSAGE_LEN, IOSQE_BUFFER_SELECT);
                add_accept(&ring, sock_listen_fd, (struct sockaddr *)&client_addr, &client_len, 0);
            }
            else if (type == READ)
            {
                int bytes_read = cqe->res;
                if (bytes_read <= 0)
                {
                    // no bytes available on socket, client must be disconnected
                    io_uring_cqe_seen(&ring, cqe);
                    shutdown(conn_i.fd, SHUT_RDWR);
                }
                else
                {
                    // bytes have been read into bufs, now add write to socket sqe

                    int bid = cqe->flags >> 16;
                    // printf("bid: %d\n", bid);

                    io_uring_cqe_seen(&ring, cqe);
                    add_socket_write(&ring, conn_i.fd, bid, bytes_read, 0);
                }
            }
            else if (type == WRITE)
            {
                // write to socket completed, re-add socket read
                io_uring_cqe_seen(&ring, cqe);

                add_provide_buf(&ring, conn_i.bid);
                add_socket_read(&ring, conn_i.fd, MAX_MESSAGE_LEN, IOSQE_BUFFER_SELECT);
            }
            // else if 
        }
        }
}

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, unsigned flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);
    io_uring_sqe_set_flags(sqe, flags);

    conn_info conn_i = {
        .fd = fd,
        .type = ACCEPT,
    };

    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

void add_socket_read(struct io_uring *ring, int fd, size_t size, unsigned flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, NULL, size, 0);
    io_uring_sqe_set_flags(sqe, flags);
    sqe->buf_group = 1337;

    conn_info conn_i = {
        .fd = fd,
        .type = READ,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

void add_socket_write(struct io_uring *ring, int fd, __u16 bid, size_t size, unsigned flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    // printf("buf: %s\n", bufs[bid]);
    io_uring_prep_send(sqe, fd, &bufs[bid], size, 0);
    io_uring_sqe_set_flags(sqe, flags);

    conn_info conn_i = {
        .fd = fd,
        .type = WRITE,
        .bid = bid,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}

void add_provide_buf(struct io_uring *ring, __u16 bid)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_provide_buffers(sqe, bufs[bid], MAX_MESSAGE_LEN, 1, 1337, bid);

    conn_info conn_i = {
        .fd = NULL,
        .type = PROV_BUF,
    };
    memcpy(&sqe->user_data, &conn_i, sizeof(conn_i));
}
