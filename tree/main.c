#include <stdio.h>
#include "tree.h"


void test_tree()
{
    Tree tree;
    init_tree(&tree, NULL, free); // Using standard free as destructor

    // Create root directory
    Directory *root = create_directory(&tree, NULL, "root");

    // Create some subdirectories
    Directory *docs = create_directory(&tree, root, "documents");
    Directory *pics = create_directory(&tree, root, "pictures");
    Directory *work = create_directory(&tree, docs, "work");

    // Create some files
    create_leaf(&tree, root, "readme.txt", NULL, 100);
    create_leaf(&tree, docs, "resume.pdf", NULL, 500);
    create_leaf(&tree, work, "project.doc", NULL, 250);
    create_leaf(&tree, pics, "vacation.jpg", NULL, 1024);
    create_leaf(&tree, pics, "family.jpg", NULL, 2048);

    // Print the entire tree
    print_tree(&tree);

    // Test finding nodes
    Directory *found_dir = find_directory(&tree, NULL, "work");
    if (found_dir)
    {
        char *path = get_node_path((Node *)found_dir);
        printf("\nFound directory 'work' at path: %s\n", path);
        free(path);
    }

    Leaf *found_file = find_leaf(&tree, NULL, "vacation.jpg");
    if (found_file)
    {
        char *path = get_node_path((Node *)found_file);
        printf("Found file 'vacation.jpg' at path: %s\n", path);
        free(path);
    }

    // Test removal
    printf("\nRemoving 'work' directory...\n");
    remove_directory(&tree, work);
    print_tree(&tree);

    // Cleanup
    destroy_tree(&tree);
}

int main()
{
    test_tree();
    return 0;
}