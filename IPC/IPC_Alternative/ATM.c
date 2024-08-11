
#include "Banking.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>

int LOGIN_COUNT = 0;
time_t rawtime;
struct tm *timeinfo;
void ExecuteOperation(struct my_msgbuf, int, char *);
void Login(struct my_msgbuf serverBuffer, int server_ID);
void Enter_Shell(int server_ID, struct my_msgbuf serverBuffer);
void sendMessage(struct my_msgbuf serverBuffer, int server_ID);

int main(int argc, const char *argv[])
{
    key_t server_key;
    int server_ID;
    struct my_msgbuf serverBuffer;

    /* Provide a key to use the ATM connected with Server */
    if ((server_key = ftok("ATM.c", 'Q')) == -1)
    {
        perror("ftok");
        exit(1);
    }

    /* Get an instance of the message queue */
    if ((server_ID = msgget(server_key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }
    printf("---==========================---\n    Welcome to the ATM Shell.\n---==========================---\n");
    serverBuffer.mtype = 1;

    while (1)
    {
        Login(serverBuffer, server_ID);       // Only when we log in, can we enter the shell
        Enter_Shell(server_ID, serverBuffer); // And only when they log out, can we leave the shell
        LOGIN_COUNT = 0;
    }

    if ((msgctl(server_ID, IPC_RMID, NULL)) == -1)
    {
        perror("msgctl");
        exit(1);
    }
    printf("Message queue was destroyed");
    return 0;
}

/**
 * Description: Infinite shell
 */
void Enter_Shell(int server_ID, struct my_msgbuf serverBuffer)
{
    // Unblock the user from logging OUT, which refreshes the login count

    printf("\n\n\nUser Logged in at %s \n\n", asctime(timeinfo));
    printf("To Withdraw Money type 'Withdraw X' where X is the amount of money\n");
    printf("To Deposit Money type 'Deposit Y' where Y is the amount of money\n");
    printf("To see the balance, do 'Request Funds' \n");

    char input[200];
    while (LOGIN_COUNT < 3)
    {
        fgets(input, sizeof(input), stdin);
        int len = strlen(input);
        if (input[len - 1] == '\n')
            input[len - 1] = '\0';
        ExecuteOperation(serverBuffer, server_ID, input);
    }
}

/**
 * Description: Take in an operation and execute it
 */
void ExecuteOperation(struct my_msgbuf serverBuffer, int server_ID, char *inString)
{
    char copy[200];
    strcpy(copy, inString);
    char *token = strtok(inString, " ");
    int count_fields = 0;
    float value;
    char op[10];
    while (token)
    {
        /* If it is a Withdrawel operation, then process it */
        if (strcmp(token, "Withdraw") == 0)
        {
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(serverBuffer.messagetext, copy);
            sendMessage(serverBuffer, server_ID);
            LOGIN_COUNT = 3;
            printf("Logging out\n");
            return;
        }
        /* If it is a Deposit operation, then process it */
        else if (strcmp(token, "Deposit") == 0)
        {
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(serverBuffer.messagetext, copy);
            sendMessage(serverBuffer, server_ID);
            LOGIN_COUNT = 3;
            printf("Logging out\n");
            return;
        }
        /* If it is a Deposit operation, then process it */
        else if (strcmp(token, "Request") == 0)
        {
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(serverBuffer.messagetext, copy);
            sendMessage(serverBuffer, server_ID);
            LOGIN_COUNT = 3;
            printf("Logging out\n");
            return;
        }
        else
        {
            printf("Message was not sent\n Please check your syntax and try again\n");
            return;
        }
        token = strtok(NULL, " ");
        count_fields++;
    }
}
/**
 * Send a message to the server with a result
 */
void sendMessage(struct my_msgbuf serverBuffer, int server_ID)
{
    int len = strlen(serverBuffer.messagetext);

    if (msgsnd(server_ID, &serverBuffer, len + 1, 0) == -1)
    {
        perror("msgsnd");
    } // if (serverBuffer.messagetext[len-1] == '\n') serverBuffer.messagetext[len-1] = '\0';
    else
    {
        if (msgrcv(server_ID, &serverBuffer, sizeof(serverBuffer.messagetext), 0, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        else
        {
            printf("%s\n", serverBuffer.messagetext);
            return;
        }
    }
}

void Login(struct my_msgbuf serverBuffer, int server_ID)
{
    while (1)
    {
        // Input Dummy Data Structure
        char input[200];
        int len = strlen(input);
        memset(serverBuffer.messagetext, '\0', sizeof(serverBuffer.messagetext));
        memset(input, '\0', sizeof(input));

        if (LOGIN_COUNT >= 3)
        {
            printf("Your account was blocked at %s due to 3 Bad Login Attempts\n", asctime(timeinfo));
            printf("Please contact your local administrator to get access to your account\n");
        }
        else
        {
            printf("Please Log in:\nEnter Your 5 Digit Account Number\n(Login Attempts that ATM does not send do not "
                   "lower your Login Attempt tries)\n");
        }
        fgets(input, sizeof(input), stdin);
        if (LOGIN_COUNT >= 3)
        {
            continue;
        }
        if (serverBuffer.messagetext[len - 1] == '\n')
        {
            serverBuffer.messagetext[len - 1] = '\0';
        }

        len = strlen(input) - 1; // for null terminating
        // get rid of newline
        if (len < 5 || len > 5)
        {
            printf("Account Number Needs to be 5 (five) digits. ATM attempts not send\n");
            printf("Login Attempts Left=%i\n\n", 3 - LOGIN_COUNT);
            continue;
        }

        if (input[strlen(input) - 1] == '\n')
        {
            input[strlen(input) - 1] = '\0';
        }

        // Copy it to messagetext
        strcat(serverBuffer.messagetext, input);
        strcat(serverBuffer.messagetext, " ");

        // get the next value from the user & rid the messageText
        printf("Input your 3 digit PIN\n");
        fgets(input, sizeof(input), stdin);
        if (input[strlen(input) - 1] == '\n')
        {
            input[strlen(input) - 1] = '\0';
        }

        len = strlen(input); // for null terminating
        // get rid of newline
        if (len < 3 || len > 3)
        {
            printf("Account PIN Needs to be 3 (three) digits. ATM attempts not send. Try Again\n");
            printf("Login Attempts Left=%i\n\n", 3 - LOGIN_COUNT);
            continue;
        }

        // Append that to the end
        strcat(serverBuffer.messagetext, input);
        len = strlen(serverBuffer.messagetext);
        printf("You have %i Login Attempts left\n", 2 - LOGIN_COUNT);
        printf("ATM sending = %s\n---Processing....---\n", serverBuffer.messagetext);

        if (msgsnd(server_ID, &serverBuffer, len + 1, 0) == -1)
        {
            perror("msgsnd");
        } // if (serverBuffer.messagetext[len-1] == '\n') serverBuffer.messagetext[len-1] = '\0';
        else
        {
            if (msgrcv(server_ID, &serverBuffer, sizeof(serverBuffer.messagetext), 0, 0) == -1)
            {
                perror("msgrcv");
                exit(1);
            }
            else
            {
                printf("ATM Result = %s ->", serverBuffer.messagetext);
                if (strcmp(serverBuffer.messagetext, "NOT OK") == 0)
                {
                    LOGIN_COUNT++;
                    printf(" INVALID LOGIN ATTEMPT\n--------------\n");
                    if (LOGIN_COUNT == 3)
                    {
                        // Grab the time for reference. Not necessary though
                        time(&rawtime);
                        timeinfo = localtime(&rawtime);
                    }
                }
                else if (strcmp(serverBuffer.messagetext, "OK") == 0)
                {
                    // Grab the time and enter the shell, we are in.
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    printf(" VALID LOGIN\n--------------\n");
                    return;
                    printf(" We are in\n");
                }
            }
        }
    }
}
