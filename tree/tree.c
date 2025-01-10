#include "tree.h"

static void destroy_directory(Tree *tree, Directory *dir);

// utility functions
uint32_t get_directory_size(Directory *dir)
{
    if (dir == NULL)
        return 0;
    return dir->total_size;
}

uint32_t get_total_size(Tree *tree)
{
    if (tree == NULL)
        return 0;
    return tree->total_size;
}

uint16_t get_directory_count(Directory *dir)
{
    if (dir == NULL)
        return 0;
    return dir->dir_count;
}

uint32_t get_total_directories(Tree *tree)
{
    if (tree == NULL)
        return 0;
    return tree->total_dirs;
}

bool is_directory(Node *node)
{
    if (node == NULL)
        return false;
    return (node->tag & (TREE_TAG_ROOT | TREE_TAG_NODE)) != 0;
}

bool is_leaf(Node *node)
{
    if (node == NULL)
        return false;
    return (node->tag & TREE_TAG_LEAF) != 0;
}

bool is_root(Node *node)
{
    if (node == NULL)
        return false;
    return (node->tag & TREE_TAG_ROOT) != 0;
}

const char *get_node_name(Node *node)
{
    if (node == NULL)
        return NULL;
    return node->name;
}

/*
 * Initializes a new tree structure with the specified comparison and destruction functions.
 *
 * Parameters:
 *   tree - Pointer to the tree structure to initialize
 *   compare - Function pointer for comparing node values (can be NULL)
 *   destroy - Function pointer for cleaning up node values (can be NULL)
 *
 * Notes:
 *   - Sets initial values for root, directory count, and total size to zero
 *   - Stores the provided function pointers for later use
 *   - Does not allocate any memory
 */
void init_tree(Tree *tree, int (*compare)(void *key1, void *key2), void (*destroy)(void *data))
{
    tree->root = NULL;
    tree->total_dirs = 0;
    tree->total_size = 0;
    tree->destroy = destroy;
    tree->compare = compare;
}

/*
 * Destroys the entire tree structure and frees all associated memory.
 *
 * Parameters:
 *   tree - Pointer to the tree structure to destroy
 *
 * Notes:
 *   - Recursively destroys all directories and their contents
 *   - Resets all tree counters to zero
 *   - Safe to call with NULL pointer
 *   - Uses destroy_directory for recursive cleanup
 */
void destroy_tree(Tree *tree)
{
    if (tree == NULL)
        return;

    destroy_directory(tree, tree->root);
    tree->root = NULL;
    tree->total_dirs = 0;
    tree->total_size = 0;
}

/*
 * Helper function that recursively destroys a directory and all its contents.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   dir - Pointer to the directory to destroy
 *
 * Notes:
 *   - Processes subdirectories first, then leaves, then the directory itself
 *   - Uses depth-first traversal to ensure proper cleanup
 *   - Calls the tree's destroy function for leaf values if provided
 *   - Handles both sibling and child relationships separately
 */
static void destroy_directory(Tree *tree, Directory *dir)
{
    if (tree == NULL || dir == NULL)
        return;

    // Process subdirectories (children)
    // The key is that next_dir links handle siblings, while recursive calls to destroy_directory handle children through first_child.

    Directory *curr_dir = dir->first_child;
    while (curr_dir != NULL)
    {
        Directory *next_dir = curr_dir->next_dir; // Save sibling link
        destroy_directory(tree, curr_dir);        // Recursively destroy this subdirectory
        curr_dir = next_dir;                      // Move to next sibling
    }

    // Then destroy all leaves (files) in this directory
    Leaf *curr_leaf = dir->first_leaf;
    while (curr_leaf != NULL)
    {
        Leaf *next_leaf = curr_leaf->next_leaf;
        // call destroy function on leaf if provides
        if (tree->destroy != NULL && curr_leaf->value != NULL)
            tree->destroy(curr_leaf->value);
        free(curr_leaf);
        curr_leaf = next_leaf;
    }
    // Finally, destroy the directory itself
    free(dir);
}

/*
 * Creates a new directory node and adds it to the specified parent directory.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   parent - Pointer to the parent directory where the new directory will be added
 *   name - Name of the new directory (must be less than 256 characters)
 *
 * Returns:
 *   Directory* - Pointer to the newly created directory on success
 *               Returns NULL if:
 *               - Any input parameter is NULL
 *               - Name is too long (>= 256 chars)
 *               - A directory with the same name exists in the parent
 *               - Memory allocation fails
 *
 * Notes:
 *   - Initializes the new directory and sets its parent
 *   - Updates the total directory count in the tree
 *   - If the parent is NULL, the new directory becomes the root
 *   - Ensures that no duplicate directory names exist within the same parent
 */
Directory *create_directory(Tree *tree, Directory *parent, const char *name)
{
    if (tree == NULL || name == NULL || strlen(name) >= 256)
        return NULL;

    Directory *new_dir = (Directory *)malloc(sizeof(Directory));
    if (new_dir == NULL)
        return NULL;

    // Initialize the new directory
    memset(new_dir, 0, sizeof(Directory));
    strncpy(new_dir->base.name, name, 255);
    new_dir->base.parent = (Node *)parent;

    // If this is the root directory
    if (parent == NULL)
    {
        if (tree->root != NULL)
        {
            free(new_dir);
            return NULL; // Root already exists
        }
        new_dir->base.tag = TREE_TAG_ROOT;
        tree->root = new_dir;
    }
    else
    {
        new_dir->base.tag = TREE_TAG_NODE;
        // Add to parent's children list
        if (parent->first_child == NULL)
        {
            parent->first_child = new_dir;
        }
        else
        {
            // Add to end of sibling list
            Directory *curr = parent->first_child;
            while (curr->next_dir != NULL)
            {
                if (strcmp(curr->base.name, name) == 0)
                {
                    free(new_dir);
                    return NULL; // Directory with same name exists
                }
                curr = curr->next_dir;
            }
            if (strcmp(curr->base.name, name) == 0)
            {
                free(new_dir);
                return NULL;
            }
            curr->next_dir = new_dir;
        }
        parent->dir_count++;
    }

    tree->total_dirs++;
    return new_dir;
}

/*
 * Removes a directory from the tree and its contents.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   dir - Pointer to the directory to be removed
 *
 * Returns:
 *   int - 0 on success, -1 if the tree or directory is NULL, or if the directory is the root with children
 *
 * Notes:
 *   - Cannot remove the root directory if it has children or leaves
 *   - Updates the parent's child list and directory count
 *   - Recursively destroys the directory and all its contents
 *   - Decreases the total directory count in the tree
 */
int remove_directory(Tree *tree, Directory *dir)
{
    if (tree == NULL || dir == NULL)
        return -1;

    // Cannot remove root if it has children
    if (dir == tree->root && (dir->first_child != NULL || dir->first_leaf != NULL))
        return -1;

    // Find parent and remove from parent's list
    Directory *parent = (Directory *)dir->base.parent;
    if (parent != NULL)
    {
        if (parent->first_child == dir)
        {
            parent->first_child = dir->next_dir;
        }
        else
        {
            Directory *curr = parent->first_child;
            while (curr != NULL && curr->next_dir != dir)
                curr = curr->next_dir;
            if (curr != NULL)
                curr->next_dir = dir->next_dir;
        }
        parent->dir_count--;
    }

    // Recursively destroy the directory and its contents
    destroy_directory(tree, dir);
    tree->total_dirs--;

    return 0;
}

/*
 * Recursively searches for a directory in the tree or subtree.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   start - Pointer to the directory from which to start the search
 *   name - The name of the directory to search for
 *
 * Returns:
 *   Directory* - Pointer to the found directory if it exists
 *               Returns NULL if the tree is NULL, name is NULL, or if the directory is not found
 *
 * Notes:
 *   - If 'start' is NULL, the search begins from the root of the tree
 *   - First checks if the current directory matches the search name
 *   - If not found, it recursively searches in all child directories
 *   - The search is case-sensitive
 */
Directory *find_directory(Tree *tree, Directory *start, const char *name)
{
    if (tree == NULL || name == NULL)
        return NULL;

    // If start is NULL, begin from root
    Directory *curr = start ? start : tree->root;

    // Check if current directory matches
    if (strcmp(curr->base.name, name) == 0)
        return curr;

    // Search in children
    Directory *child = curr->first_child;
    while (child != NULL)
    {
        Directory *result = find_directory(tree, child, name);
        if (result != NULL)
            return result;
        child = child->next_dir;
    }

    return NULL;
}

Directory *get_parent_directory(Directory *dir)
{
    if (dir == NULL || dir->base.parent == NULL)
        return NULL;
    return (Directory *)dir->base.parent;
}

/*
 * Creates a new leaf (file) node and adds it to the specified parent directory.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   parent - Pointer to the parent directory where the leaf will be added
 *   name - Name of the new leaf (must be less than 256 characters)
 *   value - Pointer to the leaf's data value
 *   size - Size of the leaf in bytes
 *
 * Returns:
 *   Leaf* - Pointer to the newly created leaf on success
 *          Returns NULL if:
 *          - Any input parameter is NULL
 *          - Name is too long (>= 256 chars)
 *          - A file with the same name exists in parent
 *          - Memory allocation fails
 *
 * Notes:
 *   - Updates total size of parent directory and all ancestors
 *   - Adds leaf to end of parent's leaf list
 *   - Initializes all leaf fields including base node properties
 *   - Name is copied into leaf with null termination
 */
Leaf *create_leaf(Tree *tree, Directory *parent, const char *name, void *value, uint16_t size)
{
    if (tree == NULL || parent == NULL || name == NULL || strlen(name) >= 256)
        return NULL;

    // Check if a file with the same name already exists in this directory
    Leaf *curr = parent->first_leaf;
    while (curr != NULL)
    {
        if (strcmp(curr->base.name, name) == 0)
            return NULL; // File already exists
        curr = curr->next_leaf;
    }

    // Create new leaf
    Leaf *new_leaf = (Leaf *)malloc(sizeof(Leaf));
    if (new_leaf == NULL)
        return NULL;

    // Initialize the leaf
    memset(new_leaf, 0, sizeof(Leaf));
    strncpy(new_leaf->base.name, name, 255);
    new_leaf->base.parent = (Node *)parent;
    new_leaf->base.tag = TREE_TAG_LEAF;
    new_leaf->value = value;
    new_leaf->size = size;

    // Add to parent's leaf list
    if (parent->first_leaf == NULL)
    {
        parent->first_leaf = new_leaf;
    }
    else
    {
        // Add to end of leaf list
        curr = parent->first_leaf;
        while (curr->next_leaf != NULL)
            curr = curr->next_leaf;
        curr->next_leaf = new_leaf;
    }

    // Update size totals
    parent->total_size += size;
    Directory *dir = parent;
    while (dir->base.parent != NULL)
    {
        dir = (Directory *)dir->base.parent;
        dir->total_size += size;
    }
    tree->total_size += size;

    return new_leaf;
}

/*
 * Removes a leaf (file) from the tree and updates the necessary size totals.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   leaf - Pointer to the leaf (file) to be removed
 *
 * Returns:
 *   int - 0 on success, -1 if the tree or leaf is NULL, or if the leaf has no parent
 *
 * Notes:
 *   - The function updates the total size of the parent directory and the tree
 *   - If the leaf has a value and a destroy function is provided, it calls the destroy function on the value
 *   - The leaf is freed after removal from the parent's list
 */
int remove_leaf(Tree *tree, Leaf *leaf)
{
    if (tree == NULL || leaf == NULL)
        return -1;

    Directory *parent = (Directory *)leaf->base.parent;
    if (parent == NULL)
        return -1;

    // Remove from parent's leaf list
    if (parent->first_leaf == leaf)
    {
        parent->first_leaf = leaf->next_leaf;
    }
    else
    {
        Leaf *curr = parent->first_leaf;
        while (curr != NULL && curr->next_leaf != leaf)
            curr = curr->next_leaf;
        if (curr != NULL)
            curr->next_leaf = leaf->next_leaf;
    }

    // Update size totals
    parent->total_size -= leaf->size;
    Directory *dir = parent;
    while (dir->base.parent != NULL)
    {
        dir = (Directory *)dir->base.parent;
        dir->total_size -= leaf->size;
    }
    tree->total_size -= leaf->size;

    // Clean up leaf data
    if (tree->destroy != NULL && leaf->value != NULL)
        tree->destroy(leaf->value);

    free(leaf);
    return 0;
}

/*
 * Recursively searches for a leaf (file) in the tree or subtree.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *   start - Pointer to the directory from which to start the search
 *   name - The name of the leaf to search for
 *
 * Returns:
 *   Leaf* - Pointer to the found leaf if it exists
 *           Returns NULL if the tree is NULL, name is NULL, or if the leaf is not found
 *
 * Notes:
 *   - If 'start' is NULL, the search begins from the root of the tree
 *   - First searches in the leaves of the current directory
 *   - If not found, it recursively searches in all child directories
 *   - The search is case-sensitive
 */
Leaf *find_leaf(Tree *tree, Directory *start, const char *name)
{
    if (tree == NULL || name == NULL)
        return NULL;

    Directory *curr = start ? start : tree->root;
    if (curr == NULL)
        return NULL;

    // First search in current directory's leaves
    Leaf *leaf = curr->first_leaf;
    while (leaf != NULL)
    {
        if (strcmp(leaf->base.name, name) == 0)
            return leaf;
        leaf = leaf->next_leaf;
    }

    // If not found, recursively search in subdirectories
    Directory *child = curr->first_child;
    while (child != NULL)
    {
        Leaf *result = find_leaf(tree, child, name);
        if (result != NULL)
            return result;
        child = child->next_dir;
    }

    return NULL;
}

/*
 * Constructs the full path of a node by concatenating all parent directory names.
 *
 * Parameters:
 *   node - Pointer to the node (can be either directory or file)
 *
 * Returns:
 *   char* - Dynamically allocated string containing the full path
 *          Returns NULL if node is NULL or if memory allocation fails
 *
 * Notes:
 *   - Traverses up the tree from node to root
 *   - Builds path in format: "root/parent/child"
 *   - Uses two passes: first to calculate length, second to build path
 *   - Caller is responsible for freeing the returned string
 *   - No leading slash is added to the path
 */
char *get_node_path(Node *node)
{
    if (node == NULL)
        return NULL;

    size_t len = 0;
    Node *curr = node;
    while (curr != NULL)
    {
        len += strlen(curr->name) + 1; // +1 for slash
        curr = curr->parent;
    }

    char *path = (char *)malloc(len + 1); // +1 for null terminator
    if (path == NULL)
        return NULL;

    path[len] = '\0';
    curr = node;

    while (curr != NULL)
    {
        size_t name_len = strlen(curr->name);
        len -= name_len;
        memcpy(path + len, curr->name, name_len);
        if (len > 0)
        {
            len--;
            path[len] = '/';
        }
        curr = curr->parent;
    }
    return path;
}

/*
 * Recursively counts the total number of files (leaves) in the tree or subtree.
 *
 * Parameters:
 *   tree - Pointer to the tree structure
 *
 * Returns:
 *   uint32_t - Total number of files (leaves) in the tree
 *              Returns 0 if tree is NULL or empty
 *
 * Notes:
 *   - Traverses the entire tree structure recursively
 *   - For each directory, counts its immediate files
 *   - Creates temporary subtrees to count files in child directories
 *   - Maintains original tree function pointers during recursion
 */
uint32_t get_total_files(Tree *tree)
{
    if (tree == NULL || tree->root == NULL)
        return 0;

    uint32_t count = 0;
    Directory *curr = tree->root;

    // Count files in current directory
    Leaf *leaf = curr->first_leaf;
    while (leaf != NULL)
    {
        count++;
        leaf = leaf->next_leaf;
    }

    // Recursively count files in subdirectories
    Directory *child = curr->first_child;
    while (child != NULL)
    {
        // Create a new Tree instance with the same function pointers
        Tree subtree = {
            .root = child,
            .total_dirs = tree->total_dirs,
            .total_size = tree->total_size,
            .destroy = tree->destroy,
            .compare = tree->compare};
        count += get_total_files(&subtree);
        child = child->next_dir;
    }
    return count;
}

void print_node_info(Node *node, int depth)
{
    if (node == NULL)
        return;

    // Print indentation
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }

    if (is_directory(node))
    {
        Directory *dir = (Directory *)node;
        printf("📁 %s/ (size: %u)\n", node->name, get_directory_size(dir));

        // Print leaves first
        Leaf *leaf = dir->first_leaf;
        while (leaf != NULL)
        {
            print_node_info((Node *)leaf, depth + 1);
            leaf = leaf->next_leaf;
        }

        // Then print subdirectories
        Directory *child = dir->first_child;
        while (child != NULL)
        {
            print_node_info((Node *)child, depth + 1);
            child = child->next_dir;
        }
    }
    else if (is_leaf(node))
    {
        Leaf *leaf = (Leaf *)node;
        printf("📄 %s (size: %u)\n", node->name, leaf->size);
    }
}

void print_tree(Tree *tree)
{
    if (tree == NULL || tree->root == NULL)
    {
        printf("Empty tree\n");
        return;
    }

    printf("\nDirectory Tree:\n");
    printf("Total size: %u bytes\n", get_total_size(tree));
    printf("Total directories: %u\n", get_total_directories(tree));
    printf("Total files: %u\n\n", get_total_files(tree));

    print_node_info((Node *)tree->root, 0);
    printf("\n");
}