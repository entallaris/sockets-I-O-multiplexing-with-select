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
#define max_thread_handlers 50
#define max_client_connections 100000
#define TIMEOUT_SELECT 1

struct networkPacket {
  char packetType[200];
  uint16_t packetId;
  char key[256];

};


int debugINT;


void read_max_open_files();
pthread_mutex_t masterLock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier; 
fd_set master;
int max_open_files;
int active_connections;
int client_sockets[max_client_connections];

pthread_t thread_handlers[max_thread_handlers];

void * handlePackets(void * arg) {
  fd_set readfds;
  FD_ZERO(&readfds);
  struct networkPacket sendingPacket;
  int id = (intptr_t) arg;
  struct networkPacket receivedPacket;
  int selectret;
  struct timeval timeoutSelect;
  timeoutSelect.tv_sec = TIMEOUT_SELECT;
  int i;

  pthread_barrier_wait(&barrier);
  while (1) {
    readfds = master;
    selectret = select(active_connections + 4, & readfds, NULL, NULL, & timeoutSelect);
    if (selectret < 0) {
      //fprintf(stderr,"error in select: %s\n", explain_select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL));
      //exit(EXIT_FAILURE);
    } else {
      for (i = id; i < max_client_connections; i += max_thread_handlers) {
        if (client_sockets[i] != 0) {
          pthread_mutex_lock(&masterLock);
          if (FD_ISSET(client_sockets[i], &master)) {
            pthread_mutex_unlock(&masterLock);
            //printf("received from  %d|", client_sockets[i]);
            recv(client_sockets[i], & receivedPacket, sizeof(struct networkPacket), MSG_WAITALL);
            //printf("%s|", receivedPacket.packetType);
            sprintf(sendingPacket.key, "TESTKEY123");
            //printf("client_sockets[i] %d|", client_sockets[i]);
            debugINT++;
            send(client_sockets[i], &sendingPacket, sizeof(struct networkPacket), MSG_WAITALL);
            printf("I HAVE SENT %d packets\n", debugINT);
          } else {
            pthread_mutex_unlock(&masterLock);
          }

          pthread_mutex_lock(&masterLock);
          FD_CLR(client_sockets[i],&master);
          pthread_mutex_unlock(&masterLock);

          shutdown(client_sockets[i], SHUT_RDWR);
          while (recv(client_sockets[i], &receivedPacket, sizeof(struct networkPacket), MSG_WAITALL) != 0) {

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



int main(int argc, char * argv[]) {
  // SET PRIORITY
  nice(-20);
  
  pthread_barrier_init(&barrier, NULL, max_thread_handlers);
  // IGNORE SIGPIPE TERMINATION

  int socket_desc, client_sock, c, read_size;
  struct sockaddr_in server, client;

  // init sets
  FD_ZERO( & master);
  //FD_ZERO( & readfds);

  read_max_open_files();

  //Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  int enable = 1;
  if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, & enable, sizeof(int)) < 0) {
    printf("/error setsockopt\n");
    exit(-1);
  }
  //int option_value = 1; /* Set NOSIGPIPE to ON */
  //if (setsockopt(socket_desc, SOL_SOCKET, SO_NOSIGPIPE, & option_value, sizeof(option_value)) < 0) {
  //  perror("setsockopt(,,SO_NOSIGPIPE)");
  //}

  if (socket_desc == -1) {
    puts("Socket Failed");
  }
  puts("\nSocket Success!\n");

  //Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(7777);

  //Bind The Socket
  if (bind(socket_desc, (struct sockaddr * ) & server, sizeof(server)) < 0) {

    puts("\nBind Failed!\n");
    return 1;
  }
  puts("\nBind Success!\n");

  //Listen
  listen(socket_desc, max_client_connections);



  //creating threads for getting new packets
  for (int i = 0; i < max_thread_handlers; i++) {
    pthread_create( & thread_handlers[i], NULL, handlePackets, (void *)(intptr_t) i);
  }

  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);

  while (1) {
    if(active_connections+4<max_open_files){
        client_sock = accept(socket_desc, (struct sockaddr * ) & client, (socklen_t * ) & c);
        puts("\nConnection accepted\n");
        if (client_sock < 0) {
          puts("\nConnection with client failed\n");
          return 1;
        }
        pthread_mutex_lock( & masterLock);
        FD_SET(client_sock, & master);
        pthread_mutex_unlock( & masterLock);

        for (int i = 0; i < max_client_connections; i++) {
          if (client_sockets[i] == 0) {
            client_sockets[i] = client_sock;
            break;
          }
        }
        
        printf("New connection accepted %d\n", client_sock);
        active_connections++;

      }
    }

  //IN CASE WE COUNT RTT_S
  //Free Pointers.

  return 0;
}


void read_max_open_files(){

  FILE *fp;
  char cmd[1024];
  char max_open_files_buff[1024];

  sprintf(cmd, "ulimit -n");
  fp = popen(cmd, "r");
  fgets(max_open_files_buff, sizeof(max_open_files_buff), fp);
  pclose(fp);
  max_open_files = atoi(max_open_files_buff);
}