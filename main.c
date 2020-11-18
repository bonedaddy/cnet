#include "deps/clinch/command_line.h"
#include "deps/ulog/logger.h"
#include "sockets.h"
#include "fd_pool.h"
#include <stdbool.h>

#define COMMAND_VERSION_STRING "v0.0.1"

struct arg_str *ip_address;
struct arg_str *port;
struct arg_str *mode;
struct arg_str *proto;

void socket_server_callback(int argc, char *argv[]) {
    thread_logger *thl = new_thread_logger(true);
    if (thl == NULL) {
        return;
    }
    fd_pool_t *fpool = new_fd_pool_t();
    
    bool tcp = false;

    if (strcmp((char *)*proto->sval, "tcp") == 0) {
        LOG_INFO(thl, 0, "using tcp");
        tcp = true;
    } else {
        LOG_INFO(thl, 0, "using udp");
    }

    int fd = listen_socket(thl, (char *)*ip_address->sval, (char *)*port->sval, tcp, true, default_sock_opts, default_socket_opts_count);
    if (fd == -1) {
        LOG_ERROR(thl, 0, "failed to get a socket to listen on");
        return;
    }

    LOGF_INFO(thl, 0, "using socket %i", fd);
    set_fd_pool_t(fpool, fd, tcp);
    
    for (;;) {
        fd_set check_set;
        int num_active = get_active_fd_pool_t(fpool, &check_set, tcp, true);
        if (num_active <= 0) {
            sleep(0.01);
            continue;
        }
        int new_fd = accept_socket(thl, fd);
        char buffer[1024];
        int rc = read(new_fd, buffer, 1024);
        if (rc == -1) {
            LOGF_ERROR(thl, 0, "read error encountered %s", strerror(errno));
            close(new_fd);
            break;
        }
        if (rc == 0) {
            sleep(0.01);
            continue;
        }
        LOGF_INFO(thl, 0, "received message %s", buffer);
        close(new_fd);
    }

    close(fd);
}

command_handler *new_socket_server_command() {
    command_handler *handler = calloc(1, sizeof(command_handler));
    if (handler == NULL) {
        return NULL;
    }
    handler->name = "socket-server";
    handler->callback = socket_server_callback;
    return handler;
}

int main(int argc, char *argv[]) {
    // default arg setup
    setup_args(COMMAND_VERSION_STRING);
    // custom arg setup
    ip_address = arg_strn(NULL, "ip-address", "<ip>", 1, 1, "ip address of host");
    port = arg_strn(NULL, "port", "<port>", 1, 1, "port of host");
    mode = arg_strn(NULL, "mode", "<mode>", 1, 1, "must be 'server' or 'client'");
    proto = arg_strn(NULL, "proto", "<proto>", 1, 1, "network protocol (tcp, udp)");
    // declare artable
    void *argtable[] = {ip_address,
                        port,
                        mode,
                        proto,
                        help,
                        version,
                        file,
                        output,
                        command_to_run,
                        end};

    // prepare arguments
    int response = parse_args(argc, argv, argtable);
    switch (response) {
        case 0:
            break;
        case -1:
            arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
            printf("parse_args failed\n");
            return response;
        case -2: // this means --help was invoked
            return 0;
    }

    // handle help if no other cli arguments were given (aka binary invoked with
    // ./some-binary)
    if (argc == 1) {
        print_help(argv[0], argtable);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
    }
    // construct the command object
    command_object *pcmd = new_command_object(argc, argv);
    if (pcmd == NULL) {
        printf("failed to get command_object");
        goto EXIT;
    }

    load_command(pcmd, new_socket_server_command());

    // END COMMAND INPUT PREPARATION
    int resp = execute(pcmd, (char *)*command_to_run->sval);
    if (resp != 0) {
        // TODO(bonedaddy): figure out if we should log this
        // printf("command run failed\n");
    }
EXIT:
    free_command_object(pcmd);
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return resp;
}
