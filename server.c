#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX 80 
#define PORT 2029
#define SA struct sockaddr 

typedef struct threadArgs{
    int *sockfd;
    struct sockaddr_in client;
}Args;


typedef struct message{
    char *msg;
    struct message *next;
}Message;

typedef struct messageBox{
    pthread_mutex_t boxLock;
    char name[25];     //name of message box
    Message *queue;
    struct messageBox *next;
}MessageBox;

MessageBox *head;


pthread_mutex_t structLock;

//debugging method
void printQueue(){
    MessageBox * current = head;
    if(current != NULL){
        Message * q = current->queue;
        while(q != NULL){

            printf("Print: %s\n", q->msg);

            q = q->next;
        }
    }
}

//Find char C from s starting at X
int charAt(char * s,char c, int x){

    for(x;x<strlen(s);x++){

        if(s[x] == c){
            return x;
        }
    }

    return 0;
    
}

void printBoxes(){
    MessageBox *current = head;
    while(current != NULL){
        printf("BOX: %s\n",current->name);
        current = current->next;
    }
}

//CREATE BOX METHOD
//Parameters boxName contains the name of the box from user input, sockfd is the client socket file descriptor
//ip stores the ip of the client
void createBox(char *boxName, int sockfd, char *ip){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    //printBoxes();
    MessageBox *current = head;
    //LOCK ENTIRE MESSAGEBOX STRUCTURE TO PREVENT OTHER CLIENTS FROM MODIFYING/SEARCHING THROUGH IT
    //WHILE CREATING THE NEW BOX
    pthread_mutex_lock(&structLock);
    //Head messageBox node is empty or available
    if(head->name == NULL || strcmp(head->name,"") == 0){
        MessageBox *newBox = (MessageBox *) malloc(sizeof(MessageBox));
        strcpy(newBox->name,boxName);
        newBox->next=head->next;
        head=newBox;
        printf("%d-%d-%d %s CREAT\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
        pthread_mutex_unlock(&newBox->boxLock);
        pthread_mutex_unlock(&structLock);
        //printBoxes();
        write(sockfd,"OK!",sizeof(char) * 8);
        return;
    }
    //Found an existing message box with the name given by the user
    else if(strcmp(head->name,boxName) == 0){
        write(sockfd,"ER:EXIST",sizeof(char) * 8);
        printf("%d-%d-%d %s ER:EXIST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
        pthread_mutex_unlock(&structLock);
        return;
    }
    //Search the list of messages boxes to find existing box or add new box
    else{
        while(current->next != NULL){

            if(strcmp(current->next->name,boxName) == 0 ){
                printf("%d-%d-%d %s ER:EXIST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:EXIST",sizeof(char) * 8);
                pthread_mutex_unlock(&structLock);
                return;
            }
            else if(strcmp(current->next->name,"") == 0){
                //Create new message box 
                MessageBox *newBox = (MessageBox *)malloc(sizeof(MessageBox));
                strcpy(newBox->name,boxName);
                newBox->next=current->next->next;
                pthread_mutex_unlock(&newBox->boxLock);
                current->next=newBox;
                //UNLOCK MESSAGEBOX LOCK
                pthread_mutex_unlock(&structLock);
                //printBoxes();
                printf("%d-%d-%d %s CREAT\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"OK!", sizeof(char) * 3);
                return;
            }
            current = current->next;
        }
        //Create new message box
        MessageBox *newBox = (MessageBox *)malloc(sizeof(MessageBox));
        pthread_mutex_unlock(&newBox->boxLock);
        strcpy(newBox->name,boxName);
        current->next=newBox;
    }
    //UNLOCK MESSAGEBOX STRUCUTURE LOCK
    pthread_mutex_unlock(&structLock);
    //printBoxes();
    printf("%d-%d-%d %s CREAT\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
    write(sockfd,"OK!", sizeof(char) * 3);
    return;
}

//ACCESSBOX METHOD ATTEMPTS TO OPEN A MESSAGEBOX AND STAYS OPEN UNTIL USER CLOSES IT OR DISCONNECTS
int accessBox(char * boxName, int sockfd, char *ip){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    MessageBox *current = head;
    //No message boxes
    if(current == NULL){
        printf("%d-%d-%d %s ER:NEXST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
        write(sockfd,"ER:NEXST",sizeof(char) * 8);
        return;
    }
    //iterate through list to the message box to open
    while(current != NULL){
        if(strcmp(current->name,boxName) == 0){
            //found a matching box print out the messages?
            if(pthread_mutex_trylock(&current->boxLock) == 0){
                printf("%d-%d-%d %s OPNBX\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"OK!",sizeof(char) * 3);
                char buff[MAX];
                for(;;){ //run this forever until box is closed


                    bzero(buff,sizeof(buff));
                    int readCheck = read(sockfd, buff, sizeof(buff));

                    if(readCheck <= 0){
                        printf("%d-%d-%d %s GDBYE\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        printf("%d-%d-%d %s disconnected\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        pthread_mutex_unlock(&current->boxLock);
                        return;
                    }

                    //NXTMG
                    if(strncmp(buff,"NXTMG",5) == 0){
                        //get message

                        if(current->queue == NULL){
                            //ER:EMPTY
                            printf("%d-%d-%d %s ER:EMPTY\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                            write(sockfd,"ER:EMPTY",sizeof(char)*10);
                        }
                        else{
                            char toSend[strlen(current->queue->msg)+100];
                            sprintf(toSend,"OK!%d!%s",strlen(current->queue->msg),current->queue->msg);
                            write(sockfd,toSend,strlen(toSend));
                            printf("%d-%d-%d %s NXTMG\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                            current->queue = current->queue->next; //move down chain should free old
                        }
                    }
                    //PUTMG
                    else if(strncmp(buff,"PUTMG",5) == 0){
                        //Extract the arg from PUTMG that contains the length of the user's message
                        int last = charAt(buff,'!',6);
                        char numTemp[last-5];
                        memcpy(numTemp,&buff[6],last-6);

                        int msgLength = atoi(numTemp);
                        char message[msgLength+100];
                        
                        char buffTemp[msgLength+MAX+10];
                        strncpy(buffTemp,&buff[last+1],(MAX-last)-1);
                        buffTemp[MAX-last] = '\0';

                        if(msgLength > (MAX - last)){
                            read(sockfd,message,sizeof(message));
                            strcat(buffTemp,message);
                            strcpy(message,buffTemp);

                            //printf("Message: %s\n",message);

                            
                        }else{
                            strcpy(message,buffTemp);
                        }

                        //keep reading to check for more
                        //Not done
                        //Put message at end of chain
                        //if first message put at top
                        Message * currentMsg = current->queue;
                        //Set head queue message if it does not exist
                        if(currentMsg == NULL){
                            current->queue = (Message *)malloc(sizeof(Message));
                            current->queue->msg = (char *)malloc(strlen(message)+1);
                            strcpy(current->queue->msg,message);
                            current->queue->next = NULL;
                        }
                        else{ //get the end of the queue
                            while(currentMsg->next != NULL){
                                
                                currentMsg = currentMsg->next;
                            }
                            currentMsg->next = (Message *)malloc(sizeof(Message));
                            currentMsg->next->msg = (char *)malloc(strlen(message)+1);
                            strcpy(currentMsg->next->msg,message);
                            currentMsg->next->next = NULL;
                        }
                        printf("%d-%d-%d %s PUTMG\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        char toSend[3+strlen(message)];
                        //sprintf(toSend,"OK!%d",strlen(message));
                        //write(sockfd,toSend,sizeof(toSend));

                    }
                    //CLSBX
                    else if(strncmp(buff,"CLSBX",5) == 0){
                        //Close box (unlock)
                        char boxtoclose[25];
                         if(buff[5] != ' ' || buff[6]<65 || buff[6] >122 || (strlen(buff) < 11) || (strlen(buff) > 31)){
                                printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                                write(sockfd,"ER:WHAT?",sizeof(char) * 8);
                        }
                        else{
                            memcpy(boxtoclose,&buff[6],25);
                            if(strcmp(current->name,boxtoclose) == 0){
                                //UNLOCK CURRENT MESSAGE BOX
                                pthread_mutex_unlock(&current->boxLock);
                                printf("%d-%d-%d %s CLSBX\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                                write(sockfd,"OK!",sizeof(char) * 3);
                                return;
                            }
                            else{
                                printf("%d-%d-%d %s ER:NOOPN\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                                write(sockfd,"ER:NOOPN",sizeof(char) * 8);
                            }
                        
                        }
                    }
                    else{
                        printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        write(sockfd,"ER:WHAT?",sizeof(char) * 8);
                        
                    }
                }
            }
            else{
                printf("%d-%d-%d %s ER:OPEND\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:OPEND",sizeof(char) * 8);
                return;
            }
        }
        current = current->next;
    }

    //Does not exist
    printf("%d-%d-%d %s ER:NEXST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
    write(sockfd,"ER:NEXST",sizeof(char) * 10);
    return;

}

//DELETEBOX METHOD DELETES THE MESSAGEBOX FROM LIST BY SETTING THE NAME TO ""
//PREVENTS DELETION OF A BOX THAT IS NOT EMPTY
void deleteBox(char *boxName, int sockfd, char *ip){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    MessageBox *current = head;
    pthread_mutex_lock(&structLock);
    //No boxes
    if(current->name == NULL){
        printf("%d-%d-%d %s ER:NEXST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
        write(sockfd,"ER:NEXST",sizeof(char)*8);
        pthread_mutex_unlock(&structLock);
        return;
    }
    if(strcmp(current->name,boxName) == 0){
        if(pthread_mutex_trylock(&current->boxLock) == 0){
            if(current->queue  == NULL){
                strcpy(current->name,"");
                pthread_mutex_unlock(&current->boxLock);
                printf("%d-%d-%d %s DELBX\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"OK!",sizeof(char) * 8);
                //printBoxes();
                pthread_mutex_unlock(&structLock);
                return;
             }
             else{
                printf("%d-%d-%d %s ER:NOTMT\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:NOTMT",sizeof(char) * 8);
                pthread_mutex_unlock(&current->boxLock);
                pthread_mutex_unlock(&structLock);
                return;
            }
        }
        else{
            printf("%d-%d-%d %s ER:OPEND\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
            write(sockfd,"ER:OPEND",sizeof(char) * 8);
            pthread_mutex_unlock(&structLock);
            return;
        }
    }

    while(current->next != NULL){
            if(strcmp(current->next->name,boxName) == 0) {
                if((pthread_mutex_trylock(&current->next->boxLock) == 0)){
                    if(current->next->queue  == NULL){
                        strcpy(current->next->name,"");
                        pthread_mutex_unlock(&current->next->boxLock);
                        printf("%d-%d-%d %s DELBX\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        write(sockfd,"OK!",sizeof(char) * 3);
                        //printBoxes();
                        pthread_mutex_unlock(&structLock);
                        return;
                    }
                    else{
                        pthread_mutex_unlock(&current->next->boxLock);
                        printf("%d-%d-%d %s ER:NOTMT\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                        write(sockfd,"ER:NOTMT",sizeof(char) * 8);
                        pthread_mutex_unlock(&structLock);
                        return;
                    }
                    pthread_mutex_unlock(&current->next->boxLock);
                }
                else{
                    printf("%d-%d-%d %s ER:OPEND\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                    write(sockfd,"ER:OPEND",sizeof(char) * 8);
                    pthread_mutex_unlock(&structLock);
                    return;
                }
            }
            current = current->next;
        }
    printf("%d-%d-%d %s ER:NEXST\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
    write(sockfd,"ER:NEXST",sizeof(char) * 8);
    pthread_mutex_unlock(&structLock);
    return;
}

// Function designed for chat between client and server. 
void * func(void * args) { 

    Args *myargs = (Args *) args;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((myargs->client).sin_addr), ip, INET_ADDRSTRLEN);
    printf("%s\n", ip);
    int sockfd  = myargs->sockfd;
    char buff[MAX]; 
    int openBox=0;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    bzero(buff, MAX); 
    
    write(sockfd,"HELLO DUMBv0 ready!",sizeof(char) * 19);
    printf("%d-%d-%d %s connected\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
    printf("%d-%d-%d %s HELLO\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
    
    for (;;) { 
        bzero(buff, MAX); 
      

        int readCheck = read(sockfd, buff, sizeof(buff));

        if(readCheck <= 0){
            close(sockfd);
            pthread_exit(NULL);
            return;
        }
        //GDBYE
        if(strncmp(buff,"GDBYE",5) == 0){
            printf("%d-%d-%d %s disconnected\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
            printf("%d-%d-%d %s GDBYE\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
        }
        //CREAT
        else if(strncmp(buff,"CREAT",5) == 0){
            char boxtofind[25];

            if(buff[5] != ' ' || buff[6]<65 || buff[6] >122 || (strlen(buff) < 11) || (strlen(buff) > 31)){
                printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:WHAT?",sizeof(char) * 8);
            }
            else{
                memcpy(boxtofind,&buff[6],25);
                createBox(boxtofind,sockfd, ip);
            }
        }
        //OPNXBX
        else if(strncmp(buff,"OPNBX",5) == 0  ){
            char boxtofind[25];
            if(buff[5] != ' ' || buff[6]<65 || buff[6] >122 || (strlen(buff) < 11) || (strlen(buff) > 31)){
                printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:WHAT?",sizeof(char) * 8);
            }
            else{
                memcpy(boxtofind,&buff[6],25);
                accessBox(boxtofind,sockfd,ip);
            }
        }
        else if(strncmp(buff,"DELBX",5) == 0){
            char boxtofind[25];
            if(buff[5] != ' ' || buff[6]<65 || buff[6] >122 || (strlen(buff) < 11) || (strlen(buff) > 31)){
                printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:WHAT?",sizeof(char) * 8);
            }
            else{
                memcpy(boxtofind,&buff[6],25);
                deleteBox(boxtofind,sockfd,ip);
            }
        }
        else if(strncmp(buff,"NXTMG",5) == 0 || strncmp(buff,"CLSBX",5) == 0 || strncmp(buff,"PUTMG", 5) == 0 ){
            printf("%d-%d-%d %s ER:NOOPN\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
            write(sockfd,"ER:NOOPN",sizeof(char) * 8);

        }
        else if(strncmp(buff,"OPNBX",5) == 0 ){
            char boxtofind[25];
            if(buff[5] != ' ' || buff[6]<65 || buff[6] >122 || (strlen(buff) < 11) || (strlen(buff) > 31)){
                printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
                write(sockfd,"ER:WHAT?",sizeof(char) * 8);
            }
            else{
                memcpy(boxtofind,&buff[6],25);
                accessBox(boxtofind,sockfd,ip);
                openBox=1;
            }
        }
        else{
            printf("%d-%d-%d %s ER:WHAT?\n",tm.tm_year+ 1900, tm.tm_mday, tm.tm_mon + 1, ip);
            write(sockfd,"ER:WHAT?", sizeof(char) * 8);
        }
    }
    } 


  

int main(int argc, char **argv){

    if(argc < 2){
        printf("Need Port Number\n");
        exit(0);
    }

    int porty = atoi(argv[1]);
    //printf("Porty %d\n",porty);

    // //making head something
    head = (MessageBox *)malloc(sizeof(MessageBox));
    // strcpy(head->name,"tester");
    
    // MessageBox * nextTest = (MessageBox *)malloc(sizeof(MessageBox));
    // strcpy(nextTest->name,"wonder");
    // head->next = nextTest;


    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(porty); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    }
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 100)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    

    
    int i = 0;
    pthread_t tids[100];

    while(1){
        // Accept the data packet from client and verification 
        len = sizeof(cli); 
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("server acccept failed...\n"); 
            exit(0); 
        } 
        else{
            Args *myargs={&connfd,&cli};
            printf("server acccept the client... ");
            pthread_create(&tids[i],NULL,func,(void *) myargs);
            
            i++; 
        }


    }
  
    // After chatting close the socket 
    close(sockfd); 
} 