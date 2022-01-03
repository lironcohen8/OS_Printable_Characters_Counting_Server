#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char**argv)
{
    int num_characters = atoi(argv[1]);
    char* data = malloc(num_characters);
    int total = 0;
    int bytes;
    int fd = open("/dev/urandom", O_RDONLY);
    while ((bytes = read(fd, data + total, num_characters - total)) > 0)
    {
        total+=bytes;
    }
    close(fd);
    total = 0;
    while ((bytes = write(STDOUT_FILENO, data + total, num_characters - total)) > 0)
    {
        total+=bytes;
    }
    free(data);
    return 0;
}