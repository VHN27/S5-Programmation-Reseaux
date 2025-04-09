#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
    int ret = 0;
    int socket_server = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_server == -1)
    {
        perror("socket");
        return 1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2407);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == -1)
    {
        perror("inet_pton");
        ret = 1;
        goto end;
    }
    if (bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        ret = 1;
        goto end;
    }
    if (listen(socket_server, 0) == -1)
    {
        perror("listen");
        ret = 1;
        goto end;
    }
    int socket_client = accept(socket_server, NULL, NULL);
    if (socket_client == -1)
    {
        perror("accept");
        ret = 1;
        goto end;
    }
    while (1)
    {
        char buffer[1024] = "\0";
        if (recv(socket_client, buffer, 1024, 0) == -1)
        {
            perror("recv");
            ret = 1;
            close(socket_client);
            goto end;
        }
        if (send(socket_client, buffer, strlen(buffer), 0) == -1)
        {
            perror("send");
            ret = 1;
            close(socket_client);
            goto end;
        }
    }
end:
    close(socket_server);
    if (ret == 0)
        close(socket_client);
    return ret;
}
