
#include <errno.h>
#include <math.h>
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

char *ExecuteOperation(struct my_msgbuf, int);
char *Withdraw(char *);
char *Request(char *);
char *Deposit(char *);
void parseInputFile();
int Login(char *);
int test_user_Exists();
int LOGIN_BIT = 0;
int Login_Cookie;
void Enter_Login(struct my_msgbuf receiver, int server_ID);
int user_Exists(int AccountNum, int PIN);

int main(int argc, const char *argv[])
{
    key_t server_key;
    int server_ID;
    struct my_msgbuf receiver;
    // Make an ftok
    // For the editor communication IPC
    if ((server_key = ftok("DBserver.c", 'E')) == -1)
    {
        perror("ftok");
        exit(1);
    }

    /* Get an instance of the ,message queue */
    if ((server_ID = msgget(server_key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    printf("The Database Listener is awake and waiting for Database Input\n=======================\n");
    /* Start the Database Data Miner  AKA DB editor*/
    for (;;)
    {
        if (msgrcv(server_ID, &receiver, sizeof(receiver.messagetext), 0, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        else
        {
            // Process the request. Try and log in

            int login_result = Login(receiver.messagetext);
            int len = strlen(receiver.messagetext);

            printf("We have tried to log in\n");
            // then we send back the reply
            if (login_result > 0)
            {
                sprintf(receiver.messagetext, "OK");
                // Have a global flag here that goes and leaves logon mode
                LOGIN_BIT = 1;
            }
            else if (login_result == 0)
            {
                sprintf(receiver.messagetext, "NOT OK");
            }
            else
            {
                strcpy(receiver.messagetext, "ERROR: NULL STRING");
            }

            /* Send the Result back to the DB server */
            len = strlen(receiver.messagetext); // We meed to reset our length after
            if (msgsnd(server_ID, &receiver, len + 1, 0) == -1)
            {
                printf("Message was not sent\n");
                perror("msgsnd");
            }
            else
            {
                printf("Editor tries to send message back to server\n");
                // Leave Login Mode if User authenticated
                if (LOGIN_BIT > 0)
                {
                    printf("Going in\n");
                    Enter_Login(receiver, server_ID);
                }
                // The message was ok? Then go to the shell
            }
            // Now we pipe back to the other process to reply
        }
        printf("DBserver SAYS: \"%s\"\n", receiver.messagetext);
    }

    if ((msgctl(server_ID, IPC_RMID, NULL)) == -1)
    {
        perror("msgctl");
        exit(1);
    }
    printf("Message queue was destroyed");

    return 0;
}

/************************                   Post Authentication 				 *************************/
/**
* Description: This is is a lot of the meat of the salad. At this point the user is logged in and is interacting with
                the shell dreictly
*@param: receiver = the memory context that we are dealing with
*@param server_ID = the ID of the server that we are dealing with
*/
void Enter_Login(struct my_msgbuf receiver, int server_ID)
{
    for (;;)
    {
        if (msgrcv(server_ID, &receiver, sizeof(receiver.messagetext), 0, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        else
        {
            // Process the request. Try and log in
            printf("DBserver SAYS: \"%s\"\n", receiver.messagetext);
            int len = strlen(receiver.messagetext);
            char *msg;
            msg = ExecuteOperation(receiver, server_ID);
            // Now what we want to do is paste that and send it back to the user
            // just return a char here ideally with their currenty balance and send it back

            strcpy(receiver.messagetext, msg);
            len = strlen(receiver.messagetext); // We meed to reset our length after
            if (msgsnd(server_ID, &receiver, len + 1, 0) == -1)
            {
                printf("Message was not sent\n");
                perror("msgsnd");
            }
            else
            {
                printf("Leaving the Shell. operating Complete\n");
                LOGIN_BIT = 0;
                return;
            }
        }
    }
}

/**
 * Description : This is the engine that takes an operating and dissects it to find out what it should do, and then
 *sends it to get done
 *@param: receiver = the memory context that we are dealing with
 *@param server_ID = the ID of the server that we are dealing with
 */
char *ExecuteOperation(struct my_msgbuf receiver, int server_ID)
{
    char copy[200];
    strcpy(copy, receiver.messagetext);
    char *token = strtok(receiver.messagetext, " ");
    int count_fields = 0;
    float value;
    char op[10];
    char *msg;
    /* char ret[100]; --> this a bug since we'll be returning an address on the stack */
    char *ret = (char *)malloc(sizeof(char) * 100);

    while (token)
    {
        if (strcmp(token, "Withdraw") == 0)
        {
            printf("Withdrawing funds");
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(receiver.messagetext, copy);
            msg = Withdraw(token);
            strcpy(ret, msg);
        }
        else if (strcmp(token, "Deposit") == 0)
        {
            printf("Depositing funds");
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(receiver.messagetext, copy);
            msg = Deposit(token);
            strcpy(ret, msg);
        }
        if (strcmp(token, "Request") == 0)
        {
            token = strtok(NULL, " ");
            value = atof(token);
            strcpy(receiver.messagetext, copy);
            msg = Request(token);
            strcpy(ret, msg);
        }
        token = strtok(NULL, " ");
        count_fields++;
    }
    return ret;
}

/**
 * Description : Pass in an amount in the form of a string to withdraw from the user
 * @param val_asString: the value that is being passe in
 */
char *Withdraw(char *val_asString)
{
    float val = atof(val_asString);
    printf("token = %f\n", val);
    int AccountNum = Login_Cookie;
    printf("Incoming Val = %f", val);
    char *localString;
    size_t len = 0;
    int bytes_read;
    char copy[100];
    char backup_String[100];
    // This will be the message to send back to the ATM
    /* char ret[100]; --> this a bug since we'll be returning an address on the stack */
    char *ret = (char *)malloc(sizeof(char) * 100);
    memset(ret, '\0', sizeof(*ret));

    /**
     * File Weaving technique. Open 2 files. Write updates file to 2nd one. Delete 1st one, and then
     * rename 2nd one to have name as 1st one. OS inherently protects filelock, and the benefit is
     * protection against memory overflow
     **/
    FILE *oldDB = fopen("database.txt", "r");
    if (oldDB == NULL)
    {
        printf("ERROR: FILE OPEN\n");
        exit(1);
    }

    FILE *newDB = fopen("database2.txt", "w");
    if (newDB == NULL)
    {
        printf("ERROR: FILE OPEN\n");
        exit(1);
    }

    /* Our counters for the fileLooping */
    int count_entries = 0;
    int count_fields = 0;

    /* Avoid tokenizing for unimportant entries */
    int processed_User_Flag = 0;
    int FIRST_PROCESS = 0;

    /* Read the entire file */
    while (bytes_read != -1)
    {
        int been_written = 0;
        bytes_read = getline(&localString, &len, oldDB);
        memset(backup_String, '\0', sizeof(backup_String));
        strcpy(backup_String, localString);
        char *token = strtok(localString, ",");

        /* Parse the string */
        while (token && !processed_User_Flag)
        {
            long int inputAccountNum = strtol(token, NULL, 10);
            /* The user already autheticated, we do not need to worry about the PIN again */
            if (count_fields == 0 && inputAccountNum == AccountNum)
            {
                // clear out our memory
                memset(copy, '\0', sizeof(copy));

                // grab the user ID, put it in the buffer
                strcat(copy, token);
                strcat(copy, ",");

                // Grab the PIN, append to buffer
                token = strtok(NULL, ",");
                strcat(copy, token);
                strcat(copy, ",");

                // Grab The Balance
                token = strtok(NULL, ",");
                float DBval = atof(token);
                float effective = DBval;

                if (DBval < val)
                {
                    effective = DBval;
                    sprintf(ret, "Funds Not Available. Balance=%f", effective);
                }
                else
                {
                    effective = DBval - val;
                    sprintf(ret, "Funds WithDrawn. Balance=%f", effective);
                }

                char dummy[100];
                // printf("In the database it is %s DBval=%f\nval=%f\neffective=%f\n" , copy, DBval, val, effective);
                if (processed_User_Flag == 0)
                {
                    fprintf(newDB, "%s%.2f\n", copy, effective);
                    // To not have to loop this each time
                    processed_User_Flag = 1;
                    // To not rewrite this line afterwards
                    FIRST_PROCESS = 1;
                }
            }
            else
            {
                /* If it not the user just write out to the second file */
                // fprintf(newDB, "%s", backup_String);
            }
            token = strtok(NULL, ",");
            count_fields++;
        }

        // We have read the whole string just output the rest of the DB
        if (processed_User_Flag > 0 && FIRST_PROCESS == 0)
        {
            if (strlen(backup_String) > 8)
            {
                fprintf(newDB, "%s", backup_String);
            }
        }

        // Flips the bit that tells us - we already wrote the current user
        if (FIRST_PROCESS == 1)
        {
            FIRST_PROCESS = 0;
        }

        count_fields = 0;
        count_entries++;
    }

    fclose(newDB);
    fclose(oldDB);
    int status = remove("database.txt");
    /* Clean up the files */
    char oldname[] = "database2.txt";
    char newname[] = "database.txt";

    int test = rename(oldname, newname);
    if (test == 0)
    {
        printf("File renamed successfully");
    }
    else
    {
        printf("Error: unable to rename the file");
    }

    return ret;
}

/**
 * Description : Verify if user Exists under this PIN and Account Number
 * @param Account Num - the user Account number. This will be cached locally upon login
 * @param PIN - authenticates the user based on content of database.txt
 * return: 1 if success, 0 of not successful
 */
char *Request(char *token)
{
    char *localString;
    size_t len = 0;
    int bytes_read;
    char *token2;
    /* char ret[100]; --> this a bug since we'll be returning an address on the stack */
    char *ret = (char *)malloc(sizeof(char) * 100);
    const char s[2] = ",";
    FILE *inFile = fopen("database.txt", "r");
    float count_entries = 0;
    float count_fields = 0;

    /* Read the Entire File */
    while (bytes_read != -1)
    {
        bytes_read = getline(&localString, &len, inFile);
        char *token = strtok(localString, ",");

        /* Parse each row */
        while (token)
        {
            /* Get Account Number */
            long int a = strtol(token, NULL, 10);
            if (count_fields == 0 && a == Login_Cookie)
            {
                token = strtok(NULL, ",");

                token = strtok(NULL, ",");
                sprintf(ret, "Funds Availalable = %s", token);
                return ret;
            }
            token = strtok(NULL, ",");
            count_fields++;
        }

        printf("The message is = %s", ret);
        fclose(inFile);
        count_fields = 0;
        count_entries++;
    }

    return ret;
}

/**
 * Description : Pass in an amount in the form of a string to withdraw from the user
 * @param val_asString: the value that is being passe in
 */
char *Deposit(char *val_asString)
{
    float val = atof(val_asString);
    printf("token = %f\n", val);
    int AccountNum = Login_Cookie;
    printf("Incoming Val = %f", val);
    char *localString;
    size_t len = 0;
    int bytes_read;
    char *token2;
    const char s[2] = ",";
    char copy[100];
    char backup_String[100];
    // This will be the message to send back to the ATM
    /* char ret[100]; --> this a bug since we'll be returning an address on the stack */
    char *ret = (char *)malloc(sizeof(char) * 100);
    memset(ret, '\0', sizeof(*ret));

    /**
     * File Weaving technique. Open 2 files. Write updates file to 2nd one. Delete 1st one, and then
     * rename 2nd one to have name as 1st one. OS inherently protects filelock, and the benefit is
     * protection against memory overflow
     **/
    FILE *oldDB = fopen("database.txt", "r");
    if (oldDB == NULL)
    {
        printf("ERROR: FILE OPEN\n");
        exit(1);
    }

    FILE *newDB = fopen("database2.txt", "w");
    if (newDB == NULL)
    {
        printf("ERROR: FILE OPEN\n");
        exit(1);
    }

    /* Our counters for the fileLooping */
    int count_entries = 0;
    int count_fields = 0;

    /* Avoid tokenizing for unimportant entries */
    int processed_User_Flag = 0;
    int FIRST_PROCESS = 0;

    /* Read the entire file */
    while (bytes_read != -1)
    {
        int been_written = 0;
        bytes_read = getline(&localString, &len, oldDB);

        memset(backup_String, '\0', sizeof(backup_String));
        strcpy(backup_String, localString);
        char *token = strtok(localString, ",");

        /* Parse the string */
        while (token && !processed_User_Flag)
        {
            long int inputAccountNum = strtol(token, NULL, 10);
            /* The user already autheticated, we do not need to worry about the PIN again */
            if (count_fields == 0 && inputAccountNum == AccountNum)
            {
                // clear out our memory
                memset(copy, '\0', sizeof(copy));

                // grab the user ID, put it in the buffer
                strcat(copy, token);
                strcat(copy, ",");

                // Grab the PIN, append to buffer
                token = strtok(NULL, ",");
                strcat(copy, token);
                strcat(copy, ",");

                // Grab The Balance
                token = strtok(NULL, ",");
                float DBval = atof(token);
                float effective = DBval + val;
                sprintf(ret, "%.2f Deposited. Balance=%.2f", val, effective);

                char dummy[100];
                if (processed_User_Flag == 0)
                {
                    fprintf(newDB, "%s%.2f\n", copy, effective);
                    // To not have to loop this each time
                    processed_User_Flag = 1;
                    // To not rewrite this line afterwards
                    FIRST_PROCESS = 1;
                }
            }
            else
            {
                /* If it not the user just write out to the second file */
                // fprintf(newDB, "%s", backup_String);
            }
            token = strtok(NULL, ",");
            count_fields++;
        }

        // We have read the whole string just output the rest of the DB
        if (processed_User_Flag > 0 && FIRST_PROCESS == 0)
        {
            if (strlen(backup_String) > 8)
            {
                fprintf(newDB, "%s", backup_String);
            }
        }

        // Flips the bit that tells us - we already wrote the current user
        if (FIRST_PROCESS == 1)
        {
            FIRST_PROCESS = 0;
        }

        count_fields = 0;
        count_entries++;
    }

    fclose(newDB);
    fclose(oldDB);
    /* Clean up the files */
    char oldname[] = "database2.txt";
    char newname[] = "database.txt";

    int test = rename(oldname, newname);
    if (test == 0)
    {
        printf("File renamed successfully");
    }
    else
    {
        printf("Error: unable to rename the file");
    }

    return ret;
}
/************************                   /End Post Authentication		 *************************/

/************************                   Authentication 				 *************************/
/**
 * Description : The log in method to check to see if the user exists
 * pass in the user account like this ACCOUNT PIN
 * @param: The input string to autheticate with
 * return: 1 for success, 0 for failure , -1 for bad string
 */
int Login(char *a)
{
    int count_fields = 0;
    int PIN;
    int Account;
    char *token = strtok(a, " ");

    if (a == NULL)
    {
        return -1;
    }

    while (token)
    {
        if (count_fields == 0)
        {
            Account = atoi(token);
        }
        else if (count_fields == 1)
        {
            PIN = atoi(token);
        }
        else
        {
            printf("Throw an error\n");
            return -1;
        }
        token = strtok(NULL, " ");
        count_fields++;
    }

    return user_Exists(Account, PIN - 1);
}

/**
 * Description : Verify if user Exists under this PIN and Account Number
 * @param Account Num - the user Account number. This will be cached locally upon login
 * @param PIN - authenticates the user based on content of database.txt
 * return: 1 if success, 0 of not successful
 */
int user_Exists(int AccountNum, int PIN)
{
    char *localString;
    size_t len = 0;
    int bytes_read;
    char *token2;
    const char s[2] = ",";
    FILE *inFile = fopen("database.txt", "r");
    float count_entries = 0;
    float count_fields = 0;

    /* Read the Entire File */
    while (bytes_read != -1)
    {
        bytes_read = getline(&localString, &len, inFile);
        char *token = strtok(localString, ",");

        /* Parse each row */
        while (token)
        {
            /* Get Account Number */
            long int a = strtol(token, NULL, 10);
            if (count_fields == 0 && a == AccountNum)
            {
                printf("We found the value\n");
                token = strtok(NULL, ",");

                /* Attempt to autheticate PIN */
                long int a = strtol(token, NULL, 10);
                if (a == PIN)
                {
                    printf("We have a winner, sir!\n");
                    /* Set Cookie */
                    Login_Cookie = AccountNum;
                    fclose(inFile);
                    return 1;
                }
            }

            token = strtok(NULL, ",");
            count_fields++;
        }

        fclose(inFile);
        count_fields = 0;
        count_entries++;
    }

    return 0;
}

/** accomponied by a test */
int test_user_Exists()
{
    // user_Exists(000117
    // Very very strange case, I think it reads it as hex or something, because the input parameter ends up being 79
    // instead of 117 because of leading 0s
    if (user_Exists(11, 23) == 1)
    {
        printf("Test User Exists Passed!\n");
    }
    else
    {
        printf("Test User Exists Failed!\n");
    }

    return 0;
}

/**
 * Description : This checks to see if the end user exists
 * @param: the User Account Number
 * return: If the user exists in the database or not
 */
void parseInputFile()
{
    char *localString;
    size_t len = 0;
    int bytes_read;
    char *token2;
    const char s[2] = ",";
    FILE *inFile = fopen("database.txt", "r");
    int one, two, three;
    int count_entries = 0;
    int count_fields = 0;

    while (bytes_read != -1)
    {
        bytes_read = getline(&localString, &len, inFile);
        char *token = strtok(localString, ",");
        while (token)
        {
            printf("token: %s\n", token);
            token = strtok(NULL, ",");
            count_fields++;
        }
    }
    fclose(inFile);
}
