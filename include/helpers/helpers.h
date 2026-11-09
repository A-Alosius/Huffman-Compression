#ifndef HELPERS_H
#define HELPERS_H

#include "structures.h"
#include "huffman.h"

#include <sys/types.h>
#include <unistd.h>

// swap nodes based on their index
void swap(Profile* profile, int first, int second);

// shift values in the profile to close unused byte gaps
void kill_spaces(Profile* profile);

// update frequency for bytes in the buffer
void update_profile(unsigned char* buffer, ssize_t bytes_read, Profile* profile);

// sort profile by frequency
void sort_profile(Profile* profile);

// compute tree height to determine max length of a code
int tree_height(HuffmanNode* tree);

// free tree
void free_tree(HuffmanNode* tree);

// free profile
void free_profile(Profile* profile);

// read an exact number of bytes
void read_exact(int in_fd, size_t num_bytes, void* target);

// write an exact number of bytes
void write_exact(int out_fd, ssize_t *write_from, 
                ssize_t* output_buffer_index, ssize_t *bytes_written, 
                unsigned char (*output_buffer)[4096]);

#endif