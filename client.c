#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#define MAX 80 
#define PORT 2029
#define SA struct sockaddr 
char *commands [9]={"HELLO","GDBYE","CREAT","OPNBX","NXTMG","PUTMG","DELBX","CLSBX","help"};

//Find char C from s strarting at X
int charAt(char * s,char c, int x){

    for(x;x<strlen(s);x++){

        if(s[x] == c){
            return x;
        }
    }

    return 0;
    
}

void getStr(char **f)
{
    
    int i;
    for(i =0; 1;i++)
    {    
        if(i)//I.e if i!=0
            *f = (char*)realloc((*f),i+1);
        else
            *f = (char*)malloc(i+1);
        (*f)[i]=getchar();
        if((*f)[i] == '\n')
        {
            (*f)[i]= '\0';
            break;
        }
    }   
}


void func(int sockfd) 
{ 
    char *task;
    char buff[MAX]; 
    char commandBuffer[5];
    int n; 
    bzero(buff,sizeof(buff));
    read(sockfd,buff,sizeof(buff));
    if(strcmp(buff,"HELLO DUMBv0 ready!") == 0)
        printf("client is connected to the server and ready to go!\n");
    else{
        printf("unable to connect to the server.\n");
        return;
    }
    for (;;) {

        printf(">");
        bzero(buff, sizeof(buff)); 
        bzero(commandBuffer,sizeof(commandBuffer));
    
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') ;
        buff[n-1]='\0';

        //CREATE COMMAND
        if(strcmp(buff,"create") == 0){
            
            printf("what is the name of your new message box?\n");
            printf("create>");
            strcpy(buff,"CREAT ");
            char boxName[25];
            //Read user input
            n=0;
            while ((boxName[n++] = getchar()) != '\n') ;
                boxName[n - 1]='\0';
        
                strcat(buff,boxName);
                //write to server 'CREAT arg0'
                write(sockfd,buff,sizeof(buff));
                bzero(buff,sizeof(buff));
                //read server response
                read(sockfd, buff, sizeof(buff));
                if(strcmp(buff,"OK!") == 0){
                    printf("box %s was successfully created!\n",boxName);
                }
                else if(strcmp(buff,"ER:WHAT?") ==0){
                    printf("Your command is in some way broken or malformed.\n");
                }
                else if(strcmp(buff,"ER:EXIST") ==0 ){
                    printf("box %s already exists!\n",boxName);
                }

            
        }
        //PUTMG COMMAND BUT NO OPEN BOX
        else if(strcmp(buff,"put") == 0 ){
            printf("what is your message?\nput>");
            char * bigMsg = NULL;
            getStr(&bigMsg);
            char payLoad[strlen(bigMsg)+20]; //for PUTMG and extra
            sprintf(payLoad,"PUTMG!%d!%s",strlen(bigMsg),bigMsg);
            write(sockfd,payLoad,sizeof(payLoad));
            bzero(buff,sizeof(buff));
            read(sockfd, buff, sizeof(buff));
        }
        //DELBX COMMAND
        else if(strcmp(buff,"delete") == 0){
            printf("what is the name of the message box you wish to delete?\n");
            printf("delete>");
            strcpy(buff,"DELBX ");
            char boxName[25];
            n=0;
            while ((boxName[n++] = getchar()) != '\n') ;
            boxName[n - 1]='\0';
               
            strcat(buff,boxName);
            //write to server 'DELBX arg0'
            write(sockfd,buff,sizeof(buff));
            bzero(buff,sizeof(buff));
            //read response from server
            read(sockfd, buff, sizeof(buff));
            if(strcmp(buff,"OK!") == 0)
                printf("Box %s was successfully deleted.\n",boxName);
            else if(strcmp(buff,"ER:WHAT?") ==0){
                    printf("Your command is in some way broken or malformed.\n");
            }
        
            
        }
        //CLSBX COMMAND BUT NO OPEN BOX
        else if(strcmp(buff,"close") == 0){
            printf("what is the name of the message box you wish to close?\n");
            printf("close>");
            strcpy(buff,"CLSBX ");
            char boxName[25];
            n=0;
            while ((boxName[n++] = getchar()) != '\n') ;
            boxName[n - 1]='\0';
               
            strcat(buff,boxName);
            write(sockfd,buff,sizeof(buff));
            bzero(buff,sizeof(buff));
            read(sockfd, buff, sizeof(buff)); 
           
        }
        //NXTMG COMMAND BUT NO OPEN BOX
        else if(strcmp(buff,"next") == 0 ){
            

            write(sockfd,"NXTMG",sizeof(buff));
            bzero(buff,sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            
        }
        //OPNBX COMMAND
        else if(strcmp(buff,"open") == 0){
            printf("what is the name of the message box you wish to open?\n");
            printf("open> ");
            strcpy(buff,"OPNBX ");
            char boxName[25];
            n=0;
            while ((boxName[n++] = getchar()) != '\n') ;
            boxName[n - 1]='\0';
            strcat(buff,boxName);
            
            write(sockfd,buff,sizeof(buff));
            bzero(buff,sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            
            if(strcmp(buff,"OK!")==0){
                printf("Success! Box \"%s\" Is Open",boxName);
                //LOOP UNTIL BOX IS CLOSED OR USER QUITS
                for(;;){

                    printf("\n%s:> ",boxName);

                    bzero(buff,sizeof(buff));
                    n = 0; 
                    while ((buff[n++] = getchar()) != '\n') ;
                    buff[n-1]='\0';

                   
                    //PUTMG
                    if(strcmp(buff,"put") == 0){
                        printf("Put Message:>");
                        char * bigMsg = NULL;
                        getStr(&bigMsg);
                        char payLoad[strlen(bigMsg)+20]; //for PUTMG and extra
                        sprintf(payLoad,"PUTMG!%d!%s",strlen(bigMsg),bigMsg);
                        write(sockfd,payLoad,sizeof(payLoad));
                        bzero(buff,sizeof(buff));
                      //  read(sockfd, buff, sizeof(buff));
                    //    if(strncmp(buff,"OK!",3) == 0)
                           printf("Message %s was successfully added to box \"%s\".\n",bigMsg,boxName);
                    }
                    //NXTMG
                    else if(strcmp(buff,"next") == 0){
                        char nextTemp[] = "NXTMG";
                        write(sockfd,nextTemp,sizeof(nextTemp)); 
                        bzero(buff, sizeof(buff));
                        read(sockfd,buff,sizeof(buff));
                        if(strcmp(buff,"ER:EMPTY") == 0)
                            printf("Box \"%s\" is empty.\n",boxName);
                        else{


                        //printf("%s\n",buff);
                        //get message legnth OK!#!message
                        int last = charAt(buff,'!',3);

                        if(last == 0){
                            printf("No more next\n");
                            break;
                        }

                        char numTemp[last-3];
                        memcpy(numTemp,&buff[3],last-3);

                        int msgLength = atoi(numTemp);

                        char message[msgLength+100];
                        char buffTemp[msgLength+MAX+10];
                            

                        memcpy(buffTemp,&buff[last+1],(MAX-last)-1);
                        buffTemp[MAX-last] = '\0';

                        if(msgLength > (MAX - last)){
                            //buffTemp[MAX+1] = '\0';
                            read(sockfd,message,sizeof(message));
                            strcat(buffTemp,message);
                            strcpy(message,buffTemp);

                                
                        }else
                            strcpy(message,buffTemp);
                        
                            printf("Next message is: %s\n",message);
                        }
                        //CLSBX
                        }else if(strcmp(buff,"close") == 0){
                            bzero(buff,sizeof(buff));
                            strcpy(buff,"CLSBX ");
                            printf("what is the name of the message box you wish to close?\nclose>");
                            char boxName[25];
                            n=0;
                            while ((boxName[n++] = getchar()) != '\n') ;
                             boxName[n - 1]='\0';
                            strcat(buff,boxName);
                            write(sockfd,buff,sizeof(buff));
                            bzero(buff, sizeof(buff));
                            read(sockfd,buff,sizeof(buff));
                            if(strcmp(buff,"OK!") == 0){
                                printf("Box closed\n");
                                break;
                            }
                            else if(strcmp(buff,"ER:NOOPN")==0){
                                printf("You do not have that box currently open.\n");
                            }
                            else if(strcmp(buff,"ER:WHAT?")==0){
                                printf("Your command is in some way broken or malformed.\n");
                            }
                            
                        }
                        else if(strcmp(buff,"quit") == 0){

                            printf("exiting DUMBv%d\n",sockfd);
                            write(sockfd,"GDBYE",sizeof(char) * 5);
                            if(read(sockfd,buff,sizeof(buff)) == 0)
                                close(sockfd);
                            exit(0);
                            

                        }
                        else{
                            write(sockfd,buff,sizeof(buff));
                            bzero(buff,sizeof(buff));
                            read(sockfd,buff,sizeof(buff));
                            if(strcmp(buff,"ER:WHAT?") == 0)
                                printf("Your command is in some way broken or malformed.\n");

                        }



                    }
             
                }else if(strcmp(buff,"ER:NEXST") == 0){
                    printf("Box does not exist\n");
                }else if(strcmp(buff,"ER:OPEND") == 0){
                    printf("Box is opened\n");
                }
                else if(strcmp(buff,"ER:WHAT?") ==0){
                    printf("Your command is in some way broken or malformed.\n");
                 }
        
        }
        //list available commands
        else if(strcmp(buff,"help") == 0){
            printf("Available commands: \nquit\ncreate\nopen\nnext\nput\ndelete\nclose\n");
        }
        //GDBYE
        else if(strcmp(buff,"quit") == 0 ){
            printf("exiting DUMBv0\n");
            write(sockfd,"GDBYE",sizeof(char) * 5);
            close(sockfd);
            exit(0);
            
            
        }
        //invalid command
        else{
            write(sockfd, buff, sizeof(buff)); 
            bzero(buff,sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            if(strncmp(buff,"ER:WHAT?",8) == 0){
                printf("Your command is in some way broken or malformed.\n");
            }
        }
        



                
                 
                
        //ERRs
         if(strcmp(buff,"ER:NOOPN")==0){
            printf("You do not have any message box currently open.\n");

         }
         else if(strcmp(buff,"ER:NOTMT")==0){
            printf("The box you are attempting to delete is not empty, so it cannot be deleted.\n");
         }
         else if(strcmp(buff,"ER:NEXST")==0){
            printf("A box with that name does not exist.\n");
         }
         else if(strcmp(buff,"ER:OPEND")==0){
            printf("This box is currently opened by another user.\n");
         }
         else if(strcmp(buff,"ER:EMPTY")==0){
            printf("No messages left in this message box.\n");
         }
         else if(strcmp(buff,"ER:EXIST")==0){
            printf("A box with that name already exists.\n");
         }
         


       }
   }
       
        
    

  
int main(int argc, char **argv)
{   

    if(argc < 3){
        printf("Need Port and IP\n");
        exit(0);
    }

    int porty = atoi(argv[2]);


    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
    servaddr.sin_port = htons(porty); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 
  
    // function for chat 
    func(sockfd); 
  
    // close the socket 
    close(sockfd); 
    
}