#include <sys/select.h>
#include <sys/types.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct fd_pool {
    size_t num_tcp_fds;
    size_t num_udp_fds;
    fd_set tcp_set;
    fd_set udp_set;
    pthread_rwlock_t tcp_lock;
    pthread_rwlock_t udp_lock;
} fd_pool_t;

fd_pool_t *new_fd_pool_t(void);
void get_all_fd_pool_t(fd_pool_t *fpool, int *buffer, size_t buffer_len, bool tcp);
bool is_set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp);
void set_fd_pool_t(fd_pool_t *fpool, int fd, bool is_tcp);
void free_fd_pool_t(fd_pool_t *fpool);

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

int main(void) {
    fd_pool_t *fpool = new_fd_pool_t();
    assert(fpool != NULL);

    set_fd_pool_t(fpool, 1, true);
    assert(fpool->num_tcp_fds == 1);
    assert(fpool->num_udp_fds == 0);
    set_fd_pool_t(fpool, 371, true);
    assert(fpool->num_tcp_fds == 2);
    assert(fpool->num_udp_fds == 0);

    set_fd_pool_t(fpool, 1, false);
    assert(fpool->num_tcp_fds == 2);
    assert(fpool->num_udp_fds == 1);
    set_fd_pool_t(fpool, 361, false);
    assert(fpool->num_tcp_fds == 2);
    assert(fpool->num_udp_fds == 2);

    bool is_set = is_set_fd_pool_t(fpool, 1, true);
    assert(is_set == true);
    is_set = is_set_fd_pool_t(fpool, 344, true);
    assert(is_set == false);

    is_set = is_set_fd_pool_t(fpool, 1, false);
    assert(is_set == true);
    is_set = is_set_fd_pool_t(fpool, 344, false);
    assert(is_set == false);

    int tcp_set_buffer[10];
    get_all_fd_pool_t(fpool, tcp_set_buffer, 2, true);
    assert(tcp_set_buffer[0] == 1);
    assert(tcp_set_buffer[1] == 371);

    int udp_set_buffer[10];
    get_all_fd_pool_t(fpool, udp_set_buffer, 2, false);
    assert(udp_set_buffer[0] == 1);
    assert(udp_set_buffer[1] == 361);

    free_fd_pool_t(fpool);
}