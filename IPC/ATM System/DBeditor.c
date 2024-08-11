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

#define SEM_MODE 0644
#define MSGTOSRV 1

int main()
{
    int msqid = msgget((key_t)1119, 1024 | 0600);
    if (msqid == -1)
    {
        perror("msgget: msgget failed");
        exit(1);
    }

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

    // sem_init.val = 1;
    //  if(semctl(semid,0,SETVAL,sem_init) == -1)
    //  {
    //      perror("Error in semaphore initialization");
    //      exit(1);
    //  }

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

    while (1)
    {
        printf("=============================\n");
        printf("=                           =\n");
        printf("=          DB EDITOR        =\n");
        printf("=                           =\n");
        printf("=============================\n");
        char ch;

        printf("Do you want to add aaccount :<Y/N>\n");
        scanf("%c", &ch);
        if (ch == 'Y' || ch == 'y')
        {
            db_item db_acc;
            db_item current_acc;
            message msg;
            char tempFunds[100];
            printf("Enter Account Number: ");
            scanf("%s", current_acc.acc_num);
            current_acc.acc_num[strcspn(current_acc.acc_num, "\n")] = 0;

            if (strlen(current_acc.acc_num) != 5)
            {
                while (strlen(current_acc.acc_num) != 5)
                {
                    printf("Enter 5 digit account Number\n");
                    scanf("%s", current_acc.acc_num);
                }
            }

            printf("Enter desired pin: ");
            scanf("%s", current_acc.pin);
            if (strlen(current_acc.pin) != 3)
            {
                while (strlen(current_acc.pin) != 3)
                {
                    printf("Enter 3 digit pin\n");
                    scanf("%s", current_acc.pin);
                }
            }
            current_acc.pin[strcspn(current_acc.pin, "\n")] = 0;

            printf("Enter Available Funds: ");
            scanf("%s", tempFunds);
            tempFunds[strcspn(tempFunds, "\n")] = 0;

            current_acc.funds = (float)strtod(tempFunds, NULL);

            msg.message_type = 1;
            msg.msg_type = UPDATE_DB;
            msg.contents = current_acc;

            if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
            {
                perror("msgsnd: msgsnd failed");
                exit(1);
            }
            else
            {
                printf("Account details Sent to DB Server\n");
            }

            if (msgrcv(msqid, &msg, sizeof(message), 1, 0) == -1)
            {
                perror("msgrcv: msgrcv failed");
                exit(1);
            }

            if (msg.msg_type == SUCCESS)
            {
                printf("Account %s added Successfully\n", msg.contents.acc_num);
            }
            else if (msg.msg_type == FAILURE)
            {
                printf("Account %s already Exist\n", msg.contents.acc_num);
            }
            getchar();
        }
    }
    // detach from shared memory
    //  shmdt(str);
    return 0;
}
