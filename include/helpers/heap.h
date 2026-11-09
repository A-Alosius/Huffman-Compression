#ifndef HEAP_H
#define HEAP_H

#include "huffman.h"

// create a leaf node
HuffmanNode* create_leaf_node(unsigned char index);

// create non_leaf node
HuffmanNode* create_inner_node(HuffmanNode* left, HuffmanNode* right);

// extract root of min heap
HuffmanNode* extract_root(Profile* profile);

// add node to heap and restore heap property
void add_node_to_heap(Profile* profile, HuffmanNode* node);

// restore heap property after popping root
void sift_down(Profile* profile);

// restore heap property of profile after adding node
void sift_up(Profile* profile);

#endif