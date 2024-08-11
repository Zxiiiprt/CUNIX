/*
**DBserver is the broker between the editor raw data and the ATM Machine
Note : the editor will always take precedence in any operation
Because the user is disposable and the server always need to function independently

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

struct my_msgbuf
{
    long mtype;
    char messagetext[200];
};

// void ParseString(char*);
int main(void)
{
    key_t user_key;
    key_t editor_key;
    int ATM_ID;
    int EDITOR_ID;
    struct my_msgbuf userBuffer;
    struct my_msgbuf editorBuffer;

    /* Configure the MSGQueue for the ATM Machine */
    // This is an outgoing message IPC
    if ((editor_key = ftok("DBserver.c", 'E')) == -1)
    {
        perror("ftok");
        exit(1);
    }

    // For the editor communication IPC ' E' for EDITOR
    if ((EDITOR_ID = msgget(editor_key, 0644 | IPC_CREAT)) == -1)
    {
        printf("Editor msgget\n");
        perror("msgget");
        exit(1);
    }

    /* Configure the MSQueue for the Database Editor */
    if ((user_key = ftok("ATM.c", 'Q')) == -1)
    {
        perror("ftok");
        exit(1);
    }

    // 	/* Get an instance of the user message queue */
    if ((ATM_ID = msgget(user_key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }
    printf("Server is connected to Database and waiting for User Input\n==\n");
    editorBuffer.mtype = 1; /* we don't really care in this case */

    for (;;)
    {
        memset(userBuffer.messagetext, '\0', sizeof(userBuffer.messagetext));
        memset(editorBuffer.messagetext, '\0', sizeof(editorBuffer.messagetext));
        if (msgrcv(ATM_ID, &userBuffer, sizeof(userBuffer.messagetext), 0, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        /**
         * If the message was passed in, this is where the sanitation will go
         * To Fulfill the functional requirements of the assignment
         */
        // handle_message()
        else
        {
            // int len = strlen(editorBuffer.messagetext);
            int len = strlen(userBuffer.messagetext);
            sprintf(editorBuffer.messagetext, userBuffer.messagetext);
            if (editorBuffer.messagetext[len - 1] == '\n')
            {
                editorBuffer.messagetext[len - 1] = '\0';
            }
            printf("Value Received from ATM: %s\nSending to Editor\n", editorBuffer.messagetext);
            // ParseString(editorBuffer.messagetext);

            // try and send it to the editor
            if (msgsnd(EDITOR_ID, &editorBuffer, len + 1, 0) == -1)
            {
                printf("Message was not sent\n");
                perror("msgsnd");
            }
            else
            {
                // printf("Editor Received the Message = %s\n",editorBuffer.messagetext);
                // Ok so this is the interesting part. We will wait for a reply from the DB editor to give back to the
                // user
                len = strlen(editorBuffer.messagetext);
                memset(editorBuffer.messagetext, '\0', sizeof(editorBuffer));
                // It wants to get something back from the received
                if (msgrcv(EDITOR_ID, &editorBuffer, sizeof(editorBuffer.messagetext), 0, 0) == -1)
                {
                    perror("msgrcv");
                    exit(1);
                }
                // And give that message back to the user. Take action as necessary
                else
                {
                    printf("Login = %s\n", editorBuffer.messagetext);
                    // try and send it to the user
                    // Could I just add this element to their queue, or would that screw something up?
                    // i.e. send the editor buffer
                    //  (msgsnd(ATM_ID, &editorBuffer,len+1, 0) == -1)
                    //  De-comment below to send back to the user
                    strcpy(userBuffer.messagetext, editorBuffer.messagetext);
                    len = strlen(editorBuffer.messagetext);
                    if (msgsnd(ATM_ID, &userBuffer, len + 1, 0) == -1)
                    {
                        printf("Message was not sent\n\n");
                        perror("msgsnd");
                    }
                    else
                    {
                        printf("Sending back to the ATM\n\n======");
                    }
                }
            }
        }
        // printf("ATM SAYS: \"%s\"\n", userBuffer.messagetext);
    }

    // Clean up and remove the IDs
    if ((msgctl(ATM_ID, IPC_RMID, NULL)) == -1)
    {
        perror("msgctl");
        exit(1);
    }

    if ((msgctl(EDITOR_ID, IPC_RMID, NULL)) == -1)
    {
        perror("msgctl");
        exit(1);
    }
    printf("Message queue was destroyed");

    return 0;
}
