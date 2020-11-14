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

#pragma once

#include "deps/ulog/logger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*! @typedef addr_info
 * @struct addrinfo
 * @brief alias for `struct addrinfo`
 */
typedef struct addrinfo addr_info;

/*! @typedef sock_addr
 * @struct sockaddr
 * @brief alias for `struct sockaddr`
 */
typedef struct sockaddr sock_addr;

/*! @typedef sock_addr_storage
 * @struct sockaddr_storage
 * @brief alias for `struct sockaddr_storage`
 */
typedef struct sockaddr_storage sock_addr_storage;

/*! @typedef socket_client
 * @struct socket_client
 * a generic tcp/udp socket client
 */
typedef struct socket_client {
    int socket_number;
    addr_info *peer_address;
} socket_client_t;

/*! @enum SOCKET_OPTS
 * @brief used to configure new sockets
 */
typedef enum {
    /*! sets socket with SO_REUSEADDR */
    REUSEADDR,
    /*! sets socket to non-blocking mode */
    NOBLOCK,
    /*! sets socket to blocking mode */
    BLOCK,
} SOCKET_OPTS;

SOCKET_OPTS default_sock_opts[] = {REUSEADDR, BLOCK};
int default_socket_opts_count = 2;

/*!
 * @brief creates a new client socket
 * @todo should we enable usage of socket options
 */
socket_client_t *new_client_socket(thread_logger *thl, char *ip, char *port,
                                   bool tcp, bool ipv4);

/*!
 * @brief used to accept a connection queued up against the given socket
 * @details it accepts an incoming connection on the socket returning
 * @details the socket number that can be used to communicate on this connection
 */
int accept_socket(thread_logger *thl, int socket);

/*!
 * @brief attempts to create a socket listening on the specified ip and port
 * @details this is a helper function to abstract away the verbosity required to
 * create a socket
 */
int listen_socket(thread_logger *thl, char *ip, char *port, bool tcp, bool ipv4,
                  SOCKET_OPTS sock_opts[], int num_opts);

/*! @brief  gets an available socket attached to bind_address
 * @return Success: file descriptor socket number greater than 0
 * @return Failure: -1
 * initializers a socket attached to bind_address with sock_opts, and binds the
 * address
 */
int get_new_socket(thread_logger *thl, addr_info *bind_address,
                   SOCKET_OPTS sock_opts[], int num_opts, bool client, bool tcp);

/*! @brief used to enable/disable blocking sockets
 * @return Failure: false
 * @return Success: true
 * @note see
 * https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking/1549344#1549344
 */
bool set_socket_blocking_status(int fd, bool blocking);

/*! @brief returns the address the client is connecting from
 */
char *get_name_info(sock_addr *client_address);

/*! @brief generates an addr_info struct with defaults
 * defaults is IPv4, TCP, and AI_PASSIVE flags
 */
addr_info default_hints();

void free_socket_client_t(socket_client_t *sock_client);