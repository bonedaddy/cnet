#include "fd_pool.h"
#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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
 * @param check_set pointer to an fd_set variable which we will write the active
 * fds into
 * @param check_set note that this set will be zero'd within the function call
 * @param tcp if true check tcp_set, if false check udp_set
 * @param read if true only check for read sockets, if false only check for
 * write sockets
 * @return number of fds
 * @todo enable supplying custom timeouts
 */
int get_active_fd_pool_t(fd_pool_t *fpool, fd_set *check_set, bool tcp, bool read) {
    fd_set *read_set = NULL;
    fd_set *write_set = NULL;
    int max_fds = 0;
    if (tcp) {
        pthread_rwlock_rdlock(&fpool->tcp_lock);
        unsafe_copy_fd_pool_t(fpool, check_set, true);
        max_fds = unsafe_max_socket_fd_pool_t(fpool, true);
        pthread_rwlock_unlock(&fpool->tcp_lock);
    } else {
        pthread_rwlock_rdlock(&fpool->udp_lock);
        unsafe_copy_fd_pool_t(fpool, check_set, false);
        max_fds = unsafe_max_socket_fd_pool_t(fpool, false);
        pthread_rwlock_unlock(&fpool->udp_lock);
    }
    if (read) {
        read_set = check_set;
    } else {
        write_set = check_set;
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50;
    int num_active = select(max_fds + 1, read_set, write_set, NULL, &timeout);
    return num_active;
}

/*!
 * @brief returns the file descriptors from tcp_set or udp_set, without checking
 * to see if any are available for read/write
 * @param buffer location in memory to write available fds into, for memory
 * efficiency use a stack-alloc'd array
 * @param buffer_len the number of items the array can store, this means we will
 * get no more than this number of available fds
 * @param tcp if true check tcp_set, if false check udp_set
 */
int get_all_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp) {
    size_t num_items = 0;
    if (tcp) {
        pthread_rwlock_rdlock(&fpool->tcp_lock);
        // https://stackoverflow.com/questions/53698995/how-to-access-fds-in-fd-set-through-indexing-in-linux/53699058#53699058
        // must not use value greater than FD_SETSIZE otherwise this will result in
        // undefined behavior
        for (int i = 1; i <= FD_SETSIZE - 1; i++) {
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
        // //
        // https://stackoverflow.com/questions/53698995/how-to-access-fds-in-fd-set-through-indexing-in-linux/53699058#53699058
        for (int i = 1; i <= FD_SETSIZE - 1; i++) {
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
    return (int)num_items;
}

/*!
 * @brief returns the highest socket number
 * @warning caller must handle locking of the mutexes
 */
int unsafe_max_socket_fd_pool_t(fd_pool_t *fpool, bool tcp) {
    int max = 0;
    if (tcp) {
        for (int i = 1; i < FD_SETSIZE - 1; i++) {
            if (FD_ISSET(i, &fpool->tcp_set)) {
                if (i > max) {
                    max = 1;
                }
            }
        }
    } else {
        for (int i = 1; i < FD_SETSIZE - 1; i++) {
            if (FD_ISSET(i, &fpool->udp_set)) {
                if (i > max) {
                    max = 1;
                }
            }
        }
    }
    return max;
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
 * @brief copies either tcp or udp fdset
 * @warning caller must handle locking of the mutexes
 */
void unsafe_copy_fd_pool_t(fd_pool_t *fpool, fd_set *dst, bool tcp) {
    // zero the dest buffer
    FD_ZERO(dst);
    if (tcp) {
        for (int i = 1; i < FD_SETSIZE - 1; i++) {
            if (FD_ISSET(i, &fpool->tcp_set)) {
                FD_SET(i, dst);
            }
        }
    } else {
        for (int i = 1; i < FD_SETSIZE - 1; i++) {
            if (FD_ISSET(i, &fpool->udp_set)) {
                FD_SET(i, dst);
            }
        }
    }
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
 * @note this does not close the file resources associated with any file
 * descriptors
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