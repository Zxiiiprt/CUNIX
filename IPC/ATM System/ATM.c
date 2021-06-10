#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include<semaphore.h>

#include "structs.h"

#define MSGTOSRV 1

#define SEM_MODE 0644



int main(int argc,char * argv[])
{
    char ch;
    int msqid = msgget((key_t)1119, 0666 | 1024);
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
    
    
    
    
    
    union semun sem_init;

    sem_init.val = 1;
    if(semctl(semid,0,SETVAL,sem_init) == -1)
    {
        perror("Error in semaphore initialization");
        exit(1);
    }
    
    struct sembuf sem;

	{   
        while(1)
        {
            
            printf("=============================\n");
            printf("=                           =\n");
            printf("=        ATM COMPONENT      =\n");
            printf("=                           =\n");
            printf("=============================\n");
            
            int wrong_pin_count = 0;
            int login_flag = 0;
            
            
            char * account_Number = (char *) malloc (10 * sizeof(char));
            char * pin = (char *) malloc (10 * sizeof(char));
            message msg;
            while(login_flag == 0 && wrong_pin_count < 3)
            {
                printf("ATM COMPONENT Input an account number\n");
                
                scanf("%s",account_Number);
                
                if(account_Number[0] == 'X' || account_Number[0] == 'x')
                {
                    printf("Terminating ATM process\n");
                    execlp("./kill.sh","kill.sh",NULL);
                }
                
                if(strlen(account_Number) != 5)
                {
                    while(strlen(account_Number) != 5)
                    {
                        printf("Enter 5 digit account Number\n");
                        scanf("%s",account_Number);
                    }
                }

                printf("ATM COMPONENT Input PIN\n");
                
                scanf("%s",pin);
                
                if(strlen(pin) != 3)
                {
                    while(strlen(pin) != 3)
                    {
                        printf("Enter 3 digit pin\n");
                        scanf("%s",pin);
                    }
                }


                db_item curr_account;

                strcpy(curr_account.acc_num,account_Number);
                strcpy(curr_account.pin,pin);
                curr_account.funds = 0;

                
                
                msg.contents = curr_account;
                msg.message_type = 1;
                msg.msg_type = LOGIN;
                
                if(msgsnd(msqid, &msg, sizeof(message), IPC_NOWAIT) == -1)
                {
                    perror("msgsnd: msgsnd failed");
                    exit(1);
                } 
                else 
                {
                    if(msgrcv(msqid, &msg, sizeof(message), 1, 0) == -1)
                    {
                        perror("msgrcv: msgrcv failed");
                        exit(1);
                    } 
                    else 
                    {
                        if(msg.msg_type == PIN_WRONG)
                        {   
                            printf("ATM COMPONENT Re enter Credentials: \n");
                            wrong_pin_count++;
                        }
                        else if(msg.msg_type == OK) 
                        {
                            login_flag = 1;
                        }
                    }
                }
                if(wrong_pin_count == 3)
                {
                    printf("ATM COMPONENT Account: %s is blocked\n",account_Number);
                    wrong_pin_count = 0;
                    
                    msg.contents = curr_account;
                    msg.message_type = 1;
                    msg.msg_type = BLOCKED;
                    if(msgsnd(msqid, &msg, sizeof(message), IPC_NOWAIT) == -1)
                    {
                        perror("msgsnd: msgsnd failed");
                        exit(1);
                    } 
                }
                else if(login_flag == 1)
                {
                    int choice;
                    printf("ATM COMPONENT What do you want to do?   \n");
                    printf("ATM COMPONENT Input 1: Check balance    \n");
                    printf("ATM COMPONENT Input 2: Withdraw         \n");
                    printf("ATM COMPONENT Input X: Exit Transaction \n");
                    scanf("%d",&choice);
                    
                    if(choice == 1)
                    {
                	 msg.contents = curr_account;
                	 msg.message_type = 1;
                	 msg.msg_type = LOGIN;
                	 msgsnd(msqid, &msg, sizeof(message), IPC_NOWAIT);
                	 msgrcv(msqid, &msg, sizeof(message), 1, 0);
                        msg.msg_type = BALANCE;
                        msg.message_type = 1;
                        if(msgsnd(msqid, &msg, sizeof(message), IPC_NOWAIT) == -1)
                        {
                            perror("msgsnd: msgsnd failed");
                            exit(1);
                        } 
                        else 
                        {
                            if(msgrcv(msqid, &msg, sizeof(message), 1, 0) == -1)
                            {
                                perror("msgrcv: msgrcv failed");
                                exit(1);
                            } 
                            else 
                            {
                                printf("ATM COMPONENT Your Balance: %f\n",msg.contents.funds);
                            }
                        }
                    }
                    else if(choice == 2)
                    {
                        printf("ATM COMPONENT Input amount to withdraw: ");
                        scanf("%f",&msg.contents.funds);
                        while(msg.contents.funds <= 0)
                        {
                        	printf("Invalid number Try again\n");
                        	scanf("%f",&msg.contents.funds);
                        }
                        msg.msg_type = WITHDRAW;
                        if(msgsnd(msqid, &msg, sizeof(message), IPC_NOWAIT) == -1)
                        {
                            perror("msgsnd: msgsnd failed");
                            exit(1);
                        } 
                        else 
                        {
                            if(msgrcv(msqid, &msg, sizeof(message), 1, 0) == -1)
                            {
                                perror("msgrcv: msgrcv failed");
                                exit(1);
                            } 
                            else 
                            {
                                if(msg.msg_type == NSF)
                                    printf("ATM COMPONENT : NOT ENOUGH FUNDS AVAILABLE\n");
                                else if(msg.msg_type == FUNDS_OK)
                                    printf("ATM COMPONENT : NEW BALANCE: %f\n",msg.contents.funds);
                            }
                        }
                    }
                    else 
                    {
                        ;    
                    }
                }
                
            }
            //system("clear");
        }
    
    }
    //shmdt(str);
    return 0;
}
