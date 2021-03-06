#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    int filesize, i;
    FILE *fd;
    
    if (argc != 3 && argc != 4) {
        printf("Usage:\t%s filename size [byte]\n", argv[0]);
        return -1;
    }
    
    filesize = atoi(argv[2]) * 256 * 1024 + (argc == 4 ? atoi(argv[3]) : 0);

    fd = fopen(argv[1], "w");
      
    perror("fopen()");

    for (i = 0; i < filesize; ++i) {
        char tmp;
        if (i % 128 == 0) tmp = '\n';
        else tmp = rand() % 26 + 'a';
        fwrite(&tmp, 1, 1, fd);
    }
    
    fclose(fd);

    return 0;
}
