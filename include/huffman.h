/**
* Author: A-Alosius
* Huffman coding algorithm in C
*
* How it works
* 1. Receive am input file
* 2. Profile it by bytes and store the frequencies of each byte
* 3. Build huffman tree from byte frequencies
* 4. Produce output with huffman codes for the bytes 
*/

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>

typedef struct HuffmanNode HuffmanNode;
typedef struct Profile Profile;
typedef struct LUT LUT;

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (4096) // modifiable at compile time
#endif

#define TREE_SIZE (256)
#define NULL_INPUTS (-24)

// initialise profile with all leaf nodes
Profile* init_profile();

// profile the file and update frequencies through simple indexing. 
// initialise frequencies to 0 at start
void profiler(int fd, Profile* profile);

// sort bytes by frequency using insertion sort
void sort_profile(Profile* profile);

// build huffman tree from profile
HuffmanNode* build_tree(Profile* profile);

// get code for given byte from the tree
bool get_code(char byte, HuffmanNode* huffman_tree, char* code, uint8_t depth);

// create a look up table to avoid 
// searching whole tree multiple times
void create_LUT(HuffmanNode* huffman_tree, LUT* lut, uint64_t* code, uint8_t depth);

// convert buffer bytes to huffman code, 
// write codes to output buffer then to output file
void update_output_buffer(char* buffer, char *bytes, ssize_t bytes_read, HuffmanNode* huffman_tree);

// create output file
void compress(int input_fd, int output_fd, LUT* lut, Profile* profile);

// extract a compressed archive
void extract(int in_fd, int out_fd);
#endif