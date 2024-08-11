#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "structs.h"
#define MAX 256

#define MSGTOSRV 1

void appendItem(db_item itemToAppend)
{
    FILE *dbfile;
    char *filename = "db.txt";
    dbfile = fopen(filename, "a");
    if (dbfile == NULL)
    {
        printf("error opening the db file for appending \n");
        exit(1);
    }
    printf("The entry to append: %s,%s,%.2f \n", itemToAppend.acc_num, itemToAppend.pin, itemToAppend.funds);
    fprintf(dbfile, "%s,%s,%.2f\n", itemToAppend.acc_num, itemToAppend.pin, itemToAppend.funds);
    fclose(dbfile);
}

db_item getItem(char *acc)
{
    int found = 0;
    FILE *dbfile;
    char str[100];
    char *filename = "db.txt";
    dbfile = fopen(filename, "r");
    if (dbfile == NULL)
    {
        printf("error opening the db file for reading \n");
        exit(1);
    }

    db_item temp_item;
    while (fgets(str, 100, dbfile) != NULL)
    {
        char *token = strtok(str, ",");
        if (strcmp(acc, token) == 0)
        {
            strcpy(temp_item.acc_num, token);
            token = strtok(NULL, ",");
            strcpy(temp_item.pin, token);
            token = strtok(NULL, ",");
            temp_item.funds = atof(token);
            found = 1;
            break;
        }
    }

    if (found != 1)
    {
        strcpy(temp_item.acc_num, "0");
    }

    return temp_item;
}

void replaceItem(db_item itemToReplace, char flag)
{
    FILE *dbfile;
    FILE *dbfile2;
    char str[MAX];
    char str2[MAX];
    char *filename = "db.txt";
    char *tempname = "temp.txt";
    dbfile = fopen(filename, "r");
    if (dbfile == NULL)
    {
        printf("error opening the db file for reading \n");
        exit(1);
    }

    dbfile2 = fopen(tempname, "w");
    if (dbfile2 == NULL)
    {
        printf("error opening the db file for writing \n");
        exit(1);
    }

    int check = 0;
    while (!feof(dbfile))
    {
        strcpy(str, "\0");
        strcpy(str2, "\0");
        fgets(str, MAX, dbfile);
        if (strcmp(str, "\0") == 0)
        {
            break;
        }
        str[strcspn(str, "\n")] = 0;
        strcpy(str2, str);

        char *token = strtok(str, ",");
        if (strcmp(token, itemToReplace.acc_num) == 0)
        {
            if (check != 0)
            {
                fprintf(dbfile2, "\n");
                check++;
            }

            if (flag == 'B')
            {
                itemToReplace.acc_num[0] = 'X';
            }

            fprintf(dbfile2, "%s,%s,%.2f\n", itemToReplace.acc_num, itemToReplace.pin, itemToReplace.funds);
        }
        else
        {
            fprintf(dbfile2, "%s\n", str2);
        }
    }

    fclose(dbfile);
    fclose(dbfile2);
    remove(filename);
    rename(tempname, filename);
}

#define SEM_MODE 0644

