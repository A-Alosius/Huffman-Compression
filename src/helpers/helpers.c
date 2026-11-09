#include "helpers/helpers.h"
#include "huffman.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>

// swap nodes based on their index
void swap(Profile* profile, int first, int second){
    // swap frequencies
    HuffmanNode* tmp = profile->nodes[first];
    profile->nodes[first] = profile->nodes[second];
    profile->nodes[second] = tmp;
}

// shift values in the profile to close unused byte gaps
void kill_spaces(Profile* profile){
    sort_profile(profile);
    uint16_t write = 0;
    for (int i = 0; i < 256; i++){
        if (profile->nodes[i]->frequency == 0){
            free(profile->nodes[i]);
        }
        else{
            profile->nodes[write] = profile->nodes[i];
            profile->nodes[i] = (write != i) ? NULL : profile->nodes[i];
            write++;
        }
    }
    profile->size = write;
}


// update frequency for bytes in the buffer
void update_profile(unsigned char* buffer, ssize_t bytes_read, Profile* profile){
    for (int i = 0; i < bytes_read; i++){
        profile->nodes[buffer[i]]->frequency++;
    }
}


// sort profile by frequency
void sort_profile(Profile* profile){
    for (int i = 1; i < 256; i++){
        int j = i;
        while (j>0 && profile->nodes[j]->frequency < profile->nodes[j-1]->frequency){
            // swap 
            swap(profile, j, j-1);
            j--;
        }
    }
}

// compute tree height to determine max length of a code
int tree_height(HuffmanNode* tree){
    if (tree == NULL){
        printf("Invalid tree\n");
        exit(1);
    }

    int left_height = 0, right_height = 0;

    if (tree->is_leaf){
        return 0;
    }
    
    left_height  = tree_height(tree->children.left);
    right_height = tree_height(tree->children.right);
    
    return (left_height > right_height) ? 1+left_height : 1+right_height;
}

// free tree
void free_tree(HuffmanNode* tree){
    if (tree == NULL){
        printf("Null tree\n");
        exit(1);
    }
    
    if (!tree->is_leaf){
        free_tree(tree->children.left);
        free_tree(tree->children.right);
    }
    free(tree);
}

// free profile
void free_profile(Profile* profile){
    if (profile == NULL){
        printf("Null profile\n");
        exit(1);
    }
    for (int i = 0; i < profile->size; i++){
        free(profile->nodes[i]);
    }
}

// read an exact number of bytes
void read_exact(int in_fd, size_t num_bytes, void* target){
    // read original size and number of symbols
    ssize_t bytes_read;
    size_t total = 0;
    while (total < num_bytes){
        bytes_read = read(in_fd, (unsigned char*)target + total, num_bytes - total);

        if (bytes_read < 0 && errno == EINTR){
            continue;
        }

        if (bytes_read < 0) perror("Read: ");

        if (bytes_read == 0){
            printf("Unexpected end of file\n");
            exit(1);
        }

        total += bytes_read;
    }
}

// write an exact number of bytes
void write_exact(int out_fd, ssize_t *write_from, ssize_t* output_buffer_index, ssize_t *bytes_written, unsigned char (*output_buffer)[4096]){
    while ((*bytes_written = write(out_fd, &output_buffer[*write_from], 
            *output_buffer_index - *write_from)) < (*output_buffer_index - *write_from)){
            // retry if write was interrupted
            if (*bytes_written < 0 && errno == EINTR) continue;

            // exit if write failed
            if (*bytes_written <= 0){
                perror("Write: ");
                exit(1);
            }
            *write_from += *bytes_written;
        }
}