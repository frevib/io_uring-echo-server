# io_uring bare minimum echo server
* uses an event loop created with io_uring
* uses liburing HEAD https://github.com/axboe/liburing
* __Linux 5.6 or higher with IORING_FEAT_FAST_POLL and IORING_OP_PROVIDE_BUFFERS required__ (available in https://git.kernel.dk/cgit/linux-block/?h=io_uring-task-poll and https://git.kernel.dk/cgit/linux-block/log/?qt=grep&q=select&h=io_uring-buf-select).


## Install and run
`make liburing`

`make io_uring_echo_server`

`./io_uring_echo_server [port_number]`

## compare with epoll echo server
https://github.com/frevib/epoll-echo-server


## Benchmarks
https://github.com/frevib/io_uring-echo-server/blob/io-uring-feat-fast-poll/benchmarks/benchmarks.md



## Versions

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