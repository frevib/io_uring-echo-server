# io_uring bare minimum echo server

* uses an event loop created with io_uring
* uses liburing 0.3
* Linux 5.4 or higer needed. (Lower versions don't return the right amount of bytes read from `io_uring_prep_readv` in `cqe->res`).

## install and run
`make`

`./io_uring_echo_server [port_number]`


## Benchmarks
* VMWare Ubuntu 18.04
* Linux 5.4.12
* 4 virtual cores
* Macbook pro i7 2,6ghz/32GB

with `rust_echo_bench`: https://github.com/haraldh/rust_echo_bench

command: `cargo run --release -- --address "localhost:6666" --number 50 --duration 60 --length 512`


```
Benchmarking: localhost:5555
50 clients, running 512 bytes, 60 sec.

Speed: 393597 request/sec, 393597 response/sec
Requests: 23615846
Responses: 23615846
```