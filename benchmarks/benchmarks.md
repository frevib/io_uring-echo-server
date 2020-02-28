# io_uring echo server benchmarks

## prerequisites to run the tests
* __Linux 5.6 or higher with IORING_FEAT_FAST_POLL required__ (available in https://git.kernel.dk/cgit/linux-block/?h=io_uring-task-poll). [Here][kernel_compile] is how to compile a Linux kernel

## programs under test
* echo server with an event loop created with __io_uring__ :
* echo server with an event loop created with __epoll__ :

## hardware used
* Vmware Ubuntu 18.04, 8gb RAM, 6 vcores, 2 vcores (one physical) isolated for the echo server with `isolcpus=0,1`.
* `uname -rnsp`: Linux ubuntu 5.6.0-rc1+ x86_64



## test tool
* Rust echo bench
* `cargo run --release -- --address "localhost:6666" --number [number of clients] --duration [duration in secs] --length [msg size]`
* 2 runs for each combination of 128, 512 and 1000 bytes message size with 1, 50, 150, 300, 500 and 1000 clients




## benchmark results, requests/second

**io_uring with IORING_FEAT_FAST_POLL**

|            | 1     | 50     | 150    | 300    | 500    | 1000   | < clients |
|------------|:-----:|:------:|:------:|:------:|:------:|:------:|:---------:|
| 128 bytes  | 13093 | 147078 | 190054 | 216637 | 211280 | 173343 |           |
| 512 bytes  | 13140 | 150444 | 193019 | 203360 | 194701 | 156880 |           |
| 1000 bytes | 14024 | 140248 | 178638 | 200853 | 183845 | 143810 |           |




**epoll**
|                | 1     | 50     | 150    | 300    | 500    | 1000   | < clients |
|----------------|:-----:|:------:|:------:|:------:|:------:|:------:|:---------:|
|  128 bytes     | 13177 | 139863 | 152561 | 145517 | 125402 | 108380 | |
|  512 bytes     | 13190 | 135973 | 147153 | 142518 | 124584 | 107257 | |
|  1000 bytes    | 13172 | 131773 | 142481 | 131748 | 123287 | 102474 | |





## extra info
* Testing with many more, > 2000 clients, causes both echo servers to crash.
* When running many clients, `io_uring_echo_server` becomes unresponsive in an uninterruptible sleep state. So for this echo server first the 128 bytes and 512 bytes benchmark is run, then the echo server is restarted and the 1000 bytes benchmark is run. I'm not sure what is happening here. There are problems with the epoll echo server.
* io_uring_echo_server needs a separate buffer per connection. Each buffer is indexed by it's file descriptor number, like `bufs[fd_number]. So if you have many connections you could have a segfault when the fd_number is too high.



[kernle_compile]: https://www.cyberciti.biz/tips/compiling-linux-kernel-26.html