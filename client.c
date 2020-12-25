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
extern int errno;
int port = 3000;
int main(int argc, char *argv[])
{

  int sd; // socket descriptor
  struct sockaddr_in server;
  char buf[100];
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
  printf("[s] V-ati conectat cu succes la server.\n Bun venit. \n Alegeti o comanda: \n 1. ?download \n 2. ?share \n 3. ?exit\n");

  while (1)
  {
    printf("[c] Introduceti un mesaj: ");
    fflush(stdout);
    char msg[100] = " ";
    strcpy(msg, buf);
    read(0, msg, sizeof(msg));
    msg[strlen(msg) - 1] = '\0';
    printf("[c] Am citit: %s\n", msg);
    if (write(sd, &msg, sizeof(msg)) <= 0)
    {
      perror("[c] Eroare la write() spre server.\n");
      return errno;
    }
    else
    {
      if (strcmp(msg, "?exit") == 0)
      {
        close(sd);
        printf("[c] Deconectat de la server.\n");
        exit(1);
      }
    }
    if (read(sd, &msg, sizeof(msg)) < 0)
    {
      perror("[c] Eroare la read() de la server.\n");
      return errno;
    }
    printf("[c] Mesajul primit de la server este: %s\n\n", msg);
  }
  close(sd);
}