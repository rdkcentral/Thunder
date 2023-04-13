#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>

int main() {
    int fd = open ("test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    printf("fd = %d, error = %s\n", fd, strerror(errno));
    int key = 0;
    system("ls -l test.txt");
    char buffer[] = "TestString";
    write(fd, buffer, sizeof(buffer));

    scanf("%d", &key);
    close(fd);
    int status = unlink("test.txt");

    return 0;
}
