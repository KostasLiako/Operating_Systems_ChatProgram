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
#include<time.h>
#include"checksum.h"


#define SEMKEY1 (key_t)7890
#define SEMKEY2 (key_t)1234
#define SEMKEY3 (key_t) 8909
#define PERMS 0600
#define SHMKEY (key_t)1357
#define SHMKEY2 (key_t)1244
#define SHMKEY3 (key_t)1278
#define SHMSIZE 100

void modifyMessage(checksum* ch,int arg) 
{ 
     for(int i = 0; i <strlen(ch->message); i++){
        int randnum = rand() % 101;
        //  printf("%d ", randnum);
        if(randnum <= arg){
            ch->message[i] = '_';
                    }
                }
}






int main(int argc, char *argv[]){


    int choice;

    printf("if you want to send message press 1, if you want to receive press 0\n");
    scanf("%d",&choice);
    getchar();

    switch (choice)
    {
    case 1:{
        /* code */
    

    union semun arg;
    arg.val=0;
    int semid,semid2,semid3;
    int shmid,shmid2,shmid3;
    int pid,pid2;
    int counter;
    
    char message[50];
    char recmessage[50];
    
    checksum* shmp;
    checksum* encrypt;
    checksum* decrypt;
    checksum* entry;
    
    entry = malloc(sizeof(checksum));
    srand(time(NULL));
    

    unsigned char hash1[MD5_DIGEST_LENGTH];
    unsigned char hash2[MD5_DIGEST_LENGTH];

    struct sembuf down = {0,-1,0}; //down operation 
    struct sembuf up = {0,1,0}; //up operation 


    
   
    entry = malloc(sizeof(checksum));
    //----CREATE SEMAPHORES AND SHARED MEMORY----
    semid = semget(SEMKEY1,1,IPC_CREAT | 0666);
    semid2 = semget(SEMKEY2,1,IPC_CREAT | 0666);
    semid3 = semget(SEMKEY3,1,IPC_CREAT | 0666);

    shmid = shmget(SHMKEY,sizeof(checksum),IPC_CREAT | 0600);
    shmid2 = shmget(SHMKEY2,sizeof(checksum),IPC_CREAT | 0600);
    shmid3 = shmget(SHMKEY3,sizeof(checksum),IPC_CREAT | 0600);



    //-----ATTACH SHARED MEMORY AND INIT SEMAPHORES---------
    shmp = (checksum*)shmat(shmid,(char*)0,0);
    encrypt = (checksum*)shmat(shmid2,(char*)0,0);
    decrypt = (checksum*)shmat(shmid3,(char*)0,0);

    if(shmp == NULL) printf("Error create shared memory\n");
    if(encrypt == NULL) printf("Error create second shared memory\n");
    if(decrypt == NULL) printf("Error create third shared memory\n");

    if(semid < 0){ printf("Error create semaphore\n"); exit(11);};
    if(semid2 < 0){ printf("Error create second semaphore\n"); exit(11);}
    

    if(semctl(semid,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}
    if(semctl(semid2,0,SETVAL,arg) < 0){ printf("Error Init second semaphore\n"); exit(12);}
    if(semctl(semid3,0,SETVAL,arg) < 0){ printf("Error Init third semaphore\n"); exit(12);}
    

    //------------CREATE ENC1 PROCESS---------
    pid = fork();

    
    if(pid < 0){
    
        perror("fork\n");
        exit(1);
    }else if(pid ==0 ){
        //-----ENC1 CODE----------

        semop(semid,&down,1);
   
        printf("ENC1 process running...\n\n");
       
        
        memcpy(recmessage,shmp->message,strlen(shmp->message)+1);
        
        //----GENERATE CHECKSUM OF MESSAGE------
        MD5((const unsigned char*)recmessage,strlen(recmessage)+1,hash1);
    
        memcpy(encrypt->message, recmessage, strlen(recmessage)+1);
        memcpy(encrypt->checksum, hash1, MD5_DIGEST_LENGTH);
        encrypt->status=0;
      
    

        
        semop(semid,&up,1);
        semop(semid2,&up,1);
        
        exit(pid);
    }
    else if(pid > 0){
        // --------- CREATE CHAN PROCESS---------
        pid2 = fork();
        if(pid2 == 0){

            //--------------CHAN CODE-------------
          
                 semop(semid2,&down,1);
                 printf("CHAN process running....\n\n");

               
                memcpy(entry->message, encrypt->message, strlen(encrypt->message)+1);
                memcpy(entry->checksum, encrypt->checksum, MD5_DIGEST_LENGTH);
                
                //-------MODIFY MESSAGE WITH RANDOM PROBABILITY-----
                modifyMessage(entry,atoi(argv[1]));
                
                memcpy(decrypt->message, entry->message,strlen(entry->message)+1);
                memcpy(decrypt->checksum, entry->checksum,MD5_DIGEST_LENGTH);

                

                semop(semid2,&up,1);
                semop(semid3,&up,1);
                sleep(1);
                counter =0;
                //--------RESEND MESSAGE-------
                while(decrypt->status != 0 || counter ==5){
                sleep(1);
                semop(semid3,&down,1);

                    
                    modifyMessage(entry,atoi(argv[1]));
                    printf("Resend the message\n");
                    memcpy(decrypt->message, entry->message,strlen(entry->message)+1);
                    memcpy(entry->message,encrypt->message,strlen(entry->message)+1);

                    if(counter == 5 ){
                         memcpy(decrypt->message, encrypt->message,strlen(entry->message)+1);
                         semop(semid3,&up,1);
                         break;}
                    counter++;
                semop(semid3,&up,1);
                }
                
                
                 
          
            exit(pid2);
        }
        else
        {   
            //-------------P1 CODE----------
            semop(semid,&up,1);
            semop(semid,&down,1);
            printf("P1 process running...\n");
            printf("Type your message..\n");
            
            fgets(message,sizeof(message), stdin);
            
            
            memcpy(shmp->message,message,strlen(message)+1);
            

            semop(semid,&up,1);
             wait(&pid);
             wait(&pid2);

            //---DELETE SEMAPHORES AND DETACH SHARE MEMORY
            if((semctl(semid,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                    exit(0);                                                                                
                }

            if((semctl(semid2,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                     exit(0);                                                                                
                 }

            shmctl(shmid, IPC_RMID, NULL);

            shmctl(shmid2, IPC_RMID, NULL);
                
            shmdt(shmp);
            shmdt(encrypt);
            shmdt(decrypt);

            free(entry);

                

               
        }
    
    

    }
    
    break;

    }

    case 0: {
        
        printf("Waiting to receive message\n\n");

    union semun arg;
    arg.val=0;
    int semid,semid2,semid3;
    int shmid,shmid2,shmid3;
    int pid,pid2;
    int counter;
    
    char message[50];
    char recmessage[50];
    
    checksum* enc1P1;
    checksum* chanEnc1;
    checksum* enc2Chan;
    checksum* entry;
    
    entry = malloc(sizeof(checksum));
    srand(time(NULL));
    

    unsigned char hash1[MD5_DIGEST_LENGTH];
    

    struct sembuf down = {0,-1,0}; //down operation 
    struct sembuf up = {0,1,0}; //up operation 


    
   
    entry = malloc(sizeof(checksum));
    //----CREATE SEMAPHORES AND SHARED MEMORY----
    semid = semget(SEMKEY1,1,IPC_CREAT | 0666);
    semid2 = semget(SEMKEY2,1,IPC_CREAT | 0666);
    semid3 = semget(SEMKEY3,1,IPC_CREAT | 0666);

    shmid = shmget(SHMKEY,sizeof(checksum),IPC_CREAT | 0600);
    shmid2 = shmget(SHMKEY2,sizeof(checksum),IPC_CREAT | 0600);
    shmid3 = shmget(SHMKEY3,sizeof(checksum),IPC_CREAT | 0600);



    //-----ATTACH SHARED MEMORY AND INIT SEMAPHORES---------
    enc1P1 = (checksum*)shmat(shmid,(char*)0,0);
    chanEnc1 = (checksum*)shmat(shmid2,(char*)0,0);
    enc2Chan = (checksum*)shmat(shmid3,(char*)0,0);

    if(enc1P1 == NULL) printf("Error create shared memory\n");
    if(chanEnc1 == NULL) printf("Error create second shared memory\n");
    if(enc2Chan == NULL) printf("Error create third shared memory\n");

    if(semid < 0){ printf("Error create semaphore\n"); exit(11);};
    if(semid2 < 0){ printf("Error create second semaphore\n"); exit(11);}
    

    if(semctl(semid,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}
    if(semctl(semid2,0,SETVAL,arg) < 0){ printf("Error Init second semaphore\n"); exit(12);}
    if(semctl(semid3,0,SETVAL,arg) < 0){ printf("Error Init third semaphore\n"); exit(12);}
    

    //------------CREATE ENC1 PROCESS---------
    pid = fork();

    
    if(pid < 0){
    
        perror("fork\n");
        exit(1);
    }else if(pid ==0 ){
        //-----ENC1 CODE----------

        semop(semid2,&down,1);
        semop(semid,&up,1);
        semop(semid,&down,1);
        printf("ENC1 process running...\n\n");


        memcpy(recmessage,chanEnc1->message,strlen(chanEnc1->message)+1);

        //-----CALCULATE CHECKSUM OF RECEIVE MESSAGE---
        MD5((const unsigned char*)recmessage,strlen(recmessage)+1,hash1);

        //-----IF CHECKSUM IS NOT EQUAL SET STATUS TO 1
        if(memcmp(hash1,chanEnc1->checksum,MD5_DIGEST_LENGTH) !=0 ){
            chanEnc1->status=1; 
            semop(semid2,&up,1);
         }

         sleep(1);
         while(chanEnc1->status !=0){
            
            sleep(1);
            semop(semid2,&down,1);
            memcpy(recmessage,chanEnc1->message,strlen(chanEnc1->message)+1);
            MD5((const unsigned char*)recmessage,strlen(recmessage)+1,hash1);
            if(memcmp(hash1,chanEnc1->checksum,MD5_DIGEST_LENGTH) == 0) chanEnc1->status=0;
                semop(semid2,&up,1);
    }       


            memcpy(enc1P1->message,recmessage,strlen(recmessage)+1);
        

        semop(semid2,&up,1);
        semop(semid,&up,1);
        exit(pid);
        
    }
    else if(pid > 0){
        // --------- CREATE CHAN PROCESS---------
        pid2 = fork();
        if(pid2 == 0){

            //--------------CHAN CODE-------------
                semop(semid3,&down,1);
               
                 printf("CHAN process running....\n\n");

                 memcpy(entry->message,enc2Chan->message,strlen(enc2Chan->message)+1);
                 memcpy(entry->checksum,enc2Chan->checksum,MD5_DIGEST_LENGTH);

                //---MODIFY INPUT MESSAGE----
                 modifyMessage(entry,atoi(argv[1]));

                 memcpy(chanEnc1->message,entry->message,strlen(enc2Chan->message)+1);
                 memcpy(chanEnc1->checksum,entry->checksum,MD5_DIGEST_LENGTH);

                semop(semid3,&up,1);
                semop(semid2,&up,1);

                 sleep(1);
                counter =0;
                //--------RESEND MESSAGE-------
                while(chanEnc1->status != 0 || counter ==5){
                sleep(1);
                semop(semid2,&down,1);

                    
                modifyMessage(entry,atoi(argv[1]));

                printf("Resend the message\n");
                memcpy(chanEnc1->message, entry->message,strlen(entry->message)+1);
                memcpy(entry->message,enc2Chan->message,strlen(entry->message)+1);

                if(counter == 5 ){
                         memcpy(chanEnc1->message,entry->message,strlen(entry->message)+1);
                         semop(semid2,&up,1);
                         break;}
                    counter++;
                semop(semid2,&up,1);
                }

                
                
                
                exit(pid2);
               
        }
        else
        {   
            //-------------P1 CODE----------
            wait(&pid);
            wait(&pid2);
            semop(semid,&down,1);
            printf("P1 process running...\n");

            memcpy(message,enc1P1->message,strlen(enc1P1->message)+1);

            //-----PRINT THE MESSAGE-----
            printf("Message is: %s \n",message);


            semop(semid,&up,1);

            if((semctl(semid,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                    exit(0);                                                                                
                }

            if((semctl(semid2,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                     exit(0);                                                                                
                 }

            shmctl(shmid, IPC_RMID, NULL);

            shmctl(shmid2, IPC_RMID, NULL);

            shmdt(enc1P1);
            shmdt(chanEnc1);
            shmdt(enc2Chan);

            free(entry);


           
    }

    } 

    }

    }
     return 0;
    

}






