#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 16

int main (){
  int ret = 0;
  int sock_serveur = socket(PF_INET, SOCK_STREAM, 0);
  if (sock_serveur == -1 ){
    perror("socket_serveur");
    return 1;
  }

  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons (8888);

  if (inet_pton(AF_INET, "127.0.0.1", &sock_addr.sin_addr) == -1){
    perror("inet_pton");
    ret = 1;
    goto end;
  }

  if (connect (sock_serveur, (struct sockaddr* )&sock_addr, sizeof(sock_addr)) == -1){
    perror("connect");
    ret = 1;
    goto end;
  }

  srand (getpid());
  int guess = rand () % (0xFFFF + 1);
  int min = 0;
  int max = 0xFFFF + 1;

  while (1){
    char buffer[16];
    int len;
    int total_bytes_sent = 0;
    int bytes_sent;
    int total_bytes_recv = 0;
    int bytes_recv;

    snprintf(buffer, BUFFER_SIZE, "%d\n", guess);
    printf("send: %s\n", buffer);
    len = strlen(buffer);

    while (total_bytes_sent < len){
      if ((bytes_sent = send(sock_serveur, buffer + total_bytes_sent, len - total_bytes_sent, 0)) == -1){
        perror("send");
        ret = 1;
        break;
      }
      total_bytes_sent += bytes_sent;
    }

    while (total_bytes_recv < BUFFER_SIZE && !memchr(buffer, '\n', total_bytes_recv)){
      if ((bytes_recv = recv(sock_serveur, buffer + total_bytes_recv, BUFFER_SIZE - 1 - total_bytes_recv, 0)) == -1){
        perror("recv");
        ret = 1;
        break;
      }
      total_bytes_recv += bytes_recv;
    }

    buffer[total_bytes_recv] = '\0';
    printf("recv: %s\n", buffer);
    if (strncmp(buffer, "PLUS", 4) == 0)
      min = guess;
    else if (strncmp(buffer, "MOINS", 5) == 0)
      max = guess;
    else
        break;
    guess = (min + max) / 2;
    
  }


  end:
    close(sock_serveur);
    return ret;
}