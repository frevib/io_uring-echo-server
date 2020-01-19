# io_uring bare minimum echo server

* uses an event loop created with io_uring
* uses liburing 0.3
* Linux 5.4 needed, lower versions don't return the right amount of bytes read from `io_uring_prep_readv` in cqe->res.

## install and run
`make`

`./io_uring_tcp_serer [port_number]`


## Benchmarks
* VMWare Ubuntu 18.04
* Linux 5.4.12
* 4 virtual cores
* Macbook pro i7 2,6ghz/32GB

with `rust_echo_bench`: https://github.com/haraldh/rust_echo_bench

```
Benchmarking: 172.16.166.152:5555
40 clients, running 512 bytes, 30 sec.

Speed: 105274 request/sec, 105274 response/sec
Requests: 3158231
Responses: 3158230
```

```
Benchmarking: 172.16.166.152:5555
50 clients, running 512 bytes, 30 sec.

Speed: 124181 request/sec, 124181 response/sec
Requests: 3725449
Responses: 3725443
```

On localhost:

```
Benchmarking: localhost:5555
50 clients, running 512 bytes, 60 sec.

Speed: 368368 request/sec, 368368 response/sec
Requests: 22102112
Responses: 22102110
```