int main()
{
    if (msgctl(msgget((key_t)1119, 0666 | IPC_CREAT), IPC_RMID, 0) == -1)
    {
        printf("error closing the msg queue \n");
        exit(EXIT_FAILURE);
    }

    int msqid = msgget((key_t)1119, 0666 | IPC_CREAT);
    key_t key_S = ftok("shm1B", 1);
    int semid = semget(key_S, 1, IPC_CREAT | SEM_MODE);
    if (semid == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n", semid);

    key_t key_S2 = ftok("shm2B", 2);
    int semid2 = semget(key_S2, 1, IPC_CREAT | SEM_MODE);
    if (semid2 == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n", semid2);

    key_t key_S3 = ftok("shm3B", 3);
    int semid3 = semget(key_S3, 1, IPC_CREAT | SEM_MODE);
    if (semid3 == -1)
    {
        perror("Error in semaphore creation");
        exit(1);
    }
    printf("Semaphore key = %d\n", semid3);

    union semun sem_init;

    sem_init.val = 1;
    // if(semctl(semid,0,SETVAL,sem_init) == -1)
    // {
    //     perror("Error in semaphore initialization");
    //     exit(1);
    // }

    // SHOWING VALUE OF SEMAPHPRE
    // int semVal = semctl(semid,0,GETVAL,0);
    // printf("Initial Semaphore value is = %d\n",semVal);

    // Doing operations on semaphore
    struct sembuf sem;

    // // ACQUIRE RESOURCE
    // sem.sem_num =0;
    // sem.sem_flg = SEM_UNDO;
    // sem.sem_op = -1;
    // semop(semid,&sem,1);
    // printf("Semaphore acquired\n");

    // // VIEW VALUE OF THE SEMAPHORE
    // semVal = semctl(semid,0,GETVAL,0);
    // printf("Semaphore value is = %d\n",semVal);

    // // RELEASE RESOURCE
    // sem.sem_num =0;
    // sem.sem_flg = SEM_UNDO;
    // sem.sem_op = 1;
    // semop(semid,&sem,1);
    // printf("Semaphore releaseed\n");

    // semVal = semctl(semid,0,GETVAL,0);
    // printf("Semaphore value is = %d\n",semVal);

    db_item db_acc;
    db_item current_acc;
    message current_msg;

    while (1)
    {
        printf("=============================\n");
        printf("=                           =\n");
        printf("=          DB SERVER        =\n");
        printf("=                           =\n");
        printf("=============================\n");
        if (msgrcv(msqid, &current_msg, sizeof(message), 1, 0) == -1)
        {
            perror("msgrcv: msgrcv failed");
            exit(1);
        }
        else
        {
            current_acc = current_msg.contents;
            if (current_msg.msg_type == LOGIN)
            {
                printf("DB_SERVER Checking Details for --> %s\n", current_acc.acc_num);

                // ACQUIRE RESOURCE
                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = -1;
                semop(semid, &sem, 1);
                printf("Semaphore acquired\n");

                db_acc = getItem(current_acc.acc_num);

                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = 1;
                semop(semid, &sem, 1);
                printf("Semaphore releaseed\n");

                if (strcmp(db_acc.acc_num, "0") == 0)
                {
                    current_msg.msg_type = PIN_WRONG;
                    current_msg.message_type = 1;
                    printf("DB SERVER SAYS CREDENTIALS WRONG\n");
                    if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                    {
                        perror("msgsnd: msgsnd failed");
                        exit(1);
                    }
                }
                else
                {
                    if (atoi(current_acc.pin) - 1 == atoi(db_acc.pin))
                    {
                        current_msg.msg_type = OK;
                        current_msg.message_type = 1;
                        if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                        {
                            perror("msgsnd: msgsnd failed");
                            exit(1);
                        }
                    }
                    else
                    {
                        current_msg.msg_type = PIN_WRONG;
                        current_msg.message_type = 1;
                        if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                        {
                            perror("msgsnd: msgsnd failed");
                            exit(1);
                        }
                    }
                }
            }
            else if (current_msg.msg_type == BLOCKED)
            {
                current_acc = current_msg.contents;

                // ACQUIRE RESOURCE
                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = -1;
                semop(semid, &sem, 1);
                printf("Semaphore acquired\n");

                replaceItem(current_acc, 'B');
                // RELEASE RESOURCE
                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = 1;
                semop(semid, &sem, 1);
                printf("Semaphore releaseed\n");
            }
            else if (current_msg.msg_type == BALANCE)
            {
                current_msg.contents.funds = db_acc.funds;
                current_msg.message_type = 1;
                if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                {
                    perror("msgsnd: msgsnd failed");
                    exit(1);
                }
            }
            else if (current_msg.msg_type == WITHDRAW)
            {
                float new_Balance = db_acc.funds - current_msg.contents.funds;
                current_acc.funds = new_Balance;
                printf("Amount to withdraw %f\n", current_msg.contents.funds);

                if (new_Balance < 0)
                {
                    current_msg.msg_type = NSF;
                }
                else
                {
                    current_acc.funds = new_Balance;
                    current_msg.msg_type = FUNDS_OK;
                    int temp_encode = atoi(current_acc.pin) - 1;
                    char temp[4];
                    sprintf(temp, "%d", temp_encode);
                    strcpy(current_acc.pin, temp);
                    replaceItem(current_acc, 0);
                    current_msg.contents.funds = new_Balance;
                }
                if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                {
                    perror("msgsnd: msgsnd failed");
                    exit(1);
                }
            }
            else if (current_msg.msg_type == UPDATE_DB)
            {
                db_acc = getItem(current_acc.acc_num);

                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = -1;
                semop(semid, &sem, 1);
                printf("Semaphore acquired\n");

                db_acc = getItem(current_acc.acc_num);

                sem.sem_num = 0;
                sem.sem_flg = SEM_UNDO;
                sem.sem_op = 1;
                semop(semid, &sem, 1);
                printf("Semaphore releaseed\n");

                if (strcmp(db_acc.acc_num, "0") == 0)
                {
                    printf("Adding this account\n");
                    int temp_encode = atoi(current_acc.pin) - 1;
                    char temp[4];
                    sprintf(temp, "%d", temp_encode);
                    strcpy(current_acc.pin, temp);

                    // ACQUIRE RESOURCE
                    sem.sem_num = 0;
                    sem.sem_flg = SEM_UNDO;
                    sem.sem_op = -1;
                    semop(semid, &sem, 1);
                    // printf("Semaphore acquired\n");

                    appendItem(current_acc);

                    sem.sem_num = 0;
                    sem.sem_flg = SEM_UNDO;
                    sem.sem_op = 1;
                    semop(semid, &sem, 1);
                    // printf("Semaphore releaseed\n");

                    current_msg.msg_type = SUCCESS;
                }
                else
                {
                    current_msg.msg_type = FAILURE;
                }
                if (msgsnd(msqid, &current_msg, sizeof(message), 0) == -1)
                {
                    perror("msgsnd: msgsnd failed");
                    exit(1);
                }
            }
        }
        // system("clear");
    }

    return 0;
}
