# Directory Tree Implementation in C

A flexible and efficient directory tree implementation that simulates a file system structure in memory. This implementation provides a hierarchical organization of directories and files, with support for various operations commonly found in file systems.

## Features

### Core Components

- **Tree Structure**: Maintains a hierarchical organization with directories and files
- **Directory Nodes**: Can contain both files and subdirectories
- **Leaf Nodes**: Represent files with associated data and size
- **Path Support**: Full path generation and navigation capabilities

### Node Types

- **Root Directory**: The top-level directory (tagged with `TREE_TAG_ROOT`)
- **Directory Nodes**: Internal nodes that can contain other directories and files (tagged with `TREE_TAG_NODE`)
- **Leaf Nodes**: File nodes that contain data (tagged with `TREE_TAG_LEAF`)

### Operations

- Directory Operations:
  - Create/Remove directories
  - Find directories by name
  - Get parent directory
  - Get directory size and count
- File Operations:
  - Create/Remove files
  - Find files by name
  - Store arbitrary data in files
  - Track file sizes
- Tree Operations:
  - Initialize/Destroy trees
  - Get total size
  - Get total directory count
  - Get total file count
  - Print tree structure

### Memory Management

- Custom memory cleanup through user-provided destroy functions
- Proper cleanup of all allocated resources
- Prevention of memory leaks during node removal

## Implementation Details

### Node Structure

```c
struct Node {
  char name[256];
  Node parent;
  uint8_t tag;
};

```

### Directory Structure
```c
struct Directory {
  Node base;
  Directory next_dir; // Sibling link
  Leaf first_leaf; // First file
  Directory first_child; // First subdirectory
  uint16_t dir_count; // Subdirectory count
  uint32_t total_size; // Total size of contents
};
```

### File Structure

```c
struct Leaf {
  Node base;
  Leaf next_leaf;
  void value;
  uint16_t size;
};
```

## Usage Example
```c
// Initialize a tree
Tree tree;
init_tree(&tree, NULL, free);
// Create root directory
Directory root = create_directory(&tree, NULL, "root");
// Create subdirectory
Directory docs = create_directory(&tree, root, "documents");
// Create a file
char data = strdup("Hello, World!");
create_leaf(&tree, docs, "hello.txt", data, strlen(data) + 1);
// Print the tree structure
print_tree(&tree);
// Clean up
destroy_tree(&tree);
```

## Features for Extension
- Path-based node access
- Directory traversal callbacks
- Custom comparison functions for sorting
- Size tracking at all levels
- Duplicate name prevention

## Notes
- Maximum filename length is 255 characters
- Supports arbitrary data storage in files
- Thread-safety not implemented (should be handled by caller)
- No built-in persistence (in-memory only)