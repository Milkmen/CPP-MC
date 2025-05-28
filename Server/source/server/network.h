#ifndef INCLUDE_NETWORK_H
#define INCLUDE_NETWORK_H

#ifdef _WIN32
#define _WIN32_WINNT 0x0601  // Windows 7 minimum
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#define SOCK_ERR INVALID_SOCKET
#define SOCK_ERR_VAL SOCKET_ERROR
#else
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int socket_t;
#define CLOSE_SOCKET close
#define SOCK_ERR -1
#define SOCK_ERR_VAL -1
#endif

#endif // !INCLUDE_NETWORK_H