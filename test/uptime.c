#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int fd;
    char buffer[128];
    ssize_t n;

    fd = open("/proc/uptime", O_RDONLY);

    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    n = read(fd, buffer, sizeof(buffer) - 1);

    if (n < 0) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';

    printf("%s\n", buffer);

    close(fd);

    return 0;
}