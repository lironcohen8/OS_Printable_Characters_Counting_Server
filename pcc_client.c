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

char *filePath, *serverIP, *fileBuffer;
uint16_t serverPort;
uint32_t networkFileSize, networkPrintableCharsCount, networkServerIP, fileSize;
struct sockaddr_in serv_addr;
int filefd, sockfd, printableCharsCount, bytesRead = 0, bytesCurrRead = 0, bytesWritten = 0, bytesCurrWrite = 0;

int main(int argc, char *argv[]) {
    int retVal;

    // Checking number of arguments
    if (argc != 4) {
        perror("Number of cmd args is not 3");
	    exit(1);
    }

    // Parsing arguments
    serverIP = argv[1];   
    sscanf(argv[2],"%hu",&serverPort);
    filePath = argv[3];

    // Opening file
    filefd = open(filePath, O_RDONLY);
    if (filefd < 0) {
        perror("Can't open file");
        exit(1);
    }

    // Calculating file size
    struct stat st; 
    retVal = fstat(filefd, &st);
    if (retVal != 0) {
        perror("Can't read file size with fstat");
        exit(1);
    }
    fileSize = (uint32_t)st.st_size;

    // Creating buffer for file content
    fileBuffer = (char *)calloc(fileSize+1, sizeof(char));
    if (fileBuffer == NULL) {
        perror("Can't allocate memory for buffer");
        exit(1);
    }
    
    // Reading file content to buffer
    retVal = read(filefd, fileBuffer, fileSize);
    if (retVal != fileSize) {
        perror("Can't read from file to buffer");
        exit(1);
    }

    // Creating socket 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Can't create socket");
        exit(1);
    }

    // Creating server address struct
	retVal = inet_pton(AF_INET, serverIP, &(serv_addr.sin_addr));
    if (retVal != 1) {
        perror("Can't convert server IP address to binary");
        exit(1);
    }
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);

    // Connecting to server
    retVal = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (retVal < 0) {
        perror("Can't connect to server");
        exit(1);
    }
    
    // Writing file size to server
    networkFileSize = htonl(fileSize);
    bytesWritten = 0;
    bytesCurrWrite = 1;
    while (bytesCurrWrite > 0) {
        bytesCurrWrite = write(sockfd, (&networkFileSize)+bytesWritten, 4-bytesWritten);
        bytesWritten += bytesCurrWrite;
    }
    if (bytesCurrWrite < 0 || bytesWritten != 4) {
        perror("Couldn't write file size to socket");
        exit(1);
    }

    // Writing file content to server
    bytesWritten = 0;
    bytesCurrWrite = 1;
    while (bytesCurrWrite > 0) {
        bytesCurrWrite = write(sockfd, fileBuffer+bytesWritten, fileSize-bytesWritten);
        bytesWritten += bytesCurrWrite;
    }
    if (bytesCurrWrite < 0 || bytesWritten != fileSize) {
        perror("Couldn't write file content to socket");
        exit(1);
    }
    
    // Recieving number of printable characters
    bytesRead = 0;
    bytesCurrRead = 1;
    while (bytesCurrRead > 0) {
        bytesCurrRead = read(sockfd, &(networkPrintableCharsCount)+bytesRead, 4-bytesRead);
        bytesRead += bytesCurrRead;
    }
    if (bytesCurrRead < 0 || bytesRead != 4) {
        perror("Couldn't read number of printable characters from socket");
        exit(1);
    }
    printableCharsCount = ntohl(networkPrintableCharsCount);

    // Printing number
    printf("# of printable characters: %u\n", printableCharsCount);

    // Closing socket
    close(sockfd);

    // Closing file
    close(filefd);

    exit(0);
}