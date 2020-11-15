#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include "fd_pool.h"
#include "sockets.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

bool stop = false;
bool stopped = false;
void *test_socket_wrapper(void *data) {
    socket_client_t *sock_client = (socket_client_t *)data;
    while (stop == false) {
        int sent = send(sock_client->socket_number, "hello", strlen("hello"), 0);
        assert(sent == 5);
    }
    free_socket_client_t(sock_client);
    stopped = true;
    pthread_exit(NULL);
}

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

        int buffer[10];
        size_t buffer_len = 10;
        memset(buffer, 0, 10);
        int fd_count = get_all_fd_pool_t(fpool, buffer, buffer_len, false);
        assert(fd_count == want_udp);
    }

    fd_set tcp_copy, udp_copy;
    unsafe_copy_fd_pool_t(fpool, &tcp_copy, true);
    unsafe_copy_fd_pool_t(fpool, &udp_copy, false);

    assert(FD_ISSET(1, &tcp_copy));
    assert(FD_ISSET(5, &tcp_copy));
    assert(FD_ISSET(10, &tcp_copy));

    assert(FD_ISSET(100, &udp_copy));
    assert(FD_ISSET(150, &udp_copy));
    assert(FD_ISSET(200, &udp_copy));

    free_fd_pool_t(fpool);
}

void test_listen_socket(void **state) {
    thread_logger *thl = new_thread_logger(true);

    typedef struct args {
        char *addr;
        char *port;
        SOCKET_OPTS sock_opts[10];
        int num_sock_opts;
        bool tcp;
        bool ipv4;
    } args_t;

    args_t tests[8];

    // ipv4 tcp - reuse & noblock
    tests[0].addr = "127.0.0.1";
    tests[0].port = "5001";
    tests[0].sock_opts[0] = REUSEADDR;
    tests[0].sock_opts[1] = NOBLOCK;
    tests[0].num_sock_opts = 2;
    tests[0].tcp = true;
    tests[0].ipv4 = true;

    // ipv4 tcp - reuse & block
    tests[1].addr = "127.0.0.1";
    tests[1].port = "5002";
    tests[1].sock_opts[0] = REUSEADDR;
    tests[1].sock_opts[1] = BLOCK;
    tests[1].num_sock_opts = 2;
    tests[1].tcp = true;
    tests[1].ipv4 = true;

    // ipv4 udp - reuse & noblock
    tests[2].addr = "127.0.0.1";
    tests[2].port = "5001";
    tests[2].sock_opts[0] = REUSEADDR;
    tests[2].sock_opts[1] = NOBLOCK;
    tests[2].num_sock_opts = 2;
    tests[2].tcp = false;
    tests[2].ipv4 = true;

    // ipv4 udp - reuse & block
    tests[3].addr = "127.0.0.1";
    tests[3].port = "5002";
    tests[3].sock_opts[0] = REUSEADDR;
    tests[3].sock_opts[1] = BLOCK;
    tests[3].num_sock_opts = 2;
    tests[3].tcp = false;
    tests[3].ipv4 = true;

    // ipv6 tcp - reuse & noblock
    tests[4].addr = "::1";
    tests[4].port = "5001";
    tests[4].sock_opts[0] = REUSEADDR;
    tests[4].sock_opts[1] = NOBLOCK;
    tests[4].num_sock_opts = 2;
    tests[4].tcp = true;
    tests[4].ipv4 = false;

    // ipv6 tcp - reuse & block
    tests[5].addr = "::1";
    tests[5].port = "5002";
    tests[5].sock_opts[0] = REUSEADDR;
    tests[5].sock_opts[1] = BLOCK;
    tests[5].num_sock_opts = 2;
    tests[5].tcp = true;
    tests[5].ipv4 = false;

    // ipv6 udp - reuse & noblock
    tests[6].addr = "::1";
    tests[6].port = "5001";
    tests[6].sock_opts[0] = REUSEADDR;
    tests[6].sock_opts[1] = NOBLOCK;
    tests[6].num_sock_opts = 2;
    tests[6].tcp = false;
    tests[6].ipv4 = false;

    // ipv6 udp - reuse & block
    tests[7].addr = "::1";
    tests[7].port = "5002";
    tests[7].sock_opts[0] = REUSEADDR;
    tests[7].sock_opts[1] = BLOCK;
    tests[7].num_sock_opts = 2;
    tests[7].tcp = false;
    tests[7].ipv4 = false;

    int sockets[8];
    memset(sockets, 0, 8);

    fd_pool_t *fpool = new_fd_pool_t();
    assert(fpool != NULL);

    for (int i = 0; i < 8; i++) {
        // todo: enable ipv4/ipv6 selection
        int sock_num = listen_socket(thl, tests[i].addr, tests[i].port, tests[i].tcp, tests[i].ipv4, tests[i].sock_opts, tests[i].num_sock_opts);
        assert(sock_num > 0);
        set_fd_pool_t(fpool, sock_num, tests[i].tcp);
        LOGF_DEBUG(thl, 0, "loop %i passed, socket is %i", i, sock_num);
        sockets[i] = sock_num;
    }
    
    socket_client_t *sock_client = new_client_socket(thl, "127.0.0.1", "5001", true, true);
    assert(sock_client != NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, test_socket_wrapper, sock_client);

    LOG_DEBUG(thl, 0, "getting available sockets");
    fd_set active_set_tcp, active_set_udp;

    // TODO: enable better testing by testing all fds
    for (;;) {
        int num_active = get_active_fd_pool_t(fpool, &active_set_tcp, true, true);  // read (tcp)
        if (num_active > 0) {
            // active 
            break;
        }
    } 

    // trigger the socket wrapper test to exit
    stop = true;

    int num_active = get_active_fd_pool_t(fpool, &active_set_tcp, true, true);  // read (tcp)
    LOGF_DEBUG(thl, 0, "found %i available fds. write %i, tcp %i", num_active, false, true);
    num_active = get_active_fd_pool_t(fpool, &active_set_tcp, true, false); // write (tcp);
    LOGF_DEBUG(thl, 0, "found %i available fds. write %i, tcp %i", num_active, true, true);
    num_active = get_active_fd_pool_t(fpool, &active_set_udp, false, true);  // read (udp)
    LOGF_DEBUG(thl, 0, "found %i available fds. write %i, tcp %i", num_active, false, false);
    num_active = get_active_fd_pool_t(fpool, &active_set_udp, false, false); // write (udp);
    LOGF_DEBUG(thl, 0, "found %i available fds. write %i, tcp %i", num_active, true, false);

    // wait for client routine to exit before beginning cleanup
    while (stopped == false) {
        sleep(1);
    }

    LOG_DEBUG(thl, 0, "closing opened sockets (if any)");
    for (int i = 0; i < 8; i++) {
        close(sockets[i]);
    }

    clear_thread_logger(thl);
    free_fd_pool_t(fpool);

    pthread_join(thread, NULL);
}

