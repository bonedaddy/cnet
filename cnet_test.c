#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include "fd_pool.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

void test_fd_pool(void **state) {
    typedef struct args {
        int fds[10];
        int num_fds;
        bool tcp;
    } args_t;

    args_t tests[2];

    tests[0].fds[0] = 1;
    tests[0].fds[1] = 5;
    tests[0].fds[2] = 10;
    tests[0].num_fds = 3;
    tests[0].tcp = true;

    tests[1].fds[0] = 100;
    tests[1].fds[1] = 150;
    tests[1].fds[2] = 200;
    tests[1].num_fds = 3;
    tests[1].tcp = false;

    fd_pool_t *fpool = new_fd_pool_t();
    assert(fpool != NULL);
    
    size_t want_tcp = 0;
    // test tcp
    for (int i = 0; i < tests[0].num_fds; i++) {
        
        set_fd_pool_t(fpool, tests[0].fds[i], true);
        want_tcp += 1;

        assert(fpool->num_tcp_fds == want_tcp);
        assert(fpool->num_udp_fds == 0);
        
        bool is_set = is_set_fd_pool_t(fpool, tests[0].fds[i], true);
        assert(is_set == true);

        int buffer[10];
        size_t buffer_len = 10;
        memset(buffer, 0, 10);

        int fd_count = get_all_fd_pool_t(fpool, buffer, buffer_len, true);
        printf("%i\n", fd_count);
        assert(fd_count == want_tcp);
    }

    size_t want_udp = 0;
    // test udp
    for (int i = 0; i < tests[1].num_fds; i++) {
        
        set_fd_pool_t(fpool, tests[1].fds[i], false);
        want_udp += 1;

        assert(fpool->num_udp_fds == want_udp);
        assert(fpool->num_tcp_fds == want_tcp);
        
        bool is_set = is_set_fd_pool_t(fpool, tests[1].fds[i], false);
        assert(is_set == true);

    }

}


void test_basic(void **state) {
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


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fd_pool),
        cmocka_unit_test(test_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}