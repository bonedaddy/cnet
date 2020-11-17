# cnet

> An opinionated C package to provide a Golang `net` stdlib like experience

`cnet` is a C library to minimize the difficulty in writing network code with C. It provides simple primitives, and functions for reducing the overhead in writing network code. By being slightly opinionated `cnet` cuts down the amount of code one needs to write to setup network code (specify ip addresses, socket options, create socket, etc...) roughly 5x -> 10x.

# details

Although C isn't tremendously difficult, writing network code in C is a process that I find to be excessively verbose. Compared to Go which you can create a network listener in less than 10 lines of code, accomplishing the same thing with C takes roughly 50 -> 150 lines of code. While it's not fair to expect C to be as simple to use as Go, part of this problem with verbosity is caused by there being no real C equivalent of Golang's stdlib `net` packet. By solving this, you can leverage modern C compiler niceness to create a usage experience that is only moderately more difficult to use than Go's `net`.

# features

* `fd_pool_t` is a thread-safe wrapper around the `FD_SET`, `FD_ISSET`, `select` and other core data types
  * Enables managing a pool of tcp and/or udp sockets (set, clear, get, etc..)
  * Enables retrieving all available sockets for reading/writing 
* socket management functions
  * listen on tcp/udp sockets
  * connect to tcp/udp sockets
  * control socket options (blocking, non-blocking, reuseaddr, etc..)
  
# dependencies

To use `cnet` you only need two third-party dependencies, and a C11 compiler:

* pthread
  * practically availble anywhere
* ulog
  * small logging library contained within the `deps/ulog` folder of this repository

# usage

## Listen On tcp://127.0.0.1:5001 W/ Default Socket Options [Server]

Default socket options enables soreuseport and enables socket blocking

```C
#include "sockets.h"

int main(void) {
  // included via sockets.h
  // enables a debug logger
  thread_logger *thl = new_thread_logger(true);
  // the returned fd is the file descriptor that can be used with this socket
  int fd = listen_socket(thl, "127.0.0.1", "5001", true, true, default_sock_opts, default_socket_opts_count);
  fd_pool_t *fpool = new_fd_pool_t(); // create the fd management pool
  set_fd_pool_t(fpool, fd, true); // set the opened file descriptor
  fd_set check_set; // will contain the available file descriptors
  int num_active = get_active_fd_pool_t(fpool, &check_set, true, true); // get all available tcp sockets for reading
  printf("found %i available sockets for reading", num_active); 
  // check_set will contain all available fd's (if any)
  clear_thread_logger(thl); // free up resources associated with thl
  close(fd); // close the opened socket
}
```

## Listen On udp://127.0.0.1:5002 W/ Docket Socket Options [Server]

Default socket options enables soreuseport and enables socket blocking

```C
#include "sockets.h"
#include "fd_pool.h"

int main(void) {
  // included via sockets.h
  // enables a debug logger
  thread_logger *thl = new_thread_logger(true);
  // the returned fd is the file descriptor that can be used with this socket
  int fd = listen_socket(thl, "127.0.0.1", "5002", false, true, default_sock_opts, default_socket_opts_count);
  fd_pool_t *fpool = new_fd_pool_t(); // create the fd management pool
  set_fd_pool_t(fpool, fd, true); // set the opened file descriptor
  fd_set check_set; // will contain the available file descriptors
  int num_active = get_active_fd_pool_t(fpool, &check_set, false, true); // get all available udp sockets for reading
  printf("found %i available sockets for reading", num_active); 
  // check_set will contain all available fd's (if any)
  clear_thread_logger(thl); // free up resources associated with thl
  close(fd); // close the opened socket
}
```

## Connect To tcp://127.0.0.1:5001 [Client]

```C
#include "sockets.h"

int main(void) {
  // included via sockets.h
  // enables a debug logger
  thread_logger *thl = new_thread_logger(true);
  socket_client_t *sock_client = new_client_socket(thl, "127.0.0.1", "5001", true, true);
  printf("client socket opened with fd %i\n", sock_client->socket_number); // print the opened file descriptor
  clear_thread_logger(thl); // free up resources associated with thl
  free_socket_client_t(sock_client); // will close the opened socket as well
}
```

## Connect To udp://127.0.0.1:5002 [Client]

```C
#include "sockets.h"

int main(void) {
  // included via sockets.h
  // enables a debug logger
  thread_logger *thl = new_thread_logger(true);
  socket_client_t *sock_client = new_client_socket(thl, "127.0.0.1", "5002", false, true);
  printf("client socket opened with fd %i\n", sock_client->socket_number); // print the opened file descriptor
  clear_thread_logger(thl); // free up resources associated with thl
  free_socket_client_t(sock_client); // will close the opened socket as well
}
```
