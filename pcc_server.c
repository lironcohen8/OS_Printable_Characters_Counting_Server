#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_OF_PRINTABLE_CHARS   (95)
#define LISTEN_QUEUE_SIZE        (10)

uint16_t serverPort;
uint32_t pcc_total[NUM_OF_PRINTABLE_CHARS];
struct sockaddr_in serv_addr, client_addr;
int listenfd, connfd, fileSize, printableCounter;
char *fileBuffer;
socklen_t addrsize = sizeof(struct sockaddr_in);

// Preparing SIGINT handler
int prepare_handler(void) {
    struct sigaction sigchld_action; // struct of sigaction to pass to registration
	memset(&sigchld_action, 0, sizeof(sigchld_action)); // setting sigaction mem to 0
	sigchld_action.sa_sigaction = sigint_handler; // setting handler to my function
	sigchld_action.sa_flags = SA_RESTART | SA_SIGINFO; // including the info
	if (sigaction(SIGINT, &sigchld_action, NULL) != 0) { // registering handler
		perror("Error in SIGINT handler registration");
		return 1;
	}
	return 0;
}

// handler for SIGINT
void sigint_handler(int signum, siginfo_t* info, void *ptr) { 
	int i;
    for (i = 0; i < NUM_OF_PRINTABLE_CHARS; i++) {
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int retVal, i, charValue;

    // Checking number of arguments
    if (argc != 2) {
        perror("Number of cmd args is not 1");
	    exit(1);
    }

    // Parsing arguments
    sscanf(argv[1],"%hu",&serverPort);

    // Initializing pcc_total
    memset(&pcc_total, 0, sizeof(uint32_t)*NUM_OF_PRINTABLE_CHARS);

    // Creating socket 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Can't create socket");
        exit(1);
    }

    // Enabling port reuse
    // TODO understand if needed in connfd
    retVal = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int);
    if (retVal < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // Creating server address struct
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serverPort);

    // Binding socket to port
    retVal = bind(listenfd, (struct sockaddr*) &serv_addr, addrsize);
    if (retVal != 0) {
        perror("Bind failed");
        exit(1);
    }

    // Listening to socket
    retVal = listen(serverPort, LISTEN_QUEUE_SIZE);
    if (retVal != 0) {
        perror("Listen failed");
        exit(1);
    }

    while (1) {
        // Accepting a connection
        connfd = accept(listenfd, (struct sockaddr*) &client_addr, &addrsize);
        if (connfd < 0) {
            perror("Accept failed");
            exit(1);
        }

        // Reading file size
        retVal = read(connfd, &fileSize, sizeof(uint32_t));
        if (retVal != sizeof(uint32_t)) {
            perror("Couldn't read file size from socket");
            exit(1);
        }
        fileSize = ntohl(fileSize);

        // Creating buffer for file content
        fileBuffer = (char *)calloc(1,fileSize+1);
        if (fileBuffer == NULL) {
            perror("Can't allocate memory for buffer");
            exit(1);
        }

        // Reading file content
        retVal = read(connfd, &fileBuffer, fileSize);
        if (retVal != fileSize) {
            perror("Couldn't read all file content from socket");
            exit(1);
        }

        // Calculating printable characters number
        printableCounter = 0;
        for (i = 0; i < fileSize; i++) {
            charValue = fileBuffer[i];
            if ((charValue >= 32) and (charValue <= 126)) {
                printableCounter++;
            }
        }

        // Writing result to client
        // TODO understand when to convert to network order
        retVal = write(connfd, &printableCounter, sizeof(uint32_t));
        if (retVal != sizeof(uint32_t)) {
            perror("Couldn't write counter result to socket");
            exit(1);
        }

        // Updating pcc_total
        for (i = 0; i < fileSize; i++) {
            charValue = fileBuffer[i];
            pcc_total[charValue-32]++;
        }

        // Closing connection socket
        retVal = close(connfd);
        if (retVal != 0) {
            perror("Can't close connection socket");
            exit(1);
        }
}