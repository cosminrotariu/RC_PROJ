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

int main(){
    sqlite3_open("myDb.db", &db);
  int verif_login_table = sqlite3_exec(db, "create table if not exists login(user varchar(50), pass varchar(50));", NULL, NULL, &error);
  if (verif_login_table != SQLITE_OK)
  {
    printf("[s] eroare: %s", error);
  }
  int verif_shared_files_table = sqlite3_exec(db, "create table if not exists files(port int, filename varchar(50), dir_parent varchar(50), extension varchar(10));", NULL, NULL, &error);
  if (verif_shared_files_table != SQLITE_OK)
  {
    printf("[s] eroare: %s", error);
  }

  char username[]="andrei";
  char password[]="888Jojo888";

  char insert[] = "insert into login VALUES(\"";
        strcpy(insert + strlen(insert), username);
        strcpy(insert + strlen(insert), "\",\"");
        strcpy(insert + strlen(insert), password);
        strcpy(insert + strlen(insert), "\");");
        printf("%s\n", insert);
        int verif_insert = sqlite3_exec(db, insert, NULL, NULL, &error);
        if (verif_insert != SQLITE_OK)
        {
          printf("[s] eroare: %s\n", error);
        }
        printf("%s\n",insert);
}