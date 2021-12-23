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
uint32_t networkFileSize, networkPrintableCharsCount, networkServerIP;
struct sockaddr_in serv_addr;
int filefd, sockfd, fileSize, printableCharsCount;

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
    fileSize = st.st_size;

    // Creating buffer for file content
    fileBuffer = (char *)calloc(1,fileSize+1);
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
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    // TODO understand if need binary
    serv_addr.sin_addr.s_addr = inet_pton(AF_INET, serverIP, &networkServerIP);

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
    retVal = write(sockfd, &fileBuffer, fileSize);
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
    exit(0);

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
}