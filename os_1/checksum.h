#include<stdio.h> 
#include<stdlib.h> 
#include<sys/wait.h> 
#include<unistd.h> 
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<string.h>
#include<openssl/md5.h>


typedef struct checksum{
   char message[50];
   char checksum[16];
   int status;
   
}checksum;

 union semun {
                int val;                /* value for SETVAL */
                struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
                ushort *array;          /* array for GETALL & SETALL */
                struct seminfo *__buf;  /* buffer for IPC_INFO */
                void *__pad;
        };


