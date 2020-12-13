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
#include"checksum.h"


#define SHMKEY3 (key_t)1278
#define SHMKEY (key_t)4567
#define SHMSIZE 100
#define SEMKEY (key_t)8909
#define SEMKEY2 (key_t)6789

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

  switch(choice){

    case 1: { 
      int semid;
      int semid2;

      union semun arg;
      arg.val=0;
      int shmid;    // p2->Enc2
      int shmid2;
      checksum* input;
      checksum* output;
      char message[50];
      char recMessage[50];
      unsigned char hash1[MD5_DIGEST_LENGTH];
      int pid;


      struct sembuf down = {0,-1,0}; //down operation 
      struct sembuf up = {0,1,0}; //up operation 

      //-----CREATE SEMAPHORE AND SHARE MEMEORY-----

      semid = semget(SEMKEY2,1,IPC_CREAT | 0666);
      semid2 = semget(SEMKEY,1,IPC_CREAT | 0666);

      shmid = shmget(SHMKEY,sizeof(checksum),IPC_CREAT | 0600);
      shmid2 = shmget(SHMKEY3,sizeof(checksum),IPC_CREAT | 0600);

      if(semid < 0){ printf("Error create semaphore\n"); exit(11);}
      if(semid2 < 0){ printf("Error create semaphore\n"); exit(11);}

      //-----ATTACH SHARED MEMORY AND INIT SEMAPHORES---------
      if(semctl(semid2,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}
      if(semctl(semid,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}

      input = (checksum*)shmat(shmid,(char*)0,0);
      output = (checksum*)shmat(shmid2,(char*)0,0);


      pid = fork();

      if(pid < 0){
        perror("fork\n");
        exit(1);}
      
      else if(pid == 0){

          semop(semid,&down,1);
          

          printf("ENC2 process running....\n\n");
          
          memcpy(recMessage,input->message,strlen(input->message)+1);

          MD5((const unsigned char*)recMessage,strlen(recMessage)+1,hash1);

          memcpy(output->message,recMessage,strlen(recMessage)+1);
          memcpy(output->checksum, hash1, MD5_DIGEST_LENGTH);

          semop(semid,&up,1);
          semop(semid2,&up,1);

      }
      else if(pid > 0){

        semop(semid,&up,1);
        semop(semid,&down,1);

        printf("P2 process running....\n");
        printf("Type your message\n");

        fgets(message,sizeof(message),stdin);
        

        memcpy(input->message,message,strlen(message)+1);

        semop(semid,&up,1);
        wait(&pid);

        shmdt(input);

        if((semctl(semid,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                    exit(0);                                                                                
                }

        if((semctl(semid2,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                     exit(0);                                                                                
                 }

        shmctl(shmid2, IPC_RMID, NULL); 


      }

              break;}
  

    case 0:{

      printf("Waiting to receive message..\n");

  int semid;
  int semid2;
  union semun arg;
  arg.val=0;
  int shmid ;
  int shmid2;
  checksum* shmp;
  checksum* output;
  char message[50];
  unsigned char hash1[MD5_DIGEST_LENGTH];
  int pid;

  struct sembuf down = {0,-1,0}; //down operation 
  struct sembuf up = {0,1,0}; //up operation 

  

  semid = semget(SEMKEY,1,IPC_CREAT | 0666);
  semid2 = semget(SEMKEY2,1,IPC_CREAT |0666);
  

  if(semid < 0){ printf("Error create semaphore\n"); exit(11);}
  if(semid2 < 0){ printf("Error create semaphore\n"); exit(11);}

  if(semctl(semid2,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}
  //if(semctl(semid,0,SETVAL,arg) < 0){ printf("Error Init semaphore\n"); exit(12);}

  shmid = shmget(SHMKEY3,sizeof(checksum),IPC_CREAT | 0600);
  shmid2 = shmget(SHMKEY,sizeof(checksum),IPC_CREAT | 0600);
  shmp = (checksum*)shmat(shmid,(char*)0,0);
  output = (checksum*)shmat(shmid2,(char*)0,0);
 
  pid=fork();

  if(pid < 0){
    
        perror("fork\n");
        exit(1);}
  else if(pid == 0){
    semop(semid,&down,1);
    semop(semid2,&up,1);
    semop(semid2,&down,1);
    printf("ENC2 process running..... \n\n");
       
  memcpy(message,shmp->message,strlen(shmp->message)+1);
  
  
  MD5((const unsigned char*)message,strlen(message)+1,hash1);
  

  //--------IF MESSAGE ISNT EQUAL RESEND THE MESSAGE-----
  if(memcmp(hash1,shmp->checksum,MD5_DIGEST_LENGTH) !=0 ){
    shmp->status=1; 
    semop(semid,&up,1);
  }
    sleep(1);
    while(shmp->status !=0){
      
      sleep(1);
      semop(semid,&down,1);
      memcpy(message,shmp->message,strlen(shmp->message)+1);
      
      MD5((const unsigned char*)message,strlen(message)+1,hash1);
      if(memcmp(hash1,shmp->checksum,MD5_DIGEST_LENGTH) == 0) shmp->status=0;
      semop(semid,&up,1);
    }

  



  memcpy(output->message,message,strlen(message)+1);
 
  
  
  semop(semid,&up,1);
  semop(semid2,&up,1);
  exit(pid);
  }
  else if(pid > 0){
      
    sleep(1);
    wait(&pid);
    semop(semid2,&down,1);
    //wait(&pid);
    
    printf("P2 process running....\n");

    printf("message is : %s\n",output->message);
    
   
    semop(semid2,&up,1);
    wait(&pid);
    shmdt(shmp);

    if((semctl(semid,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                    exit(0);                                                                                
                }

    if((semctl(semid2,0,IPC_RMID,0))==-1) {                                // Return semaphore a 
                    perror("\nCan't RPC_RMID.");                                             
                     exit(0);                                                                                
                 }

    shmctl(shmid2, IPC_RMID, NULL); 
                             
  }
  break;
    }
  }

  return 0;
}