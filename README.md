# io_uring bare minimum echo server
* uses an event loop created with io_uring
* uses liburing HEAD https://github.com/axboe/liburing
* Linux 5.6 or higher with IORING_FEAT_FAST_POLL (available in https://git.kernel.dk/cgit/linux-block/?h=io_uring-task-poll) needed.

IORING_FEAT_FAST_POLL will be available from Linux 5.7

## Install and run
`make liburing`

`make io_uring_echo_server`

`./io_uring_echo_server [port_number]`

## compare with epoll echo server
https://github.com/frevib/epoll-echo-server


## Benchmarks
#### IORING_FEAT_FAST_POLL
--number 100 --duration 60 --length 512
    Finished release [optimized] target(s) in 0.03s
     Running `target/release/echo_bench --address 'localhost:6666' --number 100 --duration 60 --length 512`
Benchmarking: localhost:6666
100 clients, running 512 bytes, 60 sec.

Speed: 80223 request/sec, 80223 response/sec
Requests: 4813380
Responses: 4813380


#### poll then send/recv
--number 100 --duration 60 --length 512
    Finished release [optimized] target(s) in 0.05s
     Running `target/release/echo_bench --address 'localhost:6666' --number 100 --duration 60 --length 512`
Benchmarking: localhost:6666
100 clients, running 512 bytes, 60 sec.

Speed: 64808 request/sec, 64808 response/sec
Requests: 3888535
Responses: 3888533


#### epoll
-number 100 --duration 60 --length 512
    Finished release [optimized] target(s) in 0.01s
     Running `target/release/echo_bench --address 'localhost:7777' --number 100 --duration 60 --length 512`
Benchmarking: localhost:7777
100 clients, running 512 bytes, 60 sec.

Speed: 65624 request/sec, 65624 response/sec
Requests: 3937467
Responses: 3937467



## Versions

### v1.4
Fixed bug that massively overstated the performance.

### v1.3
Use pre-allocated `sqe->user_data` instead of dynamically allocating memory.

### v1.1
Fix memory leak.

### v1.0
Working release.