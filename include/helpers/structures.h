#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "huffman.h"
#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>


// keep track of node pointers
// needs initialisation before profiling
struct Profile {
   HuffmanNode* nodes[TREE_SIZE];
   uint16_t size;
};

struct HuffmanNode {
    // node frequency. leaves have byte frequency
    // non-leaves have some of child node frequencies
    int64_t frequency;

    // useful for determining whether to check a node 
    // or go down to its children
    bool is_leaf;

    // leaf nodes don't have children so union
    // lets us choose whether we want children
    // or leaf node character
    union{
        unsigned char byte;
        
        struct{
            HuffmanNode* left;
            HuffmanNode* right;
        } children;
    };
};

// a lut element has bits and length for lookup
// an array of lut pointers will be used for lookup
struct LUT{
    uint64_t bits[4];
    uint8_t length;
};

#endif