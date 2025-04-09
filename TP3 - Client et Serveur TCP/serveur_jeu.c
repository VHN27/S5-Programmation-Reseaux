#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

int main(void)
{
  int ret = 0;
  int socket_serveur = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr_serveur;

  if (socket_serveur == -1)
  {
    perror("socket_serveur");
    return 1;
  }

  memset(&addr_serveur, 0, sizeof(addr_serveur));
  addr_serveur.sin_family = AF_INET;
  addr_serveur.sin_port = htons(8000);
  if (inet_pton(AF_INET,"127.0.0.1", &addr_serveur.sin_addr) == -1)
  {
    perror("inet_pton");
    ret = 1;
    goto end;
  }

  if (bind (socket_serveur, (struct sockaddr *) &addr_serveur, sizeof(addr_serveur)) == -1){
    perror("bind");
    ret = 1;
    goto end;
  }

  if (listen(socket_serveur, 0) == -1){
    perror("listen");
    ret = 1;
    goto end;
  }

  int socket_client = accept(socket_serveur, NULL, NULL);
  if (socket_client == -1)
  {
    perror("accept");
    ret = 1;
    goto end;   
  }
   
  srand (time(NULL));
  int random_number = rand() % 65536;
  int tentative = 20;
  char buffer[32] = "\0";
  int bytes_recv;

  while(tentative >= 0){
    if (tentative == 0)
    {
      snprintf(buffer, 32, "PERDU\n");
      if (send(socket_client, buffer, strlen(buffer), 0) == -1)
      {
          perror("send");
          ret = 1;
          close(socket_client);
          goto end;
      }
      break;
    }
    if ((bytes_recv = recv(socket_client, buffer, sizeof(buffer) - 1, 0)) == -1)
    {
      perror("recv");
      ret = 1;
      goto end;   
    }
    buffer[bytes_recv] = '\0';
    printf("recv: %s\n", buffer);
    int k;
    sscanf(buffer, "%d", &k);
    if (k > random_number)
        snprintf(buffer, 32, "MOINS %d\n", tentative);
    else if (k == random_number)
    {
      if (send(socket_client, buffer, strlen(buffer), 0) == -1)
      {
          perror("send");
          ret = 1;
          close(socket_client);
          goto end;
      }
      break;
    }
    else
        snprintf(buffer, 32, "PLUS %d\n", tentative);
    if (send(socket_client, buffer, strlen(buffer), 0) == -1)
    {
        perror("send");
        ret = 1;
        close(socket_client);
        goto end;
    }
    printf("send: %s\n", buffer);

    tentative--;
  }



end:
    close(socket_serveur);
    if (ret == 0)
        close(socket_client);
    return ret;
}
