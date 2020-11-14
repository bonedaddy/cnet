// Copyright 2020 Bonedaddy (Alexandre Trottier)
//
// licensed under GNU AFFERO GENERAL PUBLIC LICENSE;
// you may not use this file except in compliance with the License;
// You may obtain the license via the LICENSE file in the repository root;
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sockets.h"
#include "deps/ulog/logger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*!
 * @brief creates a new client socket
 * @todo should we enable usage of socket options
 */
socket_client_t *new_client_socket(thread_logger *thl, char *ip, char *port,
                                   bool tcp, bool ipv4) {

    addr_info hints;
    memset(&hints, 0, sizeof(addr_info));

    if (tcp == true) {
        hints.ai_socktype = SOCK_STREAM;
    } else {
        hints.ai_socktype = SOCK_DGRAM;
    }

    if (ipv4 == true) {
        hints.ai_family = AF_INET;
    } else {
        hints.ai_family = AF_INET6;
    }

    // todo: should we do this?
    hints.ai_flags = AI_PASSIVE;

    addr_info *peer_address;
    int rc = getaddrinfo(ip, port, &hints, &peer_address);
    if (rc != 0) {
        freeaddrinfo(peer_address);
        return NULL;
    }

    int client_socket_num = get_new_socket(thl, peer_address, NULL, 0, true, tcp);
    if (client_socket_num == -1) {
        LOG_ERROR(thl, 0, "failed to get new socket");
        freeaddrinfo(peer_address);
        return NULL;
    }

    socket_client_t *sock_client =
        calloc(1, sizeof(sock_client) + sizeof(peer_address));
    if (sock_client == NULL) {
        LOG_ERROR(thl, 0, "failed to calloc socket_client_t");
        return NULL;
    }

    sock_client->socket_number = client_socket_num;
    sock_client->peer_address = peer_address;

    LOG_INFO(thl, 0, "client successfully created");

    return sock_client;
}

/*!
 * @brief used to accept a connection queued up against the given socket
 * @details it accepts an incoming connection on the socket returning
 * @details the socket number that can be used to communicate on this connection
 */
int accept_socket(thread_logger *thl, int socket) {
    sock_addr_storage incoming_address;
    socklen_t addr_size = sizeof(incoming_address);
    int new_fd = accept(socket, (struct sockaddr *)&incoming_address, &addr_size);
    if (new_fd == -1) {
        LOGF_ERROR(thl, 0, "failed to accept connection %s", strerror(errno));
        return -1;
    }
    return new_fd;
}

/*!
 * @brief attempts to create a socket listening on the specified ip and port
 * @details this is a helper function to abstract away the verbosity required to
 * create a socket
 */
int listen_socket(thread_logger *thl, char *ip, char *port, bool tcp, bool ipv4,
                  SOCKET_OPTS sock_opts[], int num_opts) {
    if (sock_opts == NULL || num_opts == 0) {
        LOG_ERROR(thl, 0, "empty socket opts");
        return -1;
    }

    addr_info hints;
    memset(&hints, 0, sizeof(addr_info));

    if (tcp == true) {
        hints.ai_socktype = SOCK_STREAM;
    } else {
        hints.ai_socktype = SOCK_DGRAM;
    }

    if (ipv4 == true) {
        hints.ai_family = AF_INET;
    } else {
        hints.ai_family = AF_INET6;
    }

    addr_info *bind_address;

    int rc = getaddrinfo(ip, port, &hints, &bind_address);
    if (rc != 0) {
        freeaddrinfo(bind_address);
        return -1;
    }

    int socket_num = get_new_socket(thl, bind_address, sock_opts, 2, false, tcp);
    freeaddrinfo(bind_address);
    if (socket_num == -1) {
        LOG_ERROR(thl, 0, "failed to get new socket");
    }

    // if this is is a udp sockets, no need to start the listener
    // only tcp sockets need to do this
    if (tcp) {
        rc = listen(socket_num, 10); // todo: enable customizable connection count
        if (rc == -1) {
            LOGF_ERROR(thl, 0, "failed to listen on tcp socket %s", strerror(errno));
            return -1;
        }
    }

    return socket_num;
}

