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
int filefd, sockfd, printableCharsCount;

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
    
    // Sending file size
    networkFileSize = htonl(fileSize);
    retVal = write(sockfd, &networkFileSize, sizeof(uint32_t));
    if (retVal != sizeof(uint32_t)) {
        perror("Couldn't write file size to socket");
        exit(1);
    }
    
    // Sending file content
    retVal = write(sockfd, fileBuffer, fileSize);
    if (retVal != fileSize) {
        perror("Couldn't write all file content to socket");
        exit(1);
    }
    
    // Recieving number of printable characters
    retVal = read(sockfd, &networkPrintableCharsCount, sizeof(uint32_t));
    if (retVal != sizeof(uint32_t)) {
        perror("Couldn't read number of printable characters from socket");
        exit(1);
    }
    printableCharsCount = ntohl(networkPrintableCharsCount);

    // Printing number
    printf("# of printable characters: %u\n", printableCharsCount);

    // Closing socket
    retVal = close(sockfd);
    if (retVal != 0) {
        perror("Can't close socket");
        exit(1);
    }

    // Closing file
    retVal = close(filefd);
    if (retVal != 0) {
        perror("Can't close file");
        exit(1);
    }

    exit(0);
}