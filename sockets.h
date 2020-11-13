
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include "deps/ulog/logger.h"

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
  * @brief attempts to create a socket listening on the specified ip and port
  * @details this is a helper function to abstract away the verbosity required to create a socket
*/
int listen_socket(thread_logger *thl, char *ip, char *port, bool tcp, bool ipv4, SOCKET_OPTS sock_opts[], int num_opts);

/*! @brief  gets an available socket attached to bind_address
  * @return Success: file descriptor socket number greater than 0
  * @return Failure: -1
  * initializers a socket attached to bind_address with sock_opts, and binds the address
*/
int get_new_socket(thread_logger *thl, addr_info *bind_address, SOCKET_OPTS sock_opts[], int num_opts);


/*! @brief used to enable/disable blocking sockets
  * @return Failure: false
  * @return Success: true
  * @note see https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking/1549344#1549344
*/
bool set_socket_blocking_status(int fd, bool blocking);


/*! @brief returns the address the client is connecting from
*/
char  *get_name_info(sock_addr *client_address);

/*! @brief generates an addr_info struct with defaults
  * defaults is IPv4, TCP, and AI_PASSIVE flags
*/
addr_info default_hints();