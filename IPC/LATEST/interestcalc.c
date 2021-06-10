#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <semaphore.h>

#include "structs.h"

#define SEM_MODE 0644
#define MSGTOSRV 1

int main()
{
    int msqid = msgget((key_t)1119, 1024| 0600);
    if (msqid == -1)
    {
        perror("msgget: msgget failed");
        exit(1);
    }
    key_t key_S = ftok("shm1B",1);
    int semid = semget(key_S,1,IPC_CREAT | SEM_MODE);
    if(semid == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n",semid);
    key_t key_S2 = ftok("shm2B",2);
    int semid2 = semget(key_S2,1,IPC_CREAT | SEM_MODE);
    if(semid2 == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n",semid2);
    key_t key_S3 = ftok("shm3B",3);
    int semid3 = semget(key_S3,1,IPC_CREAT | SEM_MODE);
    if(semid3 == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n",semid3);

    char str[100];
    FILE *dbfile;
    FILE *dbfile2;
    char *filename = "db.txt";
    char *tempname = "temp.txt";
    struct sembuf sem;
    while(1)
    {
        printf("=============================\n");
        printf("=                           =\n");
        printf("=        INTEREST CALC      =\n");
        printf("=                           =\n");
        printf("=============================\n");
            
        sleep(2);
        // ACQUIRE RESOURCE
        sem.sem_num =0;
        sem.sem_flg = SEM_UNDO;
        sem.sem_op = -1;
        semop(semid,&sem,1);
        printf("Semaphore acquired\n");


        dbfile = fopen(filename, "r");
        if(dbfile == NULL)
        {
            printf("error opening the db file for reading \n");
            exit(1);
        }
        dbfile2 = fopen(tempname, "w");
        if(dbfile2 == NULL){
            printf("error opening the db file for writing \n");
            exit(1);
        }

        db_item temp_item;
        while(fgets(str, 100, dbfile) != NULL)
        {
            if(strcmp(str,"\0")==0)
            {
                printf("ERROR\n");
            }
            char *token = strtok(str, ",");
            strcpy(temp_item.acc_num, token);
            token = strtok(NULL, ",");
            strcpy(temp_item.pin, token);
            token = strtok(NULL, ",");
            temp_item.funds = atof(token);
            
            if(temp_item.funds < 0)
            {
                temp_item.funds *= 0.98;
            }
            else 
            {
                temp_item.funds *= 1.01;
            }

            fprintf(dbfile2, "%s,%s,%.2f\n", temp_item.acc_num, temp_item.pin, temp_item.funds);
        }
        size_t size = ftell(dbfile2);
        int fd = open(tempname,O_RDWR);
        ftruncate(fd, size - 1);
        fclose(dbfile);
        fclose(dbfile2);
        remove(filename);
        rename(tempname, filename);

        // RELEASE RESOURCE
        sem.sem_num =0;
        sem.sem_flg = SEM_UNDO;
        sem.sem_op = 1;
        semop(semid,&sem,1);
        printf("Semaphore releaseed\n");

    }
}