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
#include "sqlite3.h"

char *error;
sqlite3 *db;
sqlite3_stmt *stmt;
pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
pthread_t identificatori[100];
int k = 0;
int OK = 0;
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
int logare(void *arg);
void *treat(void *arg);
void *read_stdin(void *arg) //in lucru
{

  char buf[5];
  read(0, &buf, sizeof(buf));
  printf("Am citit %s\n", buf);
  if (strstr(buf, "?exit") != NULL)
  {
    printf("[CONSOLE]:  Serverul se va opri...\n");
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
int login(char msg[500], void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  if (strstr(msg, "?login"))
  {
    char password[25];
    char username[25];
    char credentials[50];
    int lg = 0;
    strcpy(credentials, msg + 7);
    for (int i = 0; strlen(credentials); i++)
    {
      if (credentials[i] == ':')
      {
        for (int j = i + 1; j < strlen(credentials); j++)
        {
          password[lg++] = credentials[j];
        }
        //password[strlen(credentials)] = '\0';
        for (int k = 0; k < i; k++)
        {
          username[k] = credentials[k];
        }
        username[i] = '\0';
        break;
      }
    }
    sqlite3_prepare_v2(db, "select * from login;", -1, &stmt, 0);
    char *user, *pass;
    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
      user = sqlite3_column_text(stmt, 0);
      pass = sqlite3_column_text(stmt, 1);
      if (strcmp(username, user) == 0)
      {
        if (strcmp(password, pass) == 0)
        {
          // printf("logare reusita\n");
          return 1;
        }
        else
        {
          //printf("parola nu este buna\n");
          if (write(tdL.cl, "parola incorecta", 100) <= 0)
          {
            perror("[CONSOLE]:  Eroare la write() catre client.\n");
          }
          return 0;
        }
      }
    }
    //printf("contul nu exista. va rugam sa va creati un cont\n");
    if (write(tdL.cl, "contul nu exista.", 100) <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
    return 0;
  }
  else if (strstr(msg, "?register"))
  {
    char password[25];
    char username[25];
    char credentials[50];
    int lg = 0;
    strcpy(credentials, msg + 10);
    for (int i = 0; i < strlen(credentials); i++)
    {
      if (credentials[i] == ':')
      {
        for (int j = i + 1; j < strlen(credentials); j++)
        {
          password[lg++] = credentials[j];
        }
        memmove(&password[lg], &password[lg + (strlen(password) - lg)], strlen(password) - lg);
        for (int k = 0; k < i; k++)
        {
          username[k] = credentials[k];
        }
        username[i] = '\0';
        break;
      }
    }
    //printf("user: %s, pass: %s\n", username, password);
    sqlite3_prepare_v2(db, "select user from login;", -1, &stmt, 0);
    char *user;
    int ok2 = 1;
    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
      user = sqlite3_column_text(stmt, 0);
      if (strcmp(username, user) == 0)
      {
        //printf("contul exista deja. va rugam sa alegeti alt user.\n");
        if (write(tdL.cl, "contul exista deja.", 100) <= 0)
        {
          perror("[CONSOLE]:  Eroare la write() catre client.\n");
        }
        return 0;
      }
    }
    if (ok2 == 1)
    {

      char insert[] = "insert into login VALUES(\"";
      strcpy(insert + strlen(insert), username);
      strcpy(insert + strlen(insert), "\",\"");
      strcpy(insert + strlen(insert), password);
      strcpy(insert + strlen(insert), "\");");
      int verif_insert = sqlite3_exec(db, insert, NULL, NULL, &error);
      if (verif_insert != SQLITE_OK)
      {
        printf("[CONSOLE]:  eroare: %s\n", error);
      }
      return 1;
    }
  }
}
int main()
{
  sqlite3_open("myDb.db", &db);
  int verif_login_table = sqlite3_exec(db, "create table if not exists login(user varchar(50), pass varchar(50));", NULL, NULL, &error);
  if (verif_login_table != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s", error);
  }
  int verif_shared_files_table = sqlite3_exec(db, "create table if not exists files(port int, filename varchar(50), dir_parent varchar(50), extension varchar(10));", NULL, NULL, &error);
  if (verif_shared_files_table != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s", error);
  }
  int pid;
  i = 0;
  printf("[CONSOLE]: Serverul a pornit. Se asteapta conexiuni...\n");
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[CONSOLE]:  Eroare la socket().\n");
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
    perror("[CONSOLE]:  Eroare la bind().\n");
    return errno;
  }
  if (listen(sd, 2) == -1)
  {
    perror("[CONSOLE]:  Eroare la listen().\n");
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
      perror("[CONSOLE]:  Eroare la accept().\n");
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
  int logged_in = 0;
  while (1)
  {
    printf("[CONSOLE]:  Asteptam mesajul de la clientul %s:%d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    fflush(stdout);
    while (logged_in != 1)
    {
      logged_in = logare((struct thData *)arg);
    }
    int ok = raspunde((struct thData *)arg);
    if (ok == 0)
      break;
    /* am terminat cu acest client, inchidem conexiunea */
  }
  close((intptr_t)arg);
  return (NULL);
};
int logare(void *arg)
{
  char msg[100];

  struct thData tdL;
  tdL = *((struct thData *)arg);

  if (read(tdL.cl, msg, 100) <= 0)
  {
    perror("[CONSOLE]:  Eroare la read() de la client.\n");
  }
  if (strstr(msg, "?login") || strstr(msg, "?register"))
  {

    printf("[CONSOLE]:  %s:%d -> %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), msg);
    //printf("[CONSOLE]:  Trimitem inapoi mesajul la clientul %s:%d... %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), nr);
    if (OK == 0)
    {
      OK = login(msg, arg);
      //printf("%d\n", OK);
      if (OK == 1)
      {
        if (write(tdL.cl, "Conectat la server cu succes.\n\nBun venit.\n\n-> ?share [filename] [parent_dir] [extension]\n-> ?download [filename] [extension]\n-> ?connect [port]\n-> ?exit", 300) <= 0)
        {
          perror("[CONSOLE]:  Eroare la write() catre client.\n");
        }
        return 1;
      }
      return 0;
    }
  }else{
    if (write(tdL.cl, "puteti folosi functionalitatile programului doar dupa ce va logati.", 100) <= 0)
        {
          perror("[CONSOLE]:  Eroare la write() catre client.\n");
        }
        return 0;
  }
}
int raspunde(void *arg)
{
  char msg[100];

  struct thData tdL;
  tdL = *((struct thData *)arg);

  if (read(tdL.cl, msg, 100) <= 0)
  {
    perror("[CONSOLE]:  Eroare la read() de la client.\n");
  }
  if (strcmp(msg, "?exit") == 0)
  {
    printf("[CONSOLE]:  deconectat de la %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    return 0;
  }
  else if (strcmp(msg, "?share") == 0)
  {
    char filename[30];
    printf("[CONSOLE]:  Clientul doreste sa partajeze un fisier in retea.\n");
    get_filename_from_user();                                               //ii spune clientului sa introduca numele fisierului pe care doreste sa il partajeze cu reteaua
    upload_to_db(filename, inet_ntoa(from.sin_addr), ntohs(from.sin_port)); //pune in db numele fisierului, alaturi de adresa clientului
    char confirm[100] = "Fisier uploadat cu succes in baza de date.\n";
    if (write(tdL.cl, confirm, 100) <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
  }
  else if (strcmp(msg, "?download") == 0)
  {
    char filename[30];
    char client_target[100];
    char *path;
    char adress[100];
    printf("[CONSOLE]:  Clientul doreste sa descarce un fisier din retea.\n");
    get_filename_from_user();         //ii spune clientului sa introduca numele fisierului pe care doreste sa il descarce
    call_db(filename, client_target); //serverul cauta in baza de date clientul care are acel fisier si obtine adresa clientului
    send_path_adress(path, adress);   //ii trimite clientului initial adresa clientului
     if (write(tdL.cl, "adresa clientului: ", 100) <= 0)
  {
    perror("[CONSOLE]:  Eroare la write() catre client.\n");
  }
  }else{
    if (write(tdL.cl, "Comanda necunoscuta.", 100) <= 0)
  {
    perror("[CONSOLE]:  Eroare la write() catre client.\n");
  }
  }
}