void test_listen_accept(void **state) {
    // reset variables
    stop = false;
    stopped = false;

   fd_pool_t *fpool = new_fd_pool_t();
    assert(fpool != NULL);

    thread_logger *thl = new_thread_logger(true);
    assert(thl != NULL);

    int fd = listen_socket(thl, "127.0.0.1", "5001", true, true, default_sock_opts, default_socket_opts_count);
    assert(fd > 0);

    set_fd_pool_t(fpool, fd, true);

    socket_client_t *sock_client = new_client_socket(thl, "127.0.0.1", "5001", true, true);
    assert(sock_client != NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, test_socket_wrapper, sock_client);

    
    
    for (;;) {
        fd_set active_set;
        int num_active = get_active_fd_pool_t(fpool, &active_set, true, true);
        switch (num_active) {
            case -1:
                LOG_ERROR(thl, 0, "failed to get active fds");
            case 0:
                sleep(1);
                continue;
            default:
                LOG_DEBUG(thl, 0, "found active fd");
                break;
        }
        assert(FD_ISSET(fd, &active_set));
        int conn_fd = accept_socket(thl, fd);
        assert(conn_fd > 0);
        close(conn_fd);
        break;
    }
    stop = true;
    while (stopped == false) {
        sleep(1);
    }
    pthread_join(thread, NULL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fd_pool),
        cmocka_unit_test(test_listen_socket),
        cmocka_unit_test(test_listen_accept)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}