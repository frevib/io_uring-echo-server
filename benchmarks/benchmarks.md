# io_uring echo server benchmarks

## prerequisites to run the tests
* __Linux 5.6 or higher with IORING_FEAT_FAST_POLL required__ (available in https://git.kernel.dk/cgit/linux-block/?h=io_uring-task-poll). [Here][kernel_compile] is how to compile a Linux kernel

## programs under test
* echo server with an event loop created with __io_uring__ :
* echo server with an event loop created with __epoll__ :

## system that runs the tests
Macbook ....


## test tool
* Rust echo bench
* `cargo run --release -- --address "localhost:6666" --number [number of clients] --duration [duration in secs] --length [msg size]`
* 2 runs for each combination of 128, 512 and 1000 bytes message size with 1, 50, 150 and 300 clients
* one physical core isolated with `isolcpus=0,1`




## benchmark results, requests/second

**io_uring with IORING_FEAT_FAST_POLL**


|          |   1   |   50   |   150  |   300  | < clients |
|:--------:|:-----:|:------:|:------:|:------:|:-:|
|  **128** | 13093 | 147078 | 190054 | 216637 | |
|  **512** | 13140 | 150444 | 193019 | 203360 | |
| **1000** | 14024 | 140248 | 178638 | 200853 | |
| **bytes sent ^** |  |  |  |  | |



**epoll**



## extra info
* testing with many more, > 2000 clients, causes both echo servers to crash



[kernle_compile]: https://www.cyberciti.biz/tips/compiling-linux-kernel-26.html