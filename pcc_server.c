#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_OF_PRINTABLE_CHARS   (95)
#define LISTEN_QUEUE_SIZE        (10)

uint16_t serverPort;
uint32_t pcc_total[NUM_OF_PRINTABLE_CHARS], networkPrintableCounter, networkFileSize;
struct sockaddr_in serv_addr, client_addr;
int listenfd = -1, connfd = -1, fileSize, printableCounter = 0, bytesRead = 0, bytesCurrRead = 0;
int bytesWritten = 0, bytesCurrWrite = 0, sigintFlag = 0;
char *fileBuffer;
socklen_t addrsize = sizeof(struct sockaddr_in);

// printing results and closing server
void finish() {
    int i;
    for (i = 0; i < NUM_OF_PRINTABLE_CHARS; i++) {
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
    // Closing listening socket
    close(listenfd);
    exit(0);
}

// handler for SIGINT
void sigint_handler(int signum, siginfo_t* info, void *ptr) { 
	// No connection to continue handling, printing result and closing server
    if (connfd < 0) {
        finish();
    }
    // Connection is in progress, finishing it and then calling finish()
    else {
        sigintFlag = 1;
    }
}

// Preparing SIGINT handler
int prepare_handler(void) {
    struct sigaction sigint_action; // struct of sigaction to pass to registration
	memset(&sigint_action, 0, sizeof(sigint_action)); // setting sigaction mem to 0
	sigint_action.sa_sigaction = sigint_handler; // setting handler to my function
	sigint_action.sa_flags = SA_SIGINFO; // including the info
	if (sigaction(SIGINT, &sigint_action, NULL) != 0) { // registering handler
		perror("Error in SIGINT handler registration");
		return 1;
	}
	return 0;
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
    memset(&pcc_total, 0, 4*NUM_OF_PRINTABLE_CHARS);

    // Preparing SIGINT handler
    retVal = prepare_handler();
    if (retVal != 0) {
        perror("Can't prepare SIGINT handler");
    }

    // Creating socket 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Can't create socket");
        exit(1);
    }

    // Enabling port reuse
    retVal = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
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
    retVal = listen(listenfd, LISTEN_QUEUE_SIZE);
    if (retVal != 0) {
        perror("Listen failed");
        exit(1);
    }

    while (1) {
        // Checking if we should finish
        if (sigintFlag == 1) {
            finish();
        }

        // Accepting a connection
        connfd = accept(listenfd, (struct sockaddr*) &client_addr, &addrsize);
        if (connfd < 0) {
            if (errno != EINTR) {
                perror("Accept failed");
                exit(1);
            }
            else {
                finish();
            }
        }

        // Reading file size (4 bytes)
        bytesRead = 0;
        bytesCurrRead = 1;
        while (bytesCurrRead > 0) {
            bytesCurrRead = read(connfd, (&networkFileSize)+bytesRead, 4-bytesRead);
            bytesRead += bytesCurrRead;
        }
        if (bytesCurrRead < 0 && errno != EINTR) {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                perror("Error while reading file size from socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
            else {
                perror("Error while reading file size from socket");
                exit(1);
            }
        }
        else { // bytesCurrRead = 0
            if (bytesRead != 4) {
                perror("Error while reading file size from socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
        }
        fileSize = ntohl(networkFileSize);

        // Creating buffer for file content
        fileBuffer = (char *)calloc(fileSize, sizeof(char));
        if (fileBuffer == NULL) {
            perror("Can't allocate memory for buffer");
            exit(1);
        }

        // Reading file content
        bytesRead = 0;
        bytesCurrRead = 1;
        while (bytesCurrRead > 0) {
            bytesCurrRead = read(connfd, fileBuffer+bytesRead, fileSize-bytesRead);
            bytesRead += bytesCurrRead;
        }
        if (bytesCurrRead < 0 && errno != EINTR) {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                perror("Error while reading file content from socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
            else {
                perror("Error while reading file content from socket");
                exit(1);
            }
        }
        else { // bytesCurrRead = 0
            if (bytesRead != fileSize) {
                perror("Error while reading file content from socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
        }

        // Calculating printable characters number
        printableCounter = 0;
        for (i = 0; i < fileSize; i++) {
            charValue = fileBuffer[i];
            if ((charValue >= 32) && (charValue <= 126)) {
                printableCounter++;
            }
        }

        // Writing result to client
        networkPrintableCounter = htonl(printableCounter);
        bytesWritten = 0;
        bytesCurrWrite = 1;
        while (bytesCurrWrite > 0) {
            bytesCurrWrite = write(connfd, (&networkPrintableCounter)+bytesWritten, 4-bytesWritten);
            bytesWritten += bytesCurrWrite;
        }
        if (bytesCurrWrite < 0 && errno != EINTR) {
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE) {
                perror("Error while writing counter to socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
            else {
                perror("Error while writing counter to socket");
                exit(1);
            }
        }
        else { // bytesCurrWrite = 0
            if (bytesWritten != 4) {
                perror("Error while writing counter to socket, continuing to next connection");
                close(connfd);
                connfd = -1;
                continue;
            }
        }

        // Updating pcc_total
        for (i = 0; i < fileSize; i++) {
            charValue = fileBuffer[i];
            if ((charValue >= 32) && (charValue <= 126)) {
                pcc_total[charValue-32]++;
            }
        }

        // Closing connection socket
        close(connfd);
        connfd = -1;
    }
}