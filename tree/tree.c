#include "tree.h"

static void destroy_directory(Tree *tree, Directory *dir);
static char **split_path(const char *path, int *count);
static void free_path_components(char **components, int count);
static Directory *find_child_directory(Directory *parent, const char *name);

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

Directory *get_parent_directory(Directory *dir)
{
    if (dir == NULL || dir->base.parent == NULL)
        return NULL;
    return (Directory *)dir->base.parent;
}

// Helper function to split and normalize path
static char **split_path(const char *path, int *count)
{
    char *path_copy = strdup(path);
    char **components = malloc(sizeof(char *) * 256); // Max 256 components
    *count = 0;

    // Skip leading slash
    char *token = strtok(path_copy, "/");
    while (token != NULL && *count < 256)
    {
        components[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, "/");
    }

    free(path_copy);
    return components;
}

// Helper to free split path
static void free_path_components(char **components, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(components[i]);
    }
    free(components);
}

/**
 * @brief Finds a child directory with the specified name within a parent directory
 *
 * @param parent The parent directory to search in
 * @param name The name of the child directory to find
 * @return Directory* Pointer to the found directory, or NULL if not found
 *
 * @note This is an internal helper function used by create_directory and create_nested_directory
 * @warning Not thread-safe, assumes parent and name are valid
 */
Directory *find_child_directory(Directory *parent, const char *name)
{
    if (!parent || !name)
        return NULL;

    Directory *child = parent->first_child;
    while (child != NULL)
    {
        if (strcmp(child->base.name, name) == 0)
        {
            return child;
        }
        child = child->next_dir;
    }
    return NULL;
}

/**
 * @brief Initializes a new tree structure with the specified comparison and destruction functions.
 *
 * @param tree Pointer to the tree structure to initialize
 * @param compare Function pointer for comparing node values (can be NULL)
 * @param destroy Function pointer for cleaning up node values (can be NULL)
 *
 * @note This function sets initial values for root, directory count, and total size to zero.
 *       It stores the provided function pointers for later use.
 *       This function does not allocate any memory.
 *
 * @warning Ensure that the tree pointer is valid before calling this function.
 */
void init_tree(Tree *tree, int (*compare)(void *key1, void *key2), void (*destroy)(void *data))
{
    tree->root = NULL;
    tree->total_dirs = 0;
    tree->total_size = 0;
    tree->destroy = destroy;
    tree->compare = compare;
}

/**
 * @brief Destroys the entire tree structure and frees all associated memory.
 *
 * @param tree Pointer to the tree structure to destroy
 *
 * @note This function recursively destroys all directories and their contents,
 *       resets all tree counters to zero, and is safe to call with a NULL pointer.
 *       It uses destroy_directory for recursive cleanup.
 *
 * @warning Ensure that no other references to the tree exist before calling this function
 *          to avoid memory access violations.
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

/**
 * @brief Helper function that recursively destroys a directory and all its contents.
 *
 * @param tree Pointer to the tree structure
 * @param dir Pointer to the directory to destroy
 *
 * @note This function processes subdirectories first, then leaves, and finally the directory itself.
 *       It uses depth-first traversal to ensure proper cleanup and calls the tree's destroy function
 *       for leaf values if provided. It handles both sibling and child relationships separately.
 *
 * @warning This function assumes that the directory pointer is valid and not NULL.
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

/**
 * @brief Creates a single directory node in the tree
 *
 * @param tree Pointer to the tree structure where the directory will be created
 * @param parent Pointer to the parent directory (NULL for root directory)
 * @param name Name of the directory to create (must be less than 256 characters)
 *
 * @return Directory* Pointer to the newly created directory, or NULL if:
 *         - Any input parameter is NULL
 *         - name length >= 256
 *         - Memory allocation fails
 *         - A directory with the same name already exists under the parent
 *         - Attempting to create a root when one already exists
 *
 * @note This function:
 *       - Creates a single directory node
 *       - Sets up parent-child relationships
 *       - Updates directory counts
 *       - Handles both root and child directory creation
 *
 * @example
 *     // Create root directory
 *     Directory *root = create_directory(tree, NULL, "root");
 *     // Create child directory
 *     Directory *child = create_directory(tree, root, "child");
 */

Directory *create_directory(Tree *tree, Directory *parent, const char *name)
{
    if (tree == NULL || name == NULL || strlen(name) >= 256)
        return NULL;

    // Check for existing directory with same name
    if (parent != NULL && find_child_directory(parent, name) != NULL)
    {
        return NULL;
    }

    Directory *new_dir = (Directory *)malloc(sizeof(Directory));
    if (new_dir == NULL)
        return NULL;

    // Initialize the new directory
    memset(new_dir, 0, sizeof(Directory));
    strncpy(new_dir->base.name, name, 255);
    new_dir->base.parent = (Node *)parent;

    // Handle root directory
    if (parent == NULL)
    {
        if (tree->root != NULL)
        {
            free(new_dir);
            return NULL;
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
                curr = curr->next_dir;
            }
            curr->next_dir = new_dir;
        }
        parent->dir_count++;
    }

    tree->total_dirs++;
    return new_dir;
}

/**
 * @brief Creates a nested directory path, creating intermediate directories as needed
 *
 * @param tree Pointer to the tree structure where the directories will be created
 * @param path Full path of directories to create (e.g., "/hello/world")
 *
 * @return Directory* Pointer to the last directory in the path, or NULL if:
 *         - Any input parameter is NULL
 *         - Path is empty
 *         - Memory allocation fails
 *         - Path component name >= 256 characters
 *         - Invalid path format
 *
 * @note This function:
 *       - Splits the path into components
 *       - Creates or finds each directory in the path
 *       - Creates missing intermediate directories
 *       - Returns existing directories if they already exist
 *       - Properly handles absolute paths (starting with /)
 *
 * @example
 *     // Create nested directories
 *     Directory *dir = create_nested_directory(tree, "/hello/world");
 *     // Creates:
 *     // /
 *     // └── hello/
 *     //     └── world/
 *
 * @warning
 *     - Path components must be separated by '/'
 *     - Each path component must be < 256 characters
 *     - Memory for path components is freed before returning
 */
