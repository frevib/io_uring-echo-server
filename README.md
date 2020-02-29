# io_uring bare minimum echo server
* uses an event loop created with io_uring
* uses liburing 0.3
* Linux 5.4 or higher needed. (Lower versions don't return the right amount of bytes read from `io_uring_prep_readv` in `cqe->res`).
* **For much better performance**, go here: https://github.com/frevib/io_uring-echo-server/tree/io-uring-feat-fast-poll

## Install and run
`make liburing`

`make io_uring_echo_server`

`./io_uring_echo_server [port_number]`

## compare with epoll echo server
https://github.com/frevib/epoll-echo-server


## Benchmarks
https://github.com/CarterLi/io_uring-echo-server


## Versions

### v1.4
Fixed bug that massively overstated the performance.

### v1.3
Use pre-allocated `sqe->user_data` instead of dynamically allocating memory.

### v1.1
Fix memory leak.

### v1.0
Working release.