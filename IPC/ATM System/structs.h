#include<stdio.h>
#include<stdlib.h> // for perror
#include<unistd.h> // for execlp
enum messagetype {BLOCKED,PIN, BALANCE, WITHDRAW, UPDATE_DB, PIN_WRONG, OK, NSF, FUNDS_OK,LOGIN,SUCCESS,FAILURE};

typedef struct DB_item{
    char acc_num[10];
    char pin[10];
    float funds;
}db_item;

typedef struct messages {
    long int message_type;
    enum messagetype msg_type;
    db_item contents;
}message;

union semun
{
    int val;
    struct semid_ds *buf;
    ushort  *array;
    struct seminfo *__buf;
};