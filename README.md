# io_uring event loop

* bare minimum event loop for the purpose of learning io_uring
* uses liburing 0.3
* Linux 5.4 needed, lower versions don't return the right amount of bytes read from `io_uring_prep_readv` in cqe->res.

## install and run
`make`

`./io_uring_tcp_serer [port_number]`