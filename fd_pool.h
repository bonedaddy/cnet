#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

/*!
  * @brief bundles together sets of file descriptors associated with tcp and/or udp sockets
  * @details it allows us to manage file descriptors en-masse, and is useful for services
  * @details that might have a bunch of different active sockets/fds and needs to select from
  * @details among them ones that are available for consumption
*/
typedef struct fd_pool {
    size_t num_tcp_fds;
    size_t num_udp_fds;
    fd_set tcp_set;
    fd_set udp_set;
    pthread_rwlock_t tcp_lock;
    pthread_rwlock_t udp_lock;
} fd_pool_t;

/*!
  * @brief allocates memory for, and initializes a new fd_pool_t object
  * @return Success: pointer to instance of fd_pool_t
  * @return Failure: NULL ptr
*/
fd_pool_t *new_fd_pool_t(void);

/*!
  * @brief polls all tcp or udp fds to determine which can be used for read/write
  * @param buffer location in memory to write available fds into, for memory efficiency use a stack-alloc'd array
  * @param buffer_len the number of items the array can store, this means we will get no more than this number of available fds
  * @param tcp if true check tcp_set, if false check udp_set
  * @param read if true only check for read sockets, if false only check for write sockets
  * @todo enable supplying custom timeouts
*/
void get_active_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp, bool read);

/*!
  * @brief returns the file descriptors from tcp_set or udp_set, without checking to see if any are available for read/write
  * @param buffer location in memory to write available fds into, for memory efficiency use a stack-alloc'd array
  * @param buffer_len the number of items the array can store, this means we will get no more than this number of available fds
  * @param tcp if true check tcp_set, if false check udp_set
*/
void get_all_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp);

/*!
  * @brief checks to see if we have the given fd as part of our pool
  * @param is_tcp if true check tcp_set, if false check udp_set
*/
bool is_set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp);

/*!
  * @param fd the file descriptor to ste within the pool
  * @param is_tcp if true check tcp_set, if false check udp_set
*/
void set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp);

/*!
  * @brief free up all resources allocated for the fd_pool_t struct
  * @note this does not close the file resources associated with any file descriptors
*/
void free_fd_pool_t(fd_pool_t *fpool);