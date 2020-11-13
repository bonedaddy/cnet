# cnet

> An opinionated C package to provide a Golang `net` stdlib like experience

`cnet` is a C library to minimize the difficulty in writing network code with C. It provides simple primitives, and functions for reducing the overhead in writing network code. By being slightly opinionated `cnet` cuts down the amount of code one needs to write to setup network code (specify ip addresses, socket options, create socket, etc...) roughly 5x -> 10x.

# details

Although C isn't tremendously difficult, writing network code in C is a process that I find to be excessively verbose. Compared to Go which you can create a network listener in less than 10 lines of code, accomplishing the same thing with C takes roughly 50 -> 150 lines of code. While it's not fair to expect C to be as simple to use as Go, part of this problem with verbosity is caused by there being no real C equivalent of Golang's stdlib `net` packet. By solving this, you can leverage modern C compiler niceness to create a usage experience that is only moderately more difficult to use than Go's `net`.

# features

* functions to ease creation, and usage of sockets and their associated file descriptors
* an "fd pool" to allow managing groups of file descriptors for a related service
  * get all managed fd's
  * get all fd's ready for reading/writing

# dependencies

To use `cnet` you only need two third-party dependencies, and a C11 compiler:

* pthread
  * practically availble anywhere
* ulog
  * small logging library contained within the `deps/ulog` folder of this repository