#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <malloc.h>
#include <pthread.h>
#include "fd_pool.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

/*!
  * @brief allocates memory for, and initializes a new fd_pool_t object
  * @return Success: pointer to instance of fd_pool_t
  * @return Failure: NULL ptr
*/
fd_pool_t *new_fd_pool_t(void) {
    fd_pool_t *fpool = calloc(1, sizeof(fd_pool_t));
    if (fpool == NULL) {
        return NULL;
    }
    
    FD_ZERO(&fpool->tcp_set);
    FD_ZERO(&fpool->udp_set);

    pthread_rwlock_init(&fpool->tcp_lock, NULL);
    pthread_rwlock_init(&fpool->udp_lock, NULL);

    return fpool;
}

/*!
  * @brief polls all tcp or udp fds to determine which can be used for read/write
  * @param buffer location in memory to write available fds into, for memory efficiency use a stack-alloc'd array
  * @param buffer_len the number of items the array can store, this means we will get no more than this number of available fds
  * @param tcp if true check tcp_set, if false check udp_set
  * @param read if true only check for read sockets, if false only check for write sockets
  * @todo enable supplying custom timeouts
*/
void get_active_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp, bool write) { 
    fd_set check_set, read_set, write_set;
    int num_fds = 0;
    if (tcp) {
        pthread_rwlock_rdlock(&fpool->tcp_lock);
        check_set = fpool->tcp_set;
        num_fds = fpool->num_tcp_fds;
        pthread_rwlock_unlock(&fpool->tcp_lock);
    } else {
        pthread_rwlock_rdlock(&fpool->udp_lock);
        check_set = fpool->udp_set;
        num_fds = fpool->num_udp_fds;
        pthread_rwlock_unlock(&fpool->udp_lock);        
    }
    if (write) {
        write_set = check_set;
    } else {
        read_set = check_set;
    }
    // todo: not yet done, just a basic layout
    select(num_fds, &read_set, &write_set, NULL, NULL);
}

/*!
  * @brief returns the file descriptors from tcp_set or udp_set, without checking to see if any are available for read/write
  * @param buffer location in memory to write available fds into, for memory efficiency use a stack-alloc'd array
  * @param buffer_len the number of items the array can store, this means we will get no more than this number of available fds
  * @param tcp if true check tcp_set, if false check udp_set
*/
void get_all_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp) {
    if (tcp) {
        pthread_rwlock_rdlock(&fpool->tcp_lock);
        size_t num_items = 0;
        for (int i = 1; i <= 65536; i++) {
            if (FD_ISSET(i, &fpool->tcp_set)) {
                buffer[num_items] = i;
                num_items += 1;
                if (num_items == buffer_len) {
                    break;
                }
            }
        }
        pthread_rwlock_unlock(&fpool->tcp_lock);
    } else {
        pthread_rwlock_rdlock(&fpool->udp_lock);
        size_t num_items = 0;
        for (int i = 1; i <= 65536; i++) {
            if (FD_ISSET(i, &fpool->udp_set)) {
                buffer[num_items] = i;
                num_items += 1;
                if (num_items == buffer_len) {
                    break;
                }
            }
        }
        pthread_rwlock_unlock(&fpool->udp_lock);
    }
}

/*!
  * @brief checks to see if we have the given fd as part of our pool
  * @param is_tcp if true check tcp_set, if false check udp_set
*/
bool is_set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp) {
    bool set = false;
    if (is_tcp) {
        pthread_rwlock_rdlock(&fpool->tcp_lock);
        set = FD_ISSET(fd, &fpool->tcp_set);
        pthread_rwlock_unlock(&fpool->tcp_lock);
    } else {
        pthread_rwlock_rdlock(&fpool->udp_lock);
        set = FD_ISSET(fd, &fpool->udp_set);
        pthread_rwlock_unlock(&fpool->udp_lock);
    }
    return set;
}

/*!
  * @param fd the file descriptor to ste within the pool
  * @param is_tcp if true check tcp_set, if false check udp_set
*/
void set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp) {
    if (is_tcp == true) {
        pthread_rwlock_wrlock(&fpool->tcp_lock);
        FD_SET(fd, &fpool->tcp_set);
        fpool->num_tcp_fds += 1;
        pthread_rwlock_unlock(&fpool->tcp_lock);
    } else {
        pthread_rwlock_wrlock(&fpool->udp_lock);
        FD_SET(fd, &fpool->udp_set);
        fpool->num_udp_fds += 1;
        pthread_rwlock_unlock(&fpool->udp_lock);        
    }
    
}

/*!
  * @brief free up all resources allocated for the fd_pool_t struct
  * @note this does not close the file resources associated with any file descriptors
*/
void free_fd_pool_t(fd_pool_t *fpool) {
    
    // lock to prevent any access to struct fields during free
    pthread_rwlock_wrlock(&fpool->tcp_lock);
    pthread_rwlock_wrlock(&fpool->udp_lock);

    FD_ZERO(&fpool->tcp_set);
    FD_ZERO(&fpool->udp_set);

    pthread_rwlock_destroy(&fpool->tcp_lock);
    pthread_rwlock_destroy(&fpool->udp_lock);

    free(fpool);
}