Directory *create_nested_directory(Tree *tree, const char *path)
{
    if (tree == NULL || path == NULL || strlen(path) == 0)
        return NULL;

    int count;
    char **components = split_path(path, &count);
    if (!components)
        return NULL;

    Directory *current = tree->root;
    // Directory *parent = NULL;

    for (int i = 0; i < count; i++)
    {
        if (strlen(components[i]) == 0)
            continue; // Skip empty components

        if (i == 0)
        {
            // Handle root level
            if (current == NULL)
            {
                current = create_directory(tree, NULL, components[i]);
            }
            else
            {
                Directory *found = find_child_directory(current, components[i]);
                if (!found)
                {
                    current = create_directory(tree, current, components[i]);
                }
                else
                {
                    current = found;
                }
            }
        }
        else
        {
            // Handle nested directories
            Directory *found = find_child_directory(current, components[i]);
            if (!found)
            {
                found = create_directory(tree, current, components[i]);
            }
            if (!found)
            {
                free_path_components(components, count);
                return NULL;
            }
            current = found;
        }

        if (!current)
        {
            free_path_components(components, count);
            return NULL;
        }
    }

    free_path_components(components, count);
    return current;
}

/**
 * @brief Searches for a directory in the tree based on the specified path.
 *
 * @param tree Pointer to the tree structure where the search will be performed
 * @param path A string representing the full path of the directory to search for
 *
 * @return Directory* Pointer to the found directory if it exists, or NULL if:
 *         - Any input parameter is NULL
 *         - The path is empty
 *         - The directory is not found in the tree
 *
 * @note This function splits the input path into components to handle nested directories.
 *       It iterates through each component, searching for the corresponding directory.
 *       If any component is not found, the function returns NULL.
 *       The search is case-sensitive and traverses the tree structure recursively.
 */

Directory *find_directory(Tree *tree, const char *path)
{
    if (!tree || !path || strlen(path) == 0)
        return NULL;

    int count;
    char **components = split_path(path, &count);
    if (!components)
        return NULL;

    Directory *current = tree->root;

    // Skip first component if it's empty (happens with leading /)
    int start = (strlen(components[0]) == 0) ? 1 : 0;

    for (int i = start; i < count; i++)
    {
        if (!current)
        {
            free_path_components(components, count);
            return NULL;
        }
        current = find_child_directory(current, components[i]);
    }

    free_path_components(components, count);
    return current;
}

/**
 * @brief Removes a directory from the tree and its contents.
 *
 * @param tree Pointer to the tree structure
 * @param dir Pointer to the directory to be removed
 *
 * @return int - 0 on success, or -1 if:
 *         - The tree or directory is NULL
 *         - The directory is the root with children
 *
 * @note This function cannot remove the root directory if it has children or leaves.
 *       It updates the parent's child list and directory count, recursively destroys
 *       the directory and all its contents, and decreases the total directory count in the tree.
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

/**
 * @brief Creates a new leaf (file) node and adds it to the specified parent directory.
 *
 * @param tree Pointer to the tree structure
 * @param parent Pointer to the parent directory where the leaf will be added
 * @param name Name of the new leaf (must be less than 256 characters)
 * @param value Pointer to the leaf's data value
 * @param size Size of the leaf in bytes
 *
 * @return Leaf* Pointer to the newly created leaf on success, or NULL if:
 *         - Any input parameter is NULL
 *         - Name is too long (>= 256 chars)
 *         - A file with the same name exists in parent
 *         - Memory allocation fails
 *
 * @note This function updates the total size of the parent directory and all ancestors,
 *       adds the leaf to the end of the parent's leaf list, initializes all leaf fields
 *       including base node properties, and copies the name into the leaf with null termination.
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

/**
 * @brief Removes a leaf (file) from the tree and updates the necessary size totals.
 *
 * @param tree Pointer to the tree structure
 * @param leaf Pointer to the leaf (file) to be removed
 *
 * @return int - 0 on success, or -1 if:
 *         - The tree or leaf is NULL
 *         - The leaf has no parent
 *
 * @note This function updates the total size of the parent directory and the tree.
 *       If the leaf has a value and a destroy function is provided, it calls the destroy
 *       function on the value. The leaf is freed after removal from the parent's list.
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
/**
 * @brief Recursively searches for a leaf (file) in the tree or subtree.
 *
 * @param tree Pointer to the tree structure
 * @param start Pointer to the directory from which to start the search
 * @param name The name of the leaf to search for
 *
 * @return Leaf* Pointer to the found leaf if it exists, or NULL if:
 *         - The tree is NULL
 *         - Name is NULL
 *         - The leaf is not found
 *
 * @note If 'start' is NULL, the search begins from the root of the tree.
 *       The function first searches in the leaves of the current directory,
 *       and if not found, it recursively searches in all child directories.
 *       The search is case-sensitive.
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

/**
 * @brief Recursively counts the total number of files (leaves) in the tree or subtree.
 *
 * @param tree Pointer to the tree structure
 *
 * @return uint32_t Total number of files (leaves) in the tree, or 0 if:
 *         - The tree is NULL
 *         - The tree is empty
 *
 * @note This function traverses the entire tree structure recursively.
 *       For each directory, it counts its immediate files and creates temporary
 *       subtrees to count files in child directories. It maintains original tree
 *       function pointers during recursion.
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
