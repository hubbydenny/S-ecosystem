#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        write(2, "usage: cat <file>\n", 18);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0) {
        write(1, buf, n);
    }
    if (n == -1)
        perror("read");

    close(fd);
    return 0;
}
