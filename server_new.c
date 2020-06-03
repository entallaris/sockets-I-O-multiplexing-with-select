/*
        This is the one who has the reference clock and synchronizes with it.
 */

#include <stdio.h> 
#include <string.h> //strlen
#include <sys/socket.h> 
#include <arpa/inet.h> //inet_addr
#include <unistd.h> //write
#include <sys/time.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h> 
#include <signal.h>
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <pthread.h>
#define max_thread_handlers 10
#define max_client_connections 1000000
#define TIMEOUT_SELECT 1
#define SERVER_PORT 7777

struct networkPacket {
  char packetType[200];
  uint16_t packetId;
  char key[256];

};

int debugINT;

pthread_mutex_t masterLock = PTHREAD_MUTEX_INITIALIZER;

fd_set master, readfds;
int max_sock;
int active_connections;
int client_sockets[max_client_connections];

pthread_t thread_handlers[max_thread_handlers];


void * handlePackets(void * );
int main(int argc, char * argv[]) {
  // SET PRIORITY
  nice(-20);

  // IGNORE SIGPIPE TERMINATION

  int socket_desc, client_sock, c, read_size;
  struct sockaddr_in server, client;

  // init sets
  FD_ZERO( & master);
  FD_ZERO( & readfds);

  //Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  int enable = 1;
  if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, & enable, sizeof(int)) < 0) {
    printf("/error setsockopt\n");
    exit(-1);
  }
 // int option_value = 1; /* Set NOSIGPIPE to ON */
 // if (setsockopt(socket_desc, SOL_SOCKET, SO_NOSIGPIPE, & option_value, sizeof(option_value)) < 0) {
 //   perror("setsockopt(,,SO_NOSIGPIPE)");
 //}

  if (socket_desc == -1) {
    puts("Socket Failed");
  }
  puts("\nSocket Success!\n");

  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(SERVER_PORT);

  //Bind The Socket
  if (bind(socket_desc, (struct sockaddr * ) & server, sizeof(server)) < 0) {

    puts("\nBind Failed!\n");
    return 1;
  }
  puts("\nBind Success!\n");

  //Listen
  listen(socket_desc, max_client_connections);
  //creating threads for getting new packets
  for (long i = 0; i < max_thread_handlers; i++) {
    pthread_create( & thread_handlers[i], NULL, handlePackets, (void * ) i);
  }

  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);

  while (1) {
    client_sock = accept(socket_desc, (struct sockaddr * ) & client, (socklen_t * ) & c);
    puts("\nConnection accepted\n");
    if (client_sock < 0) {
      puts("\nConnection with client failed\n");
      return 1;
    }
    pthread_mutex_lock( & masterLock);
    FD_SET(client_sock, & master);
    printf("New connection accepted %d\n", client_sock);
    pthread_mutex_unlock( & masterLock);
    for (int i = 0; i < max_client_connections; i++) {
      if (client_sockets[i] == 0) {
        client_sockets[i] = client_sock;
        break;
      }
    }
    active_connections++;

  }

  //Free Pointers.

  return 0;
}


void * handlePackets(void * arg) {
  struct networkPacket sendingPacket;
  printf("%ld\n", (long) arg);
  struct networkPacket receivedPacket;
  int selectret;
  struct timeval timeoutSelect;
  timeoutSelect.tv_sec = TIMEOUT_SELECT;

  while (1) {

    readfds = master;
    selectret = select(active_connections + 4, & readfds, NULL, NULL, & timeoutSelect);

    if (selectret < 0) {
      //fprintf(stderr,"error in select: %s\n", explain_select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL));
      //exit(EXIT_FAILURE);
    } else {
      for (int i = (long) arg; i < max_client_connections; i += max_thread_handlers) {
        
        if (client_sockets[i] != 0) {
        
          pthread_mutex_lock(&masterLock);
          if (FD_ISSET(client_sockets[i], &master)) {
            
            pthread_mutex_unlock(&masterLock);
            //printf("received from  %d|", client_sockets[i]);
            recv(client_sockets[i], & receivedPacket, sizeof(struct networkPacket), MSG_WAITALL);
            printf("%s|", receivedPacket.packetType);
            sprintf(sendingPacket.key, "TESTKEY123");;
            //printf("client_sockets[i] %d|", client_sockets[i]);
            send(client_sockets[i], & sendingPacket, sizeof(struct networkPacket), MSG_WAITALL);

          } else {
            pthread_mutex_unlock(&masterLock);
          }

          pthread_mutex_lock(&masterLock);
          FD_CLR(client_sockets[i],&master);
          pthread_mutex_unlock(&masterLock);
          
          shutdown(client_sockets[i], SHUT_RDWR);
          while (recv(client_sockets[i], & receivedPacket, sizeof(struct networkPacket), MSG_WAITALL) != 0) {

          }
          printf("Closing\n");
          close(client_sockets[i]);
          client_sockets[i] = 0;
          active_connections--;
        }

      }

    }

  }

  return 0;
}