/*! @brief  gets an available socket attached to bind_address
 * @return Success: file descriptor socket number greater than 0
 * @return Failure: -1
 * initializers a socket attached to bind_address with sock_opts, and binds the
 * address
 */
int get_new_socket(thread_logger *thl, addr_info *bind_address,
                   SOCKET_OPTS sock_opts[], int num_opts, bool client, bool tcp) {
    // creates the socket and gets us its file descriptor
    int listen_socket_num =
        socket(bind_address->ai_family, bind_address->ai_socktype,
               bind_address->ai_protocol);
    // less than 0 is an error
    if (listen_socket_num < 0) {
        LOG_ERROR(thl, 0, "socket creation failed");
        return -1;
    }
    int one;
    int rc;
    bool passed;
    for (int i = 0; i < num_opts; i++) {
        switch (sock_opts[i]) {
            case REUSEADDR:
                one = 1;
                // set socket options before doing anything else
                // i tried setting it after listen, but I don't think that works
                rc = setsockopt(listen_socket_num, SOL_SOCKET, SO_REUSEADDR, &one,
                                sizeof(int));
                if (rc != 0) {
                    LOG_ERROR(thl, 0, "failed to set socket reuse addr");
                    return -1;
                }
                LOG_INFO(thl, 0, "set socket opt REUSEADDR");
                break;
            case BLOCK:
                passed = set_socket_blocking_status(listen_socket_num, true);
                if (passed == false) {
                    LOG_ERROR(thl, 0, "failed to set socket blocking mode");
                    return -1;
                }
                LOG_INFO(thl, 0, "set socket opt BLOCK");
                break;
            case NOBLOCK:
                passed = set_socket_blocking_status(listen_socket_num, false);
                if (passed == false) {
                    LOG_ERROR(thl, 0, "failed to set socket blocking mode");
                    return -1;
                }
                LOG_INFO(thl, 0, "set socket opt NOBLOCK");
                break;
            default:
                LOG_ERROR(thl, 0, "invalid socket option");
                return -1;
        }
    }
    if (client == true) {
        if (tcp == true) {
            /*! @todo should we do this on UDP connections?? */
            rc = connect(listen_socket_num, bind_address->ai_addr,
                         bind_address->ai_addrlen);
            if (rc != 0) {
                close(listen_socket_num);
                return -1;
            }
        }
        return listen_socket_num;
    }
    // binds the address to the socket
    bind(listen_socket_num, bind_address->ai_addr, bind_address->ai_addrlen);
    if (errno != 0) {
        LOGF_ERROR(thl, 0, "socket bind failed with error %s", strerror(errno));
        return -1;
    }
    return listen_socket_num;
}

/*! @brief used to enable/disable blocking sockets
 * @return Failure: false
 * @return Success: true
 * @note see
 * https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking/1549344#1549344
 */
bool set_socket_blocking_status(int fd, bool blocking) {
    if (fd < 0) {
        return false;
    } else {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            return false;
        }
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
    }
}

/*! @brief returns the address the client is connecting from
 */
char *get_name_info(sock_addr *client_address) {
    char address_info[256]; // destroy when function returns
    getnameinfo(client_address, sizeof(*client_address),
                address_info,         // output buffer
                sizeof(address_info), // size of the output buffer
                0,                    // second buffer which outputs service name
                0,                    // length of the second buffer
                NI_NUMERICHOST        // want to see hostnmae as an ip address
    );
    char *addr = malloc(sizeof(address_info));
    if (addr == NULL) {
        return NULL;
    }
    strcpy(addr, address_info);
    return addr;
}

/*! @brief generates an addr_info struct with defaults
 * defaults is IPv4, TCP, and AI_PASSIVE flags
 */
addr_info default_hints() {
    addr_info hints;
    memset(&hints, 0, sizeof(hints));
    // change to AF_INET6 to use IPv6
    hints.ai_family = AF_INET;
    // indicates TCP, if you want UDP use SOCKT_DGRAM
    hints.ai_socktype = SOCK_STREAM;
    // indicates to getaddrinfo we want to bind to the wildcard address
    hints.ai_flags = AI_PASSIVE;
    return hints;
}

void free_socket_client_t(socket_client_t *sock_client) {
    close(sock_client->socket_number);
    freeaddrinfo(sock_client->peer_address);
    free(sock_client);
}