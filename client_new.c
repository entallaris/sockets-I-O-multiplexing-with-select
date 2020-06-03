#include<stdio.h> 
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h> 
#include <sys/time.h>
#include<unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define numberOfPackets 18000
#define correctionValue 10

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7777

struct networkPacket
{
    char packetType[200];
    uint16_t packetId;
    char key[256];
    
};


int main(int argc , char *argv[])
{

    // SET PRIORITY
    nice(-20);

    int sock;
    struct sockaddr_in server;
   
    int read_size;
    // RTT_packet gets a timestamp on each receive. This is done to avoid any delay that the code can add.
    struct timeval RTT_packet,RTT_start,RTT_end,RTT_client; 
    // recvPacketServer is the packet that client receives from the server
    // replyPacket is the packet that the client sents back to the client.
    struct networkPacket recvPacketServer, replyPacket;

    //Create socket

    sock = socket(AF_INET , SOCK_STREAM , 0);

    if (sock == -1){
        printf("Socket Failed!");
    }
    puts("Socket Success!");

    server.sin_addr.s_addr = inet_addr( SERVER_IP );
    server.sin_family = AF_INET;
    server.sin_port = htons( SERVER_PORT );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
        puts("\nConnect Failed!\n");
        return 1;
    }
     
    puts("\nConnect Success\n");

    // Start RTT Procedure.
    struct networkPacket packet;

    while(1){
      sprintf(packet.packetType, "%d,test", getpid());
      printf("Sending \n");
      send(sock , &packet , sizeof(struct networkPacket) , 0);
      recv(sock, &recvPacketServer, sizeof(struct networkPacket), 0);
      printf("%s\n", recvPacketServer.key);
      while(recv(sock, &recvPacketServer, sizeof(struct networkPacket), 0) != 0){

      }
      break;
      
    }


    return 0;
}
