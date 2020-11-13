#include <sys/select.h>
#include <sys/types.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "fd_pool.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"


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