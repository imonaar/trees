#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct Node Node;
typedef struct Leaf Leaf;
typedef struct Directory Directory;
typedef struct Tree Tree;

#define TREE_TAG_ROOT 0x01 /* 0000 0001 */
#define TREE_TAG_NODE 0x02 /* 0000 0010 */
#define TREE_TAG_LEAF 0x04 /* 0000 0100 */

struct Node
{
    char name[256];
    Node *parent;
    uint8_t tag;
};

struct Leaf
{
    Node base;
    Leaf *next_leaf; // Links to next file in same directory
    void *value;
    uint16_t size;
};

// Directory node with separate lists for files and subdirectories
struct Directory
{
    Node base;
    Directory *next_dir;    // Links to next sibling directory
    Leaf *first_leaf;       // Links to first file
    Directory *first_child; // Links to first subdirectory
    uint16_t dir_count;     // Number of subdirectories
    uint32_t total_size;    // Total size of all contents
};

struct Tree
{
    Directory *root;
    uint32_t total_dirs;
    uint32_t total_size;
    void (*destroy)(void *);
    int (*compare)(void *, void *);
};

// Tree initialization and cleanup
void init_tree(Tree *tree, int (*compare)(void *key1, void *key2), void (*destroy)(void *data));
void destroy_tree(Tree *tree);

// Directory operations
Directory *create_directory(Tree *tree, Directory *parent, const char *name);
int remove_directory(Tree *tree, Directory *dir);
Directory *find_directory(Tree *tree, Directory *start, const char *name);
Directory *get_parent_directory(Directory *dir);

// File (Leaf) operations
Leaf *create_leaf(Tree *tree, Directory *parent, const char *name, void *value, uint16_t size);
int remove_leaf(Tree *tree, Leaf *leaf);
Leaf *find_leaf(Tree *tree, Directory *start, const char *name);

// Tree traversal and information
void traverse_tree(Tree *tree, void (*callback)(Node *node, void *user_data), void *user_data);
uint32_t get_directory_size(Directory *dir);
uint32_t get_total_size(Tree *tree);
uint16_t get_directory_count(Directory *dir);
uint32_t get_total_directories(Tree *tree);
uint32_t get_total_files(Tree *tree);

// Path-based operations
Node *get_node_by_path(Tree *tree, const char *path);
char *get_node_path(Node *node);

// Utility functions
bool is_directory(Node *node);
bool is_leaf(Node *node);
const char *get_node_name(Node *node);
void print_tree(Tree *tree);

#endif