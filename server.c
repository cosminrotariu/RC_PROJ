#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
pthread_t identificatori[100];
int k = 0;
int sd; //socket descriptor
#define PORT 3000
int i;
extern int errno;

typedef struct thData
{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl;       //descriptorul intors de accept
} thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
int raspunde(void *);
struct sockaddr_in server; // structura folosita de server
struct sockaddr_in from;

void *read_stdin(void *arg) //in lucru
{

  char buf[5];
  read(0, &buf, sizeof(buf));
  printf("Am citit %s\n", buf);
  if (strstr(buf, "?exit") != NULL)
  {
    printf("[s] Serverul se va opri...\n");
    exit(0);
  }
  return NULL;
}
void get_filename_from_user()
{
  printf("Called get_filename_from_user()\n");
}
void call_db(char filename[30], char client_target[300])
{
  printf("Called call_db()\n");
}
void send_path_adress(char *path, char adress[300])
{
  printf("Called send_path_adress()\n");
}
void upload_to_db(char filename[30], char adress[300], int port)
{
  printf("Called upload_to_db()\n");
}

int main()
{
  int pid;
  i = 0;
  printf("Serverul a pornit. Se asteapta conexiuni...\n");
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[s] Eroare la socket().\n");
    return errno;
  }
  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[s] Eroare la bind().\n");
    return errno;
  }
  if (listen(sd, 2) == -1)
  {
    perror("[s] Eroare la listen().\n");
    return errno;
  }

  pthread_t t;
  pthread_create(&t, NULL, &read_stdin, NULL);

  while (1)
  {
    int client;
    thData *td; //parametru functia executata de thread
    int length = sizeof(from);
    fflush(stdout);
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[s] Eroare la accept().\n");
      continue;
    }
    // int idThread; //id-ul threadului
    // int cl; //descriptorul intors de accept

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);
  }
  pthread_join(t, NULL);
};

void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  while (1)
  {
    printf("[s] Asteptam mesajul de la clientul %s:%d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    fflush(stdout);
    int ok = raspunde((struct thData *)arg);
    if (ok == 0)
      break;
    /* am terminat cu acest client, inchidem conexiunea */
  }
  close((intptr_t)arg);
  return (NULL);
};

int raspunde(void *arg)
{
  char msg[100];
  struct thData tdL;
  tdL = *((struct thData *)arg);
  if (read(tdL.cl, msg, 100) <= 0)
  {
    perror("[s] Eroare la read() de la client.\n");
  }
  if (strcmp(msg, "?exit") == 0)
  {
    printf("[s] deconectat de la %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    return 0;
  }
  else
  {

    printf("[s] Mesajul receptionat de la clientul %s:%d este -> %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), msg);
    //printf("[s] Trimitem inapoi mesajul la clientul %s:%d... %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), nr);

    if (strcmp(msg, "?share") == 0)
    {
      char filename[30];
      printf("[s] Clientul doreste sa partajeze un fisier in retea.\n");
      get_filename_from_user();                                               //ii spune clientului sa introduca numele fisierului pe care doreste sa il partajeze cu reteaua
      upload_to_db(filename, inet_ntoa(from.sin_addr), ntohs(from.sin_port)); //pune in db numele fisierului, alaturi de adresa clientului
      char confirm[100] = "  [s] Fisier uploadat cu succes in baza de date.\n";
      if (write(tdL.cl, confirm, 100) <= 0)
      {
        perror("[s] Eroare la write() catre client.\n");
      }
    }
    else if (strcmp(msg, "?download") == 0)
    {
      char filename[30];
      char client_target[100];
      char *path;
      char adress[100];
      printf("[s] Clientul doreste sa descarce un fisier din retea.\n");
      get_filename_from_user();         //ii spune clientului sa introduca numele fisierului pe care doreste sa il descarce
      call_db(filename, client_target); //serverul cauta in baza de date clientul care are acel fisier si obtine adresa clientului
      send_path_adress(path, adress);   //ii trimite clientului initial adresa clientului
    }

    if (write(tdL.cl, msg, 100) <= 0)
    {
      perror("[s] Eroare la write() catre client.\n");
    }
    // else
    //   printf("[server] Mesajul a fost trasmis cu succes catre clientul %s:%d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
  }
}
