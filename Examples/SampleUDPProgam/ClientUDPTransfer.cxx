// Multicast Client
// written for LINUX
// Version 0.0.2
//
// Change: IP_MULTICAST_LOOP : Enable / Disable loopback for outgoing messages
//
// Compile : gcc -o server server.c
//
// This code has NOT been tested
//

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <iostream>
#include <string.h>


#if defined(_WIN32) && !defined(__CYGWIN__)
#define WSA_VERSION MAKEWORD(1,1)
#define igtlCloseSocketMacro(sock) (closesocket(sock))
#else
#define igtlCloseSocketMacro(sock) (shutdown(sock, 2))
#endif

#define MAXBUFSIZE 65536 // Max UDP Packet size is 64 Kbyte

int main()
{
  int sock, status, socklen;
  char buffer[MAXBUFSIZE];
  struct sockaddr_in saddr;
  struct ip_mreq imreq;
  
  // set content of struct saddr and imreq to zero
  memset(&saddr, 0, sizeof(struct sockaddr_in));
  memset(&imreq, 0, sizeof(struct ip_mreq));
  
#if defined(_WIN32) && !defined(__CYGWIN__)
  // Declare variables
  WSADATA wsaData;
  //SOCKET ListenSocket;
  //sockaddr_in service;
  
  //---------------------------------------
  // Initialize Winsock
  int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
  if( iResult != NO_ERROR )
  {
    std::cerr << "Error at WSAStartup" << std::endl;
    return -1;
  }
#endif
  
  // open a UDP socket
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if ( sock < 0 )
  {
   return 0;
  }
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(4096); // listen on port 4096
  saddr.sin_addr.s_addr = htonl(INADDR_ANY); // bind socket to any interface
  status = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  
  if ( status < 0 )
  {
    return 0;
  }
  imreq.imr_multiaddr.s_addr = inet_addr("226.0.0.1");
  imreq.imr_interface.s_addr = INADDR_ANY; // use DEFAULT interface
#if defined(_WIN32) && !defined(__CYGWIN__)
  status = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
    (const char *)&imreq, sizeof(struct ip_mreq));
#else
  // JOIN multicast group on default interface
  status = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                      (const void *)&imreq, sizeof(struct ip_mreq));
#endif  
  socklen = sizeof(struct sockaddr_in);
  while(1)
  {
    // receive packet from socket
    status = recvfrom(sock, buffer, MAXBUFSIZE, 0,
                      (struct sockaddr *)&saddr, (socklen_t *)&socklen);
    std::cerr<<buffer<<std::endl;
  }
  // shutdown socket
  igtlCloseSocketMacro(sock);
  
  return 0;
}