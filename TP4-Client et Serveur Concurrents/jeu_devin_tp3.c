#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 100

int server_1p();
void game_1p(int sock);


int main(int argn,char** args) {
  if(argn<=1|| strcmp(args[1],"-1p")==0){
    server_1p();
  }
}

/***************LES FONCTIONS DE LECTURE ET ECRITURE SUR SOCKET ******************/

int lire_nb(int sock, char *buf) {
	int taille, ind = 0;
	do {
		if((taille = recv(sock, buf+ind, BUFSIZE-1-ind, 0)) < 0) {
			perror("recv");
			exit(1);
		}
		ind += taille;
	} while(ind < BUFSIZE-1 && taille != 0 && !memchr(buf, '\n', ind));
	
	if(ind > BUFSIZE){
		fprintf(stderr, "erreur format message recu\n");
		exit(1);
	}
	buf[ind] = 0;

	return ind;
}

int ecrire_nb(int sock, char *buf) {
	int lg = 0, lu;
	do {
		if((lu = send(sock, buf, strlen(buf), 0)) <= 0) {
			perror("send");
			exit(1);
		}
		lg += lu;
	} while(lg < strlen(buf));

	return lg;
}

/***************LES FONCTIONS DE JEU ******************/	

/**
 * fait tourner une partie pour un joueur dont le socket est passé en argument
 */

void* game (void* arg) {
  int *joueur = (int *)  arg;
  game_1p(*joueur);
  close(*joueur);
  free(joueur);
  return NULL;
}

void game_1p(int sock) {
  srand(time(NULL) + sock);
  
  // une valeur aléatoire mystere entre 0 et 65536
  unsigned short int n = rand() % (1 << 16); 
  printf("nb mystere pour partie socket %d = %d\n", sock, n);

  unsigned short int guess;  // le nb proposé par le joueur, sur 2 octets
  int taille = 0;
  int tentatives = 20;
  int gagne = 0;
  char buff_in[BUFSIZE];
  while ((taille = lire_nb(sock, buff_in)) > 0) {
    sscanf(buff_in, "%hu", &guess);
    printf("Joueur courant a envoyé : %d\n", guess);
    char reponse[20];
    if (n < guess || n > guess) {
      tentatives--;
    }
    if (tentatives == 0)
      sprintf(reponse, "PERDU\n");
    else if (n < guess)
      sprintf(reponse, "MOINS %d\n", tentatives);
    else if (n > guess)
      sprintf(reponse, "PLUS %d\n", tentatives);
    else {
      sprintf(reponse, "GAGNE\n");
      gagne = 1;
    }
    ecrire_nb(sock, reponse);
    if (gagne || !tentatives) break;
  }
  printf("Fin de partie\n");

  close(sock);
}

/**
 * serveur pour jeu 1 player avec 1 connexion à la fois
 */
int server_1p_fork(int serv_sock) {
  int client_sock = 0;
  while (1) {
    client_sock = accept(serv_sock, NULL, NULL);
    if (client_sock < 0) {
      fprintf(stderr, "Échec de accept");
      exit(-1);
    } 
    else {
      pid_t pid;
      if ((pid = fork()) == 0) {
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
            pthread_create(&thread, NULL, game, client_sock);
            printf("Connexion acceptee, nouvelle partie lancée.\n");
        }
    }
    return 0;
}

int server_1p() {
  int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(4242);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (r != 0) {
    fprintf(stderr, "Échec de bind");
    exit(-1);
  }

  r = listen(serv_sock, 0);
  if (r != 0) {
    fprintf(stderr, "Échec de listen");
    exit(-1);
  }

  server_1p_threads(serv_sock);

  return 0;
}
