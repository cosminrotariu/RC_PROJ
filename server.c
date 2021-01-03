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
#include <sys/wait.h>
#include "sqlite/sqlite3.h"
#include "shared.h"
#define PORT 3000

sqlite3 *db;
sqlite3_stmt *stmt;
pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
int k = 0;

int sd; //socket descriptor
char *error;
int i;
extern int errno;

typedef struct thData
{
  // int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl; //descriptorul intors de accept
  int port;
} thData;

typedef struct files
{
  char port[10];
  char filename[50];
} f;
 f file[10];

void *treat(void *arg); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
int logare(void *arg,int portul);
int raspunde(void *arg, int portull);
struct sockaddr_in server; // structura folosita de server
struct sockaddr_in from;

static int write_string(int socket, const char *string)
{
  char buffer[MESSAGE_BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));
  size_t length = strlen(string);
  if (length + 1 > sizeof(buffer))
  {
    abort();
  }
  strcpy(buffer, string);
  return write(socket, buffer, sizeof(buffer));
}
// void *read_stdin(void *arg) //in lucru
// {

//   char buf[5];
//   read(0, &buf, sizeof(buf));
//   printf("Am citit %s\n", buf);
//   if (strstr(buf, "?exit") != NULL)
//   {
//     printf("[CONSOLE]:  Serverul se va opri...\n");
//     exit(0);
//   }
//   return NULL;
// }
const char *call_db(char filename[30])
{
  sqlite3_prepare_v2(db, "select * from files;", -1, &stmt, 0);
  const char *portul, *searched_filename;
  while (sqlite3_step(stmt) != SQLITE_DONE)
  {
    searched_filename = (const char *)sqlite3_column_text(stmt, 2);
    portul = (const char *)sqlite3_column_text(stmt, 1);
    if (strcmp(filename, searched_filename) == 0)
    {
        return portul;
      }
      else
      {
        return 0;
      }
  }
}
void upload_to_db(char filename[30], char adress[30], int port)
{
  char insert[256] = "insert into files VALUES(\"";
  strcpy(insert + strlen(insert), adress);
  strcpy(insert + strlen(insert), "\",\"");
  sprintf(insert + strlen(insert), "%d", port);
  strcpy(insert + strlen(insert), "\",\"");
  strcpy(insert + strlen(insert), filename);
  // strcpy(insert + strlen(insert), "\",\"");
  // strcpy(insert + strlen(insert), parent_dir);
  // strcpy(insert + strlen(insert), "\",\"");
  // strcpy(insert + strlen(insert), extension);
  strcpy(insert + strlen(insert), "\");");
  int verif_insert = sqlite3_exec(db, insert, NULL, NULL, &error);
  if (verif_insert != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s\n", error);
  }
}
int login(char msg[], void *arg)
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
        for (int k = 0; k < i; k++)
        {
          username[k] = credentials[k];
        }
        username[i] = '\0';
        break;
      }
    }
    sqlite3_prepare_v2(db, "select * from login;", -1, &stmt, 0);
    const char *user, *pass;
    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
      user = (const char *)sqlite3_column_text(stmt, 0);
      pass = (const char *)sqlite3_column_text(stmt, 1);
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
          if (write_string(tdL.cl, "parola incorecta") <= 0)
          {
            perror("[CONSOLE]:  Eroare la write() catre client.\n");
          }
          return 0;
        }
      }
    }
    //printf("contul nu exista. va rugam sa va creati un cont\n");
    if (write_string(tdL.cl, "contul nu exista.") <= 0)
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
    const char *user;
    int ok2 = 1;
    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
      user = (const char *)sqlite3_column_text(stmt, 0);
      if (strcmp(username, user) == 0)
      {
        //printf("contul exista deja. va rugam sa alegeti alt user.\n");
        if (write_string(tdL.cl, "contul exista deja.") <= 0)
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
  return 0;
}
int main()
{
  sqlite3_open("myDb.db", &db);
  int verif_login_table = sqlite3_exec(db, "create table if not exists login(user varchar(50), pass varchar(50));", NULL, NULL, &error);
  if (verif_login_table != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s", error);
  }
  int verif_shared_files_table = sqlite3_exec(db, "create table if not exists files(adress varchar(30),port int, filename varchar(30));", NULL, NULL, &error);
  if (verif_shared_files_table != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s", error);
  }
  //int pid;
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

  while (1)
  {
    int client;
    thData *td;
    socklen_t length = sizeof(from);
    fflush(stdout);
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[CONSOLE]:  Eroare la accept().\n");
      continue;
    }
    int cl; //descriptorul intors de accept

    td = (struct thData *)malloc(sizeof(struct thData));
    td->cl = client;
    //td->port = ntohs(from.sin_port);

    pthread_create(&th[i], NULL, &treat, td);
  }
}
int findPort(void *arg){
  struct thData tdL;
  tdL = *((struct thData *)arg);
    int portul;
    if (read(tdL.cl, &portul, sizeof(portul)) <= 0)
    {
      perror("[CONSOLE]:  Eroare la read() de la client.\n");
    }
    tdL.port = portul;
    return tdL.port;
}
void *treat(void *arg)
{
  int portul =0;
  int nr = 0;
  struct thData tdL;
  tdL = *((struct thData *)arg);
  while (1)
  {
    //printf("[CONSOLE]:  Asteptam mesajul de la clientul %s:%d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
    fflush(stdout);
    pthread_detach(pthread_self());
    if(portul==0){
    portul = findPort((struct thData *)arg);
    }
    while (nr==0)
    {
      nr = logare((struct thData *)arg,portul);
    }
    int ok = raspunde((struct thData *)arg, portul);
    if (ok == 0)
      break;
    /* am terminat cu acest client, inchidem conexiunea */
  }
  close((intptr_t)arg);
  return (NULL);
}
int logare(void *arg,int portul)
{

  char msg[MESSAGE_BUFFER_SIZE];

  struct thData tdL;
  tdL = *((struct thData *)arg);
  tdL.port = portul;
  // if (nr == -1)
  // {
  //   int portul;
  //   if (read(tdL.cl, &portul, sizeof(portul)) <= 0)
  //   {
  //     perror("[CONSOLE]:  Eroare la read() de la client.\n");
  //   }
  //   tdL.port = portul;
  // }
  if (read(tdL.cl, &msg, sizeof(msg)) <= 0)
  {
    perror("[CONSOLE]:  Eroare la read() de la client.\n");
  }
  printf("[%s:%d]: `%s`\n", inet_ntoa(from.sin_addr), tdL.port, msg);

  if (strstr(msg, "?login") || strstr(msg, "?register"))
  {

    //printf("[CONSOLE]:  %s:%d -> %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), msg);
    //printf("[CONSOLE]:  Trimitem inapoi mesajul la clientul %s:%d... %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), nr);
    int OK = 0;
    if (OK == 0)
    {
      OK = login(msg, (struct thData *)arg);
      //printf("%d\n", OK);
      if (OK == 1)
      {
        if (write_string(tdL.cl, "Conectat la server cu succes.\n\n"
                                 "Bun venit.\n\n"
                                 "-> ?share [filename]\n"
                                 "-> ?download [port] [filename]\n"
                                 "-> ?list\n"
                                 "-> ?exit") <= 0)
        {
          perror("[CONSOLE]:  Eroare la write() catre client.\n");
        }
        return 1;
      }
      return 0;
    }
  }
  else
  {
    if (write_string(tdL.cl, "puteti folosi functionalitatile programului doar dupa ce va logati.") <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
    return 0;
  }
  return 0;
}
void deleteFilesDB(int port)
{
  char deleteseq[256] = "delete from files where port=";
  sprintf(deleteseq + strlen(deleteseq), "%d", port);
  int verif_insert = sqlite3_exec(db, deleteseq, NULL, NULL, &error);
  if (verif_insert != SQLITE_OK)
  {
    printf("[CONSOLE]:  eroare: %s\n", error);
  }
}
void listFiles(){
 

   sqlite3_prepare_v2(db, "select * from files;", -1, &stmt, 0);
   int i=0;
   while (sqlite3_step(stmt) != SQLITE_DONE){
     strcpy(file[i].filename,(const char*)sqlite3_column_text(stmt,2));
     strcpy(file[i].port,(const char*)sqlite3_column_text(stmt, 1));
     i++;
   }
}
int raspunde(void *arg, int portull)
{
  char msg[MESSAGE_BUFFER_SIZE];
  int okei = 0;
  struct thData tdL;
  tdL = *((struct thData *)arg);
  char filename[30];
  char parent_dir[30];
  char extension[10];
  tdL.port = portull;

  if (read(tdL.cl, &msg, sizeof(msg)) <= 0)
  {
    perror("[CONSOLE]:  Eroare la read() de la client.\n");
  }
  printf("[%s:%d]: `%s`\n", inet_ntoa(from.sin_addr), tdL.port, msg);

  if (strcmp(msg, "?exit") == 0)
  {
    printf("[CONSOLE]:  deconectat de la %s:%d\n", inet_ntoa(from.sin_addr), tdL.port);
    deleteFilesDB(portull);
    return 0;
  }
  else if (strstr(msg, "?share"))
  { //msg =   ?share file.jpg
    okei = 1;

    printf("[CONSOLE]:  Clientul doreste sa partajeze un fisier in retea.\n");

    char *token = strtok(msg, " ");
    token = strtok(NULL, " ");
    strcpy(filename, token);
    // token = strtok(NULL, " ");
    // strcpy(parent_dir, token);
    // token = strtok(NULL, " ");
    // strcpy(extension, token);
    // //printf("filename: %s, parent_dir: %s, extension: %s\n", filename, parent_dir, extension);

    upload_to_db(filename, inet_ntoa(from.sin_addr), tdL.port); //pune in db numele fisierului, alaturi de adresa clientului

    if (write_string(tdL.cl, "Fisier uploadat cu succes in baza de date.\n") <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
    return 1;
  }
  else if (strstr(msg, "?list"))
  {
    okei = 1;

    printf("[CONSOLE]:  Clientul doreste sa afle ce fisiere sunt partajate in retea.\n");
    listFiles();
    char ceva[16 * 1024]="Fisierele partajate in retea alaturi de port-uri sunt:\n";
    int i=0;
    while(1){
      if(strcmp(file[i].filename,"\0")==0){
        break;
      }
      sprintf(ceva+strlen(ceva), "%s <-> %s\n", file[i].filename,file[i].port);
      //printf("FISIER: %s PORT:%s\n",file[i].filename,file[i].port);
    
      i++;
    }
    //printf("%s\n",ceva);
    sprintf(ceva+strlen(ceva),"\nFolositi comanda ?download [port] [filename] pentru a descarca fisierul.");
    if (write_string(tdL.cl, ceva) <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
    return 1;
  }
  // else if (strstr(msg, "?download"))
  // { //?download [port] [filename]
  //   okei = 1;
  //   // printf("[CONSOLE]:  Clientul doreste sa descarce un fisier din retea.\n");
  //   // char *token = strtok(msg, " ");
  //   // char portul[10];
  //   // token = strtok(NULL, " ");
  //   // strcpy(portul,token);
  //   // token=strtok(NULL," ");
  //   // strcpy(filename, token);
  //   // //const char *portul = call_db(filename); //serverul cauta in baza de date clientul care are acel fisier si obtine adresa clientului

  //   // // char send_msg[200] = "Folositi comanda ?do ";
  //   // // strcpy(send_msg + strlen(send_msg), portul);
  //   // // strcpy(send_msg + strlen(send_msg), " pentru a va conecta la client.");
  //   // if (write_string(tdL.cl, send_msg) <= 0)
  //   // {
  //   //   perror("[CONSOLE]:  Eroare la write() catre client.\n");
  //   // }
  //   return 1;
  // }
  if (okei == 0 || strstr(msg, filename) == NULL || strstr(msg, parent_dir) == NULL)
  {
    printf("%s\n", msg);
    if (write_string(tdL.cl, msg) <= 0)
    {
      perror("[CONSOLE]:  Eroare la write() catre client.\n");
    }
  }
  return 0;
}