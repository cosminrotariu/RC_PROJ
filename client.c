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
#include "shared.h"

extern int errno;
int port = 3000;
int main(int argc, char *argv[])
{
  int sd; // socket descriptor
  struct sockaddr_in server;
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[c] Eroare la connect().\n");
    return errno;
  }
  printf("[s] Bun venit.\n Pentru a va loga folositi comanda '?login user:parola'. \n Daca nu aveti cont, folositi comanda '?register user:parola pentru a va crea unul.\n");

  while (1)
  {
    printf("[c] Introduceti o comanda: ");
    fflush(stdout);
    char msg[MESSAGE_BUFFER_SIZE];
    //char msg[300];
    //strcpy(msg, buf);
    int r = read(0, msg, sizeof(msg));
    if (r <= 0)
    {
      perror("[c] Eroare la read() de la server.\n");
      return errno;
    }
    msg[r - 1] = '\0';
    printf("[c] Am citit: %s\n", msg);
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
  close(sd);
}