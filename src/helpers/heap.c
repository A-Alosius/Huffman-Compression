#include "helpers/heap.h"
#include "helpers/structures.h"
#include "helpers/helpers.h"
#include "huffman.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>


// create a leaf node
HuffmanNode* create_leaf_node(unsigned char index){
    // allocate space for new node
    HuffmanNode* new_node = malloc(sizeof(*new_node));

    if (new_node == NULL){
        printf("Error creating node\n");
        exit(-1);
    }

    new_node->frequency = 0;
    new_node->is_leaf = true;
    new_node->byte = index;

    return new_node;
}

// create non_leaf node
 HuffmanNode* create_inner_node(HuffmanNode* left, HuffmanNode* right){
    // validate inputs
    if(left == NULL || right == NULL){
        printf("Invalid Child nodes\n");
        _exit(NULL_INPUTS); // define macro later for error diagnosis
    }

    // allocate space for new node
    HuffmanNode* new_node = malloc(sizeof(*new_node));

    if (new_node == NULL){
        printf("Error creating node\n");
        exit(-1);
    }

    new_node->frequency = left->frequency + right->frequency;
    new_node->is_leaf = false;
    new_node->children.left = left;
    new_node->children.right = right;

    return new_node;
}

// extract root of min heap
HuffmanNode* extract_root(Profile* profile){
    if (profile == NULL){
        printf("Invalid profile\n");
        exit(NULL_INPUTS); // define macro later for error diagnosis
    }

    if (profile->size == 0){
        return NULL;
    }

    HuffmanNode* tmp = profile->nodes[0];
    profile->nodes[0] = profile->nodes[profile->size-1];
    profile->nodes[profile->size-1] = NULL;
    profile->size--;

    // if size is 0 or 1, no need to sift
    if (profile->size > 1) sift_down(profile);

    return tmp;
}

// add node to heap and restore heap property
void add_node_to_heap(Profile* profile, HuffmanNode* node){
    if (profile == NULL || node == NULL){
        printf("Invalid profile or node\n");
        exit(NULL_INPUTS);
    }

    if (profile->size < TREE_SIZE){
        profile->nodes[profile->size] = node;
        profile->size++;
        sift_up(profile);
    }
    else{
        printf("Tree is full, could not add node\n");
        exit(-1);
    }
}

// restore heap property after popping root
void sift_down(Profile* profile){
    if (profile == NULL){
        printf("Invalid profile\n");
        exit(NULL_INPUTS); // define macro later for error diagnosis
    }

    int index = 0;

    while (profile->nodes[index] != NULL && profile->size > 2*index+1){
        // validate left input
        if (profile->nodes[2*index + 1] == NULL){
            printf("Invalid left child nodes\n");
            exit(NULL_INPUTS);
        }
        
        // tmp variables to for easier reading
        int64_t left_frequency, right_frequency;
        int swap_index = 0;
        
        // if no right child, focus on left
        if (2*index + 2 > profile->size-1){
            swap_index = 2*index + 1; // 
        }
        // if the index for the right child is valid, validate right child
        else if (profile->nodes[2*index+2] == NULL){
            printf("Invalid right child node\n");
            exit(NULL_INPUTS);
        } 
        // compare parent with the smaller of its two children       
        else{
            left_frequency = profile->nodes[2*index + 1]->frequency;
            right_frequency = profile->nodes[2*index + 2]->frequency;
            swap_index = (left_frequency > right_frequency) ? (2*index + 2) : (2*index + 1);
        }
        
        // compare parent and child to determine swap necessity
        if (profile->nodes[index]->frequency > profile->nodes[swap_index]->frequency){
            swap(profile, index, swap_index);
            index = swap_index;
        }
        else{
            break;
        }
    }
}

// restore heap property of profile after adding node
void sift_up(Profile* profile){
    if (profile == NULL){
        printf("Invalid profile\n");
        exit(NULL_INPUTS); // define macro later for error diagnosis
    }

    int index = profile->size - 1;

    if (index < 0){
        printf("Invalid index\n");
        exit(NULL_INPUTS); //
    }

    HuffmanNode* tmp;
    // compare node with parent and swap if less and repeat until node greater than parent
    while (index > 0 && profile->nodes[index]->frequency < profile->nodes[(index-1)/2]->frequency){
        tmp = profile->nodes[index];
        profile->nodes[index] = profile->nodes[(index-1)/2];
        profile->nodes[(index-1)/2] = tmp;
        index = (index-1)/2;
    }
}