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
    printf("Client opens file\n");
    filefd = open(filePath, O_RDONLY);
    if (filefd < 0) {
        perror("Can't open file");
        exit(1);
    }

    // Calculating file size
    printf("Client calculates file size\n");
    struct stat st; 
    retVal = fstat(filefd, &st);
    if (retVal != 0) {
        perror("Can't read file size with fstat");
        exit(1);
    }
    fileSize = st.st_size;

    // Creating buffer for file content
    printf("Client creates buffer for file content\n");
    fileBuffer = (char *)calloc(fileSize+1, sizeof(char));
    if (fileBuffer == NULL) {
        perror("Can't allocate memory for buffer");
        exit(1);
    }
    
    // Reading file content to buffer
    printf("Client reads file content to buffer\n");
    retVal = read(filefd, fileBuffer, fileSize);
    if (retVal != fileSize) {
        perror("Can't read from file to buffer");
        exit(1);
    }

    // Creating socket 
    printf("Client crates socket\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Can't create socket");
        exit(1);
    }

    // Creating server address struct
    printf("Client creating server address struct\n");
	inet_pton(AF_INET, serverIP, &serv_addr.sin_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort); // sets port from args, Note: htons for endiannes
    // TODO understand if need binary

    // Connecting to server
    printf("Client connects to server\n");    
    retVal = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (retVal < 0) {
        perror("Can't connect to server");
        exit(1);
    }

    // Sending file size
    printf("Client sends file size\n");
    networkFileSize = htonl(fileSize);
    retVal = write(sockfd, &networkFileSize, sizeof(uint32_t));
    if (retVal != sizeof(uint32_t)) {
        perror("Couldn't write file size to socket");
        exit(1);
    }

    // Sending file content
    printf("Client sends file content\n");  
    retVal = write(sockfd, fileBuffer, fileSize);
    if (retVal != fileSize) {
        perror("Couldn't write all file content to socket");
        exit(1);
    }
    
    // Recieving number of printable characters
    printf("Client recieves number\n");  
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
    printf("Client closes sockets\n");  
    retVal = close(sockfd);
    if (retVal != 0) {
        perror("Can't close socket");
        exit(1);
    }

    // Closing file
    printf("Client closes file\n");  
    retVal = close(filefd);
    if (retVal != 0) {
        perror("Can't close file");
        exit(1);
    }
}