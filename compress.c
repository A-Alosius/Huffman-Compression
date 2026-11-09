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

void print_LUT(LUT* lut){
    if (lut == NULL){
        printf("Invalid LUT\n");
        return;
    }

    for (int byte = 0; byte < 256; byte++){
        if (lut[byte].length == 0){
            continue;
        }

        if (byte >= 32 && byte <= 126){
            printf("'%c' (%d): ", byte, byte);
        }
        else{
            printf("0x%02X (%d): ", byte, byte);
        }

        for (int bit = 0; bit < lut[byte].length; bit++){
            int word_index = bit / 64;
            int bit_index = bit % 64;

            uint64_t value =
                (lut[byte].bits[word_index] >> bit_index) & 1ULL;

            printf("%llu", (unsigned long long)value);
        }

        printf("  length=%u\n", (unsigned int)lut[byte].length);
    }
}

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

    // initialise profile for file parsing
    Profile* profile = init_profile();

    // run profiling
    profiler(in_fd, profile);

    // second profile to preserve bytes and frequencies
    // for writing to output file
    Profile profile_copy;
    for (int i = 0; i < profile->size; i++){
        profile_copy.nodes[i] = malloc(sizeof(*profile_copy.nodes[i]));
        profile_copy.nodes[i]->is_leaf = true;
        profile_copy.nodes[i]->frequency = profile->nodes[i]->frequency;
        profile_copy.nodes[i]->byte = profile->nodes[i]->byte;
    }
    profile_copy.size = profile->size;

    // create huffman tree from profile
    HuffmanNode* huffman_tree = build_tree(profile);

    // allocate and build Look up table from tree
    LUT* lut = malloc(sizeof(*lut)*256);
    
    // initialise lut lengths to 0
    for (int i = 0; i <256; i++){
        lut[i].length = 0;
    }

    uint64_t* code = malloc(sizeof(*code)*4);
    create_LUT(huffman_tree, lut, code, 0);

    compress(in_fd, out_fd, lut, &profile_copy);
    // extract(in_fd, out_fd);

    // clean up
    close(in_fd);
    close(out_fd);
    free(lut); 
    free(code);
    free_tree(huffman_tree);
    free_profile(&profile_copy);
    free(profile);
    
    return 0;
}