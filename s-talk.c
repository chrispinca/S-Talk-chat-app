#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include "list.h"

//Initialize lists and ports as global variables to be used in all threads
List* sendList;
List* receiveList;
int myPort;
const char *otherMachineName;
int otherMachinePort;

//thread synchronizing using mutex
pthread_mutex_t sendListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t receiveListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sendListFlag = PTHREAD_COND_INITIALIZER;
pthread_cond_t receiveListFlag = PTHREAD_COND_INITIALIZER;

//exit s-talk signal/flag
bool exit_s_talk = false;


void* keyInputThread(void* arg) {
    char buffer[1024];
    puts("Enter your messages below (exit by typing '!'): \n");
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("Error on input read");
            exit(EXIT_FAILURE);
        }

        if (strcmp(buffer, "!\n") == 0) {
            //signal that user exits
            pthread_mutex_lock(&sendListMutex);
            exit_s_talk=true;
            pthread_cond_signal(&sendListFlag);
            pthread_mutex_unlock(&sendListMutex);
            break;
        }

        if (exit_s_talk==true)
        {
            break;
        }


        char* message = strdup(buffer);

        pthread_mutex_lock(&sendListMutex);

        List_append(sendList, message);

        //signal that sendList is not empty
        pthread_cond_signal(&sendListFlag);

        pthread_mutex_unlock(&sendListMutex);
        
    }
    pthread_cancel(pthread_self());
    return NULL;
}

void* sendMsgThread(void *arg) {

    int s;
    struct addrinfo hints, *res, *p;
    char portStr[10];
    snprintf(portStr, sizeof(portStr), "%d", otherMachinePort);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(otherMachineName, portStr, &hints, &res) != 0) {
        perror("Failed to resolve the other machine's host");
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s < 0) {
            perror("Failed to create socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        perror("FAILED TO CREATE SOCKET");
        exit(EXIT_FAILURE);
    }

    while (1) {
        
        pthread_mutex_lock(&sendListMutex);

        //stall until sendlist is not empty
        while (List_count(sendList) == 0 && exit_s_talk==false) {
            pthread_cond_wait(&sendListFlag, &sendListMutex);  
        }
        
        if (exit_s_talk==true)
        {
            break;
        }

        char *message = (char *)List_remove(sendList);

        pthread_mutex_unlock(&sendListMutex);

        size_t len = strlen(message);

        if (sendto(s, message, len, 0, p->ai_addr, p->ai_addrlen) < 0) {
            perror("Message failed on send");
            exit(EXIT_FAILURE);
        }
    }

    freeaddrinfo(res);
    close(s);
    
    pthread_cancel(pthread_self());
}


//This function receives the messages sent to it 
void *getMsgThread(void *arg) {
    int myPort = *(int *)arg;
    int s;
    struct sockaddr_in addr, clientAddr; //socket address structure
    char buffer[1024]; //buffer for messages
    socklen_t addrLen = sizeof(clientAddr); //client address length

    //create socket
    s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s < 0) {
        perror("Socket failed on creation");
        exit(EXIT_FAILURE);
    }

    //adress structure config
    addr.sin_family = AF_INET;
    addr.sin_port = htons(myPort);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&addr.sin_zero, '\0', 8);

    //bind socket to local address
    if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    //keep running until terminated
    while(1) {
        //check cancel flag
        pthread_testcancel();

        if (exit_s_talk){
            break;
        }
        
        ssize_t receivedBytes = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrLen);

        if (receivedBytes < 0) {
            perror("Failed to receive message");
            exit(EXIT_FAILURE);
        }

        buffer[receivedBytes] = '\0';
        if (exit_s_talk){
            break;
        }
        if (strcmp(buffer, "!\n") == 0) {
            break;
        }


        char* message = strdup(buffer);

        pthread_mutex_lock(&receiveListMutex);

        List_append(receiveList, message);

        //signal that receive list is non-empty
        pthread_cond_signal(&receiveListFlag);

        pthread_mutex_unlock(&receiveListMutex);
        
        //check cancel flag
        pthread_testcancel();
    }

    close(s); //close socket
    pthread_cancel(pthread_self());
}

void *screenOutputThread(void *arg) {
    while (1) {
        //check cancel flag
        pthread_testcancel();

        if (exit_s_talk==true)
        {   
            break;
        }

        pthread_mutex_lock(&receiveListMutex);

        //stall until receive list non-empty
        while (List_count(receiveList) == 0 && exit_s_talk==false) {
            pthread_cond_wait(&receiveListFlag, &receiveListMutex);  
        }

        if (exit_s_talk==true)
        {   
            break;
        }

        char *message = (char *)List_first(receiveList);

        pthread_mutex_unlock(&receiveListMutex);

        if (message != NULL) {
            fputs("Received message: ", stdout);
            fputs(message, stdout);

            pthread_mutex_lock(&receiveListMutex);

            List_remove(receiveList);
            //signal receive list non-empty
            pthread_cond_signal(&receiveListFlag);

            pthread_mutex_unlock(&receiveListMutex);
        }
        pthread_testcancel();
    }
    
    pthread_exit(NULL);
}

//function to free the dynamically allocated items in the lists
void freeItems(void*pItem)
{
    if(pItem!=NULL)
    {
        free(pItem);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        fprintf(stderr, "Correct Format is: %s [my port number] [remote machine name] [remote port number]\n", argv[0]);
        return 1;  // return an error code
    }

    myPort = atoi(argv[1]);
    otherMachineName = argv[2];
    otherMachinePort = atoi(argv[3]);

    printf("My Port: %d\n", myPort);
    printf("Remote Machine: %s\n", otherMachineName);
    printf("Remote Port: %d\n", otherMachinePort);

    //generate list instances

    sendList = List_create();
    receiveList = List_create();

    //declare and generate threads

    pthread_t keyInputThreadId, sendMsgThreadId, getMsgThreadId, screenOutputThreadId;

    if (pthread_create(&keyInputThreadId, NULL, keyInputThread, NULL) != 0) {
        perror("Failed to create the keyInputThread");
        return 1;
    }

    if (pthread_create(&sendMsgThreadId, NULL, sendMsgThread, &otherMachinePort) != 0) {
        perror("Failed to create the sendMsgThread");
        return 1;
    }

    if (pthread_create(&getMsgThreadId, NULL, getMsgThread, &myPort) != 0) {
        perror("Failed to create the getMsgThread");
        return 1;
    }

    if (pthread_create(&screenOutputThreadId, NULL, screenOutputThread, NULL) != 0) {
        perror("Failed to create screenOutputThread");
        exit(EXIT_FAILURE);
    }

    // allow threads to complete
    pthread_join(keyInputThreadId, NULL);
    pthread_join(sendMsgThreadId, NULL);

    // clean up the lists
    List_free(sendList, freeItems);
    List_free(receiveList, freeItems);

    pthread_cancel(keyInputThreadId);
    pthread_cancel(sendMsgThreadId);
    pthread_cancel(getMsgThreadId);
    pthread_cancel(screenOutputThreadId);

    return 0;
}


