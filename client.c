#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "shared.h"
pthread_t t[1];
int printRandoms(int lower, int upper,int count)
{
  int i;
  for (i = 0; i < count; i++)
  {
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
  }
}

int portul;
extern int errno;
static void *serv(void *arg)
{
  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  char msg[100];           //mesajul primit de la client
  char msgrasp[100] = " "; //mesaj de raspuns pentru client
  int sd;                  //descriptorul de socket

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[c] Eroare la socket().\n");
  }

  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(portul);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[c] Eroare la bind().\n");
  }

  if (listen(sd, 5) == -1)
  {
    perror("[c] Eroare la listen().\n");
  }

  while (1)
  {
    int client;
    int length = sizeof(from);

    // printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    client = accept(sd, (struct sockaddr *)&from, &length);

    if (client < 0)
    {
      perror("[c] Eroare la accept().\n");
      continue;
    }
    printf("\n[c] S-a conectat cineva.\n");
    bzero(msg, 100);
    //printf("[server]Asteptam mesajul...\n");
    fflush(stdout);

    if (read(client, msg, 100) <= 0)
    {
      perror("[c] Eroare la read() de la client.\n");
      close(client);
      continue;
    }

    printf("[c] Mesajul a fost receptionat...%s\n", msg);

    bzero(msgrasp, 100);
    strcat(msgrasp, "Hello ");
    strcat(msgrasp, msg);

    printf("[c] Trimitem mesajul inapoi...%s\n", msgrasp);

    if (write(client, msgrasp, 100) <= 0)
    {
      perror("[server]Eroare la write() catre client.\n");
      continue;
    }
    else
      printf("[c] Mesajul a fost transmis cu succes.\n");
    close(client);
  }
}

void downloadFile(int port, char filename[]){ 
    //trb pus si un filename
    printf("[c] Conectare la client in curs...\n");
    int sd2;
    struct sockaddr_in server2;
    if ((sd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("eroare la socket\n");
    }

    server2.sin_family = AF_INET;
    server2.sin_addr.s_addr = inet_addr("127.0.0.1");
    server2.sin_port = htons(port);

    if (connect(sd2, (struct sockaddr *)&server2, sizeof(struct sockaddr)) == -1)
    {
      perror("[c] Eroare la connect().\n");
    }
    printf("[c] Conectat la client\n");
    char msg[100];
    bzero(msg, 100);
    if (write(sd2, filename, strlen(filename)) <= 0)
    {
      perror("[c] Eroare la write() spre server.\n");
    }

    if (read(sd2, msg, sizeof(msg)) < 0)
    {
      perror("[client]Eroare la read() de la server.\n");
    }
    printf("[c] Mesajul primit este: %s\n", msg);

    close(sd2);
}

int main(int argc, char *argv[])
{
  srand(time(0));
  portul = printRandoms(3001, 99999, 1);
  printf("PORTUL: %d\n", portul);

  char msg[MESSAGE_BUFFER_SIZE];

  pthread_create(&t[0], NULL, &serv, NULL);
  int sd; // socket descriptor
  struct sockaddr_in server;
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(3000);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[c] Eroare la connect().\n");
    return errno;
  }
  if (write(sd, &portul, sizeof(portul)) <= 0)
    {
      perror("[c] Eroare la write() spre server.\n");
      return errno;
    }
  printf("[s] Bun venit.\n Pentru a va loga folositi comanda '?login user:parola'. \n Daca nu aveti cont, folositi comanda '?register user:parola pentru a va crea unul.\n");
  while (1)
  {
    printf("[c] Introduceti o comanda: ");
    fflush(stdout);
    int r = read(0, msg, sizeof(msg));
    if (r <= 0)
    {
      perror("[c] Eroare la read() de la server.\n");
      return errno;
    }
    msg[r - 1] = '\0';
    printf("[c] Am citit: %s\n", msg);
    if (strstr(msg, "?download"))
    {
      char filename[20];
      char *token = strtok(msg," ");
      token = strtok(NULL," ");
      int portToConnect = atoi(token);
      token = strtok(NULL," ");
      strcpy(filename,token);
      printf("Port: %d, Filename: %s\n",portToConnect,filename);
      downloadFile(portToConnect,filename);
    }else{
    if (write(sd, &msg, sizeof(msg)) <= 0)
    {
      perror("[c] Eroare la write() spre server.\n");
      return errno;
    }
    if (strcmp(msg, "?exit") == 0)
    {
      close(sd);
      printf("[c] Deconectat de la server.\n");
      exit(1);
    }
    char rec[MESSAGE_BUFFER_SIZE];
    if (read(sd, &rec, sizeof(rec)) <= 0)
    {
      perror("[c] Eroare la read() de la server.\n");
      return errno;
    }
    printf("[s] %s\n", rec);
  }
  }
  close(sd);

  pthread_join(t[0], NULL);
}