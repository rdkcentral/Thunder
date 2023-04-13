#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main() {
    errno = 0;
    int fd = open ("test.txt", O_RDWR);
    printf("fd = %d, error = %s\n", fd, strerror(errno));
    system("ls -l test.txt");
    int key = 0;
    scanf("%d", &key);
    system("ls -l test.txt");
    char buffer[50];
    int size = read(fd, buffer, sizeof(buffer));
    printf("size = %d buffer = %s\n", size, buffer);
    int status = close(fd);
    printf("fd = %d, error = %s\n", status, strerror(errno));
    return 0;
}
