# io_uring bare minimum echo server
* uses an event loop created with io_uring
* uses liburing HEAD https://github.com/axboe/liburing
* Linux 5.6 or higher with IORING_FEAT_FAST_POLL (available in https://git.kernel.dk/cgit/linux-block/?h=io_uring-task-poll) needed.


## Install and run
`make liburing`

`make io_uring_echo_server`

`./io_uring_echo_server [port_number]`

## compare with epoll echo server
https://github.com/frevib/epoll-echo-server


## Benchmarks

* Vmware Ubuntu 18.04, 8gb RAM, 6 vcores, 1 core isolated for the echo server with `isolcpus=0`
* Using Rust echo bench (https://github.com/haraldh/rust_echo_bench): `cargo run --release -- --address "localhost:6666" --number 50 --duration 20 --length 512`

#### io_uring_echo_server with IORING_FEAT_FAST_POLL
    Finished release [optimized] target(s) in 0.00s
     Running `target/release/echo_bench --address 'localhost:6666' --number 50 --duration 20 --length 512`
Benchmarking: localhost:6666
50 clients, running 512 bytes, 20 sec.

Speed: 133046 request/sec, 133046 response/sec
Requests: 2660930
Responses: 2660930



#### epoll_echo_server https://github.com/frevib/epoll-echo-server
    Finished release [optimized] target(s) in 0.00s
     Running `target/release/echo_bench --address 'localhost:7777' --number 50 --duration 20 --length 512`
Benchmarking: localhost:7777
50 clients, running 512 bytes, 20 sec.

Speed: 98191 request/sec, 98191 response/sec
Requests: 1963833
Responses: 1963833




## Versions

### v1.4
Fixed bug that massively overstated the performance.

### v1.3
Use pre-allocated `sqe->user_data` instead of dynamically allocating memory.

### v1.1
Fix memory leak.

### v1.0
Working release.