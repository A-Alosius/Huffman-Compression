#include "huffman.h"
#include "helpers/structures.h"
#include "helpers/helpers.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv){
    if (argc != 3){
        printf("Wrong number of inputs\n");
        exit(1);
    }

    if (argv == NULL){
        printf("Invalid  argv\n");
        exit(1);
    }

    // open source file and output file
    int in_fd = open(argv[1], O_RDONLY);
    int out_fd = open(argv[2], O_WRONLY | O_CREAT, S_IRWXU | S_IRUSR);

    if (in_fd < 0 || out_fd < 0){
        perror("File opening failed: ");
        exit(1);
    }

    extract(in_fd, out_fd);

    // clean up
    close(in_fd);
    close(out_fd);
    
    return 0;
}