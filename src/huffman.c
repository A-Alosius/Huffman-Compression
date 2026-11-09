#include "huffman.h"
#include "helpers/structures.h"
#include "helpers/helpers.h"
#include "helpers/heap.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>


// initialise profile with all leaf nodes
Profile* init_profile(){
    // allocate memory for profile
    Profile* profile = malloc(sizeof(*profile));

    if (profile == NULL){
        printf("Error creating profile\n");
        exit(-1);
    }

    profile->size = 0;

    // init nodes for full byte range (character representations)
    for (int i = 0; i < 256; i++){
        profile->nodes[(unsigned char)i] = create_leaf_node((unsigned char)i);
    }
    return profile;
}

// scan through input file, reading bytes and update
// profiler frequencies at index of bytes.
void profiler(int fd, Profile* profile){
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    errno = 0;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0){
        update_profile(buffer, bytes_read, profile);
    }

    if (bytes_read < 0 && errno != 0){
        perror("Read: ");
        _exit(1);
    }

    // close gaps and get length of heap of valid nodes
    kill_spaces(profile);
}

// build huffman tree from smallest frequencies up
HuffmanNode* build_tree(Profile* profile){
    if (profile == NULL){
        printf("Invalid profile or tree\n");
        exit(NULL_INPUTS);
    }

    // if there's only one unique character, return tree
    // with one node
    if (profile->size == 1){
        return profile->nodes[0];
    }

    // loop through profile and create nodes as we go
    // retrieve smallest nodes in tree and build bigger
    // nodes as their parents
    while(profile->size > 1) {
        HuffmanNode* child1 = extract_root(profile);
        HuffmanNode* child2 = extract_root(profile);
        HuffmanNode* new_node = create_inner_node(child1, child2);
        add_node_to_heap(profile, new_node);
    }

    if (profile->size < 1){
        printf("something went wrong somewhere\n");
        exit(-1);
    }
    return profile->nodes[0];
}

// create code for byte from tree
// left child get 1, right gets 0
// used for testing
bool get_code(char byte, HuffmanNode *huffman_tree, char* code, uint8_t depth){
    // validate current node
    if (huffman_tree == NULL){
        printf("Invalid tree node\n");
        exit(1);
    }

    // check byte value if we are at a leaf
    if (huffman_tree->is_leaf){
        return huffman_tree->byte == byte;
    }

    // recurse down left branch and if we find the byte, return true
    if (get_code(byte, huffman_tree->children.left, code, depth+1)){
        code[depth] = '1';
        return true;
    }

    // recurse down left branch and if we find the byte, return true
    if (get_code(byte, huffman_tree->children.right, code, depth+1)){
        code[depth] = '0';
        return true;
    }
    return false; // byte not found at this leaf
}

// create a look up table to avoid 
// searching whole tree multiple times
// code is an array of 4 64 bits unsigned ints
void create_LUT(HuffmanNode* huffman_tree, LUT* lut, uint64_t* code, uint8_t depth){
    if (huffman_tree == NULL || lut == NULL){
        printf("Invalid tree or LUT\n");
        exit(1);
    }

    // check byte value if we are at a leaf
    if (huffman_tree->is_leaf){
        // if there is only one node, only one code exists
        if (depth == 0){
            lut[huffman_tree->byte].bits[0] = 0;
            lut[huffman_tree->byte].length = 1;
            return;
        }

        // write code to appropriate lut index
        for (int i = 0; i < 4; i++){
            lut[huffman_tree->byte].bits[i] = code[i];
        }
        lut[huffman_tree->byte].length = depth;
        return;
    }

    // validate children
    if (huffman_tree->children.left == NULL || huffman_tree->children.right == NULL){
        printf("Invalid left or right node\n");
        exit(1);
    }

    // code is 4 * 64 bits. We do LSB->MSB writes
    // for index i = depth/64, current decision (l or r)
    // based on whether we go left or right, is written
    // to the least significant bit since we shift by
    // depth % 64 and start at depth 0. So first bit from
    // tree is an LSB of current code index and so on
    code[depth/64] |= ((uint64_t) 1 << depth % 64);
    create_LUT(huffman_tree->children.left, lut, code, depth+1);

    code[depth/64] &= ~((uint64_t)1 << depth % 64);
    create_LUT(huffman_tree->children.right, lut, code, depth+1);
}

// run the coding character by character
void compress(int in_fd, int out_fd, LUT* lut, Profile* profile){
    if (in_fd < 0 || out_fd < 0){
        printf("Invalid input or output file\n");
        exit(1);
    }

    // will store codes until full or all chars processed
    // and then be written to output
    unsigned char output_buffer[4096] = {0};

    // read buffer
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    errno = 0;

    // check file size and write it to output header
    int64_t file_size = lseek(in_fd, 0, SEEK_END);
    if (file_size == -1){
        perror("Seek ");
        exit(1);
    }
    memcpy(&output_buffer[0], &file_size, sizeof(int64_t));

    // write number of unique entries to file
    memcpy(&output_buffer[8], &profile->size, sizeof(uint16_t));
    ssize_t output_buffer_index = 10;
    ssize_t bytes_written;

    for (int i = 0; i <= profile->size; i++){
        // if the buffer will be full before we can write the next byte and frequency
        // write it to output file and reset buffer
        if ((output_buffer_index + 9) > 4096 || i == profile->size){
            ssize_t write_from = 0;
            write_exact(out_fd, &write_from, &output_buffer_index, 
                &bytes_written, &output_buffer);
            output_buffer_index = 0;
            if (i == profile->size) break;
        }
        
        output_buffer[output_buffer_index] = profile->nodes[i]->byte;
        memcpy(&output_buffer[++output_buffer_index], &profile->nodes[i]->frequency, 8);
        output_buffer_index += 8;
    }

    /**
     * Header size will be equal to 
     * 8 bytes - original final size for decompression stop condition
     * 2 bytes - number of unique symbols
     * profile->size (256 max) * 9 bytes (1 byte symbol + 8 byte frequency)
     * This gives a max overhead of 2314 bytes
     */

    // move to start of file before compression
    off_t reset = lseek(in_fd, 0, SEEK_SET);
    if (reset == -1){
        perror("Seek ");
        exit(1);
    }

    // reset output buffer occupancy
    output_buffer_index = 0;

    // read bytes, convert to huffman code, 
    // write to output buffer then to output file
    unsigned char byte_code = 0;
    int num_bits_in_tmp = 0;
    while ((bytes_read = read(in_fd, buffer, BUFFER_SIZE))){
        // continue reading if read was interrupted
        if (bytes_read < 0 && errno == EINTR) continue;

        // exit if read failed
        if (bytes_read < 0){
            perror("Read: ");
            exit(1);
        }

        // get code for each char, write to output 
        // buffer when full or out of chars
        for (int i = 0; i < bytes_read; i++){
            uint8_t length = lut[buffer[i]].length;
            uint64_t* bits = lut[buffer[i]].bits;
            // where in the current index of each byte in the 4 64 bits code we currently are
            int num_processed_bits = 0; 

            // handle the indexes of bits that are full first
            for (int j = 0; j < length/64; j++){
                int num_bits = 64; 
                num_processed_bits = 0;
                while (num_bits > 0){
                    if ((output_buffer_index + 1) > 4096){
                        ssize_t write_from = 0;
                        // handle partial writes also
                        write_exact(out_fd, &write_from, &output_buffer_index, 
                            &bytes_written, &output_buffer);
                    }
                    
                    if (num_bits_in_tmp == 8){
                        output_buffer[output_buffer_index++] = byte_code;
                        num_bits_in_tmp = 0;
                        byte_code = 0;
                    }
                    
                    // we are using an LSB convention from the create LUT step
                    // so, we consume bits from right to left for each code index
                    // if numbits left in a code index is less than free space
                    // shift code[i] right by number of bits already consumed
                    // (num_processed_bits) AND with mast exposing the last free
                    // space bits in the code then OR by byte code
                    int free_bits = 8 - num_bits_in_tmp;
                    if (num_bits >= free_bits){
                        uint64_t mask = ((uint64_t)1 << free_bits) -1;
                        // we shift right because we process bits from right to left
                        uint64_t bits_to_process = (bits[j] >> num_processed_bits) & mask;
                        byte_code |= (unsigned char)(bits_to_process << num_bits_in_tmp);
                        num_bits -= free_bits;
                        num_processed_bits += free_bits;
                        num_bits_in_tmp = 8;
                    }

                    // if we have more free space than number of bits
                    // shift the code right by number of bits already processed
                    // & by mask exposing current bits to process 1 << numbits
                    // (remember we process from right to left)
                    // OR by byte code

                    /*THIS COULD BE A HELPER FXN WITH THE PREVIOUS IF */
                    else{
                        uint64_t mask = ((uint64_t)1 << num_bits) -1;
                        // we shift right because we process bits from right to left
                        uint64_t bits_to_process = (bits[j] >> num_processed_bits) & mask;
                        byte_code |= (unsigned char)(bits_to_process << num_bits_in_tmp);
                        num_processed_bits = 0;
                        num_bits_in_tmp += num_bits;
                        num_bits = 0;
                    }
                }
            }

            // process incomplete word or short codes
            num_processed_bits = 0;
            int num_bits = length % 64;
            int remainder_index = length / 64;

            while (num_bits > 0){
                if ((output_buffer_index + 1) > 4096){
                    ssize_t write_from = 0;

                    // handle partial writes also
                    write_exact(out_fd, &write_from, &output_buffer_index, &bytes_written, &output_buffer);
                    output_buffer_index = 0;
                }

                if (num_bits_in_tmp == 8){
                    output_buffer[output_buffer_index++] = byte_code;
                    num_bits_in_tmp = 0;
                    byte_code = 0;
                }

                int free_bits = 8 - num_bits_in_tmp;

                if (num_bits >= free_bits){
                    uint64_t mask = ((uint64_t)1 << free_bits) - 1;

                    uint64_t bits_to_process =
                        (bits[remainder_index] >> num_processed_bits) & mask;

                    byte_code |= (unsigned char)(bits_to_process << num_bits_in_tmp);

                    num_bits -= free_bits;
                    num_processed_bits += free_bits;
                    num_bits_in_tmp = 8;
                }
                else{
                    // create mask for bits to process
                    uint64_t mask = ((uint64_t)1 << num_bits) - 1;

                    // extract bits to process
                    uint64_t bits_to_process = (bits[remainder_index] >> num_processed_bits) & mask;

                    // write bits to byte code
                    byte_code |= (unsigned char)(bits_to_process << num_bits_in_tmp);
                    
                    // update relevant counts
                    num_processed_bits += num_bits;
                    num_bits_in_tmp += num_bits;
                    num_bits = 0;
                }
            }
        }
    }

     // append final full or incomplete byte
    if (num_bits_in_tmp > 0){
        if ((output_buffer_index + 1) > 4096){
            ssize_t write_from = 0;

            // handle partial writes also
            write_exact(out_fd, &write_from, &output_buffer_index, 
                &bytes_written, &output_buffer);
            output_buffer_index = 0;
        }

        // unused most-significant bits remain zero
        output_buffer[output_buffer_index++] = byte_code;
    }
    // write remaining output buffer
    if (output_buffer_index > 0){
        ssize_t write_from = 0;

        // handle partial writes also
        write_exact(out_fd, &write_from, &output_buffer_index, &bytes_written, &output_buffer);

        // reset buffer occupancy
        output_buffer_index = 0;
    }            
}

void extract(int in_fd, int out_fd){
    /**
     * Header size will be equal to 
     * 8 bytes - original final size for decompression stop condition
     * 2 bytes - number of unique symbols
     * profile->size (256 max) * 9 bytes (1 byte symbol + 8 byte frequency)
     * This gives a max overhead of 2314 bytes
     */

    int64_t original_size;
    uint16_t num_symbols;

    // read original size and number of symbols
    read_exact(in_fd, 8, &original_size);
    read_exact(in_fd, 2, &num_symbols);

    // printf("orginal size = %ld num symbols = %d\n", original_size, num_symbols);

    // validate num_symbols
    if (num_symbols > 256){
        printf("More symbols than representable\n");
        exit(1);
    }
    
    // allocate memory for profile
    Profile* profile = malloc(sizeof(*profile));
    if (profile == NULL){
        perror("Malloc: ");
        exit(1);
    }
    profile->size = num_symbols;

    // input and output buffers
    unsigned char input_buffer[4096];
    unsigned char output_buffer[4096];

    // profile bytes: 1 byte for char + 
    // 8 for frequency * number of symbols
    size_t profile_bytes = 9 * num_symbols;
    read_exact(in_fd, profile_bytes, input_buffer);

    int j = 0;
    long long sum_freq = 0;

    // populate profile
    for (int i = 0; i < num_symbols; i++){
        profile->nodes[i] = malloc(sizeof(*profile->nodes[i]));

        if (profile->nodes[i] == NULL){
            perror("Malloc: ");
            free_profile(profile);
            exit(1);
        }

        profile->nodes[i]->is_leaf = true;
        profile->nodes[i]->byte = input_buffer[j++];
        memcpy(&profile->nodes[i]->frequency, &input_buffer[j], sizeof(profile->nodes[i]->frequency));
        j += sizeof(profile->nodes[i]->frequency);

        sum_freq += profile->nodes[i]->frequency;
    }

    // sanity check to ensure the header has all correct information
    if (original_size < 0 || sum_freq != original_size){
        printf("Error: File size changed\n");
        free_profile(profile);
        exit(1);
    }

    // rebuild huffman tree
    HuffmanNode* huffman_tree = build_tree(profile);

    // validate tree
    if (huffman_tree == NULL){
        printf("Failed to create tree\n");
        exit(1);
    }

    // copy tree pointer for traversal. This pointer will
    // be reset to the tree root each time a leaf is found
    HuffmanNode* tree_copy = huffman_tree;

    // useful variables
    ssize_t bytes_read;
    ssize_t output_buffer_index = 0;
    long long decoded_bytes = 0;
    ssize_t bytes_written;

    // read bytes and traverse tree to find their bytes
    while ((bytes_read = read(in_fd, input_buffer, BUFFER_SIZE))){
        // continue reading if read was interrupted
        if (bytes_read < 0 && errno == EINTR) continue;

        // exit if read failed
        if (bytes_read < 0){
            perror("Read: ");
            exit(1);
        }
    
        // traverse tree for current rad bytes
        for (int i = 0; i < bytes_read; i++){
            // progressively extract LSB and 
            // move left or right based on value
            for (int k = 0; k < 8; k++){
                int bit = input_buffer[i] & (1 <<k);
                
                // if output buffer is full, flush to ouput
                // and reset index
                if (output_buffer_index == 4096){
                    ssize_t write_from = 0;
                    write_exact(out_fd, &write_from, &output_buffer_index, &bytes_written, &output_buffer);
                    output_buffer_index = 0;
                }

                // move down tree
                // if bit is 1, go left, else right
                if (bit){
                    if (tree_copy->children.left == NULL){
                        printf("Invalid left child\n");
                        exit(1);
                    }
                    tree_copy = tree_copy->children.left;
                    if (tree_copy->is_leaf){
                        output_buffer[output_buffer_index++] = tree_copy->byte;
                        printf("Byte: %c\n", tree_copy->byte);
                        tree_copy = huffman_tree;
                        decoded_bytes++;
                        // stop if full file size reached
                        if (decoded_bytes == original_size){
                            printf("extraction complete\n");
                            break;
                        }
                    }
                }
                else{
                    // if there was only one symbol
                    if (tree_copy->is_leaf){
                        output_buffer[output_buffer_index++] = tree_copy->byte;
                        decoded_bytes++;
                        if (decoded_bytes == original_size) break; 
                        continue;
                    }

                    if (tree_copy->children.right == NULL){
                        printf("Invalid right child\n");
                        exit(1);
                    }
                    tree_copy = tree_copy->children.right;
                    if (tree_copy->is_leaf){
                        output_buffer[output_buffer_index++] = tree_copy->byte;
                        printf("Byte: %c\n", tree_copy->byte);
                        tree_copy = huffman_tree;
                        decoded_bytes++;
                        // stop if full file size reached
                        if (decoded_bytes == original_size){
                            printf("extraction complete\n");
                            break;
                        }
                    }
                }  
            }

        }
    }

    // flush any remaining bits from output buffer
    if (output_buffer_index > 0){
        ssize_t write_from = 0;
        write_exact(out_fd, &write_from, &output_buffer_index, &bytes_written, &output_buffer);
    }
    free_tree(huffman_tree);
    free(profile);
}