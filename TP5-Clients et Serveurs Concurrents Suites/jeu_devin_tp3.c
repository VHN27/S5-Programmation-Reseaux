#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 100


int party_index = 0;
int k;
int n;
int serv_sock;
typedef struct
{
    int index;
    int rank;
    int partie_gagne;
    int index_client;
    int **clients_socks;
    pthread_mutex_t recv_message;
    pthread_t *threads;
    int mystery_number;
} party;

int initialize_socket();
void *game_np_point(void *);
int server_1p_fork(int);
int server_1p_threads(int);
int server_np();

int main(int argn, char **args)
{
    serv_sock = initialize_socket();
    int ret = 0;
    if (argn <= 1 || strcmp(args[1], "-1p") == 0)
    {
        ret = server_1p_fork(serv_sock);
        // ret = server_1p_threads(serv_sock);
    }
    else
    {
        sscanf(args[1], "%d", &n);
        sscanf(args[2], "%d", &k);
        ret = server_np();
    }
    return ret;
}

int initialize_socket()
{
    int serv_sock = socket(PF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 serv_addr;
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(8888);
    serv_addr.sin6_addr = in6addr_any;
    int only6 = 0;
    setsockopt(serv_sock, IPPROTO_IPV6, IPV6_V6ONLY, &only6, sizeof(only6));

    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0)
    {
        fprintf(stderr, "Échec de bind");
        exit(-1);
    }

    r = listen(serv_sock, 0);
    if (r != 0)
    {
        fprintf(stderr, "Échec de listen");
        exit(-1);
    }


    return serv_sock;
}

void *initialize_party(void *arg)
{
    party *new_party = (party *)arg;
    srand(time(NULL) + new_party->index);
    new_party->mystery_number = rand() % (1 << 16);
    printf("Nombre mystère: %d\n", new_party->mystery_number);
    for (int i = 0; i < n; i++)
        pthread_create(new_party->threads + i, NULL, game_np_point, new_party);
    for (int i = 0; i < n; i++)
        pthread_join(*(new_party->threads + i), NULL);
    pthread_mutex_destroy(&new_party->recv_message);
    free(new_party->clients_socks);
    free(new_party->threads);
    free(new_party);
    return NULL;
}
/***************LES FONCTIONS DE LECTURE ET ECRITURE SUR SOCKET ******************/

int lire_nb(int sock, char *buf)
{
    int taille, ind = 0;
    do
    {
        if ((taille = recv(sock, buf + ind, BUFSIZE - 1 - ind, 0)) < 0)
        {
            perror("recv");
            exit(1);
        }
        ind += taille;
    } while (ind < BUFSIZE - 1 && taille != 0 && !memchr(buf, '\n', ind));

    if (ind > BUFSIZE)
    {
        fprintf(stderr, "erreur format message recu\n");
        exit(1);
    }
    buf[ind] = 0;

    return ind;
}

int ecrire_nb(int sock, char *buf)
{
    int lg = 0, lu;
    do
    {
        if ((lu = send(sock, buf, strlen(buf), 0)) <= 0)
        {
            perror("send");
            exit(1);
        }
        lg += lu;
    } while (lg < (int)strlen(buf));

    return lg;
}

/***************LES FONCTIONS DE JEU ******************/

/**
 * fait tourner une partie pour un joueur dont le socket est passé en argument
 */
void game_1p(int sock)
{
    srand(time(NULL) + sock);

    // une valeur aléatoire mystere entre 0 et 65536
    unsigned short int n = rand() % (1 << 16);
    printf("nb mystere pour partie socket %d = %d\n", sock, n);

    unsigned short int guess; // le nb proposé par le joueur, sur 2 octets
    int taille = 0;
    int tentatives = 20;
    int gagne = 0;
    char buff_in[BUFSIZE];
    while ((taille = lire_nb(sock, buff_in)) > 0)
    {
        sscanf(buff_in, "%hu", &guess);
        printf("Joueur courant a envoyé : %d\n", guess);
        char reponse[20];
        if (n < guess || n > guess)
            tentatives--;
        if (tentatives == 0)
            sprintf(reponse, "PERDU\n");
        else if (n < guess)
            sprintf(reponse, "MOINS %d\n", tentatives);
        else if (n > guess)
            sprintf(reponse, "PLUS %d\n", tentatives);
        else
        {
            sprintf(reponse, "GAGNE\n");
            gagne = 1;
        }
        ecrire_nb(sock, reponse);
        if (gagne || !tentatives)
            break;
    }
    printf("Fin de partie\n");
}

void *game_1p_point(void *arg)
{
    int *joueurs = (int *)arg;
    game_1p(*joueurs);
    close(*joueurs);
    free(arg);
    return NULL;
}

void game_np(int sock, int k, party *party)
{
    unsigned short int guess; // le nb proposé par le joueur, sur 2 octets
    int taille = 0;
    int gagne = 0;
    char buff_in[BUFSIZE];
    while ((taille = lire_nb(sock, buff_in)) > 0)
    {
        sscanf(buff_in, "%hu", &guess);
        printf("Joueur %d de la partie %d a envoyé : %d\n", sock, party->index, guess);
        char reponse[20];
        if (party->mystery_number != guess)
            k--;
        if (k == 0)
            sprintf(reponse, "PERDU\n");
        
        else if (party->mystery_number < guess)
            sprintf(reponse, "MOINS %d\n", k);
        else if (party->mystery_number > guess)
            sprintf(reponse, "PLUS %d\n", k);
        else
        {
            pthread_mutex_lock(&party->recv_message);
            if (!party->partie_gagne){
                sprintf(reponse, "GAGNE %d\n", party->rank++);
                party->partie_gagne = sock; 
            }
            
            pthread_mutex_unlock(&party->recv_message);

            gagne = 1;
        }

        printf ("k = %d et partie_gagne = %d\n", k, party->partie_gagne);
        pthread_mutex_lock(&party->recv_message);
        if (party->partie_gagne == sock || !party->partie_gagne){
            ecrire_nb(sock, reponse);

        }else{
            k = 0;
            ecrire_nb(sock, "PERDU\n");
        }
        pthread_mutex_unlock(&party->recv_message);
        if (gagne || !k){
            printf("sortie avec k = %d et gagne = %d\n", k, gagne);
            break;
        }
    }
}

void *game_np_point(void *arg)
{
    party *new_party = ((party *)arg);
    int *joueurs = *(new_party->clients_socks + new_party->index_client++);
    game_np(*joueurs, k, new_party);
    close(*joueurs);
    free(joueurs);
    return NULL;
}

/**
 * serveur pour jeu 1 player avec 1 connexion à la fois
 */
int server_1p_fork(int serv_sock)
{
    int client_sock = 0;
    while (1)
    {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock < 0)
        {
            fprintf(stderr, "Échec de accept");
            exit(-1);
        }
        else
        {
            pid_t pid;
            if ((pid = fork()) == 0)
            {
                printf("Connexion acceptee, nouvelle partie lancée.\n");
                game_1p(client_sock);
            }
            waitpid(pid, NULL, WNOHANG);
            close(client_sock);
        }
    }
    close(serv_sock);
    return 0;
}

int server_1p_threads(int serv_sock)
{
    while (1)
    {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(serv_sock, NULL, NULL);
        if (*client_sock < 0)
        {
            fprintf(stderr, "Échec de accept");
            exit(1);
        }
        else
        {
            pthread_t thread;
            pthread_create(&thread, NULL, game_1p_point, client_sock);
            printf("Connexion acceptee, nouvelle partie lancée.\n");
        }
    }
    return 0;
}

int server_np()
{
    while (1)
    {
        party *new_party = malloc(sizeof(party));
        new_party->index = party_index++;
        new_party->rank = 1;
        new_party->partie_gagne = 0;
        new_party->index_client = 0;
        new_party->clients_socks = malloc(n * sizeof(int *));
        new_party->threads = malloc(n * sizeof(pthread_t));
        pthread_mutex_init(&new_party->recv_message, NULL);
        for (int i = 0; i < n; i++)
        {
            *(new_party->clients_socks + i) = malloc(sizeof(int));
            *(*(new_party->clients_socks + i)) = accept(serv_sock, NULL, NULL);
            printf("Joueur %d a rejoint la partie %d\n", *(*(new_party->clients_socks + i)), new_party->index);
        }
        pthread_t thread;
        pthread_create(&thread, NULL, initialize_party, new_party);

    }
    return 0;
}
