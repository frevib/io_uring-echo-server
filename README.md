# io_uring bare minimum echo server
* uses an event loop created with io_uring
* uses liburing https://github.com/axboe/liburing
* __Linux 5.7 or higher with IORING_FEAT_FAST_POLL and IORING_OP_PROVIDE_BUFFERS required__


## Install and run
#### First install the latest liburing:

`git clone https://github.com/axboe/liburing`

`./configure`

`make`

`make install`


#### Then install io_uring-echo-server:

`git clone https://github.com/frevib/io_uring-echo-server.git`

`make io_uring_echo_server`

`./io_uring_echo_server [port_number]`

## compare with epoll echo server
https://github.com/frevib/epoll-echo-server


## Benchmarks (without buffer selection)
https://github.com/frevib/io_uring-echo-server/blob/io-uring-feat-fast-poll/benchmarks/benchmarks.md



## Versions

### v1.7
* Use latest liburing from https://github.com/axboe/liburing

### v1.6
* Use IORING_OP_PROVIDE_BUFFERS

### v1.5
* Use IORING_FEAT_FAST_POLL, which increases performance a lot
* Remove all polling, this is now handled by IORING_FEAT_FAST_POLL

### v1.4
Fix bug that massively overstated the performance.

### v1.3
Use pre-allocated `sqe->user_data` instead of dynamically allocating memory.

### v1.1
Fix memory leak.

### v1.0
Working release.
