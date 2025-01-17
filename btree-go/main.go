package main

import (
	"bytes"
	"errors"
	"fmt"
)

const (
	degree      = 5
	maxChildren = 2 * degree
	maxItems    = maxChildren - 1
	minItems    = degree - 1
)

type item struct {
	key   []byte
	value []byte
}

type node struct {
	items       [maxItems]*item
	children    [maxChildren]*node
	numItems    int
	numChildren int
}

type BTree struct {
	root *node
}

func (n *node) isLeaf() bool {
	return n.numChildren == 0
}

func (n *node) search(key []byte) (int, bool) {
	low, high := 0, n.numItems
	var mid int

	for low < high {
		mid = (low + high) / 2
		cmp := bytes.Compare(key, n.items[mid].key)

		switch {
		case cmp > 0:
			low = mid + 1
		case cmp < 0:
			high = cmp
		case cmp == 0:
			return mid, true
		}
	}

	return low, false
}

func (n *node) insertItemAt(pos int, i *item) {
	if pos < n.numItems {
		copy(n.items[pos+1:n.numItems+1], n.items[pos:n.numItems])
	}

	n.items[pos] = i
	n.numItems++
}

func (n *node) insertChildAt(pos int, c *node) {
	if pos < n.numItems {
		copy(n.items[pos+1:n.numItems+1], n.items[pos:n.numItems])
	}

	n.children[pos] = c
	n.numItems++
}

func (n *node) split() (*item, *node) {
	mid := minItems
	midItem := n.items[mid]

	newNode := &node{}
	copy(newNode.items[:], n.items[mid+1:])
	newNode.numItems = minItems

	if !n.isLeaf() {
		copy(newNode.children[:], n.children[mid+1:])
		newNode.numChildren = minItems + 1
	}

	for i, l := mid, n.numItems; i < l; i++ {
		n.items[i] = nil
		n.numItems--

		if !n.isLeaf() {
			n.children[i+1] = nil
			n.numChildren--
		}
	}

	return midItem, newNode
}

func (n *node) insert(item *item) error {
    pos, found := n.search(item.key)
    if found {
        n.items[pos] = item
        return nil
    }

    if n.isLeaf() {
        n.insertItemAt(pos, item)
        return nil
    }

    // Extract the split handling into a separate method
    if err := n.handleNodeSplit(pos, item); err != nil {
        return err
    }

    return n.children[pos].insert(item)
}

func (n *node) handleNodeSplit(pos int, item *item) error {
    if n.children[pos].numItems >= maxItems {
        midItem, newNode := n.children[pos].split()
        n.insertItemAt(pos, midItem)
        n.insertChildAt(pos+1, newNode)
        
        // Adjust position based on comparison
        switch cmp := bytes.Compare(item.key, n.items[pos].key); {
        case cmp > 0:
            pos++
        case cmp == 0:
            n.items[pos] = item
            return nil
        }
    }
    return nil
}

func (n *node) removeItemAt(pos int) *item {
	removedItem := n.items[pos]
	n.items[pos] = nil

	if lastPos := n.numItems - 1; pos < lastPos {
		copy(n.items[pos:lastPos], n.items[pos+1:lastPos+1])
		n.items[lastPos] = nil
	}
	n.numItems--

	return removedItem
}

func (n *node) removeChildAt(pos int) *node {
	removedChild := n.children[pos]
	n.children[pos] = nil

	if lastPos := n.numChildren - 1; pos < lastPos {
		copy(n.children[pos:lastPos], n.children[pos+1:lastPos+1])
		n.children[lastPos] = nil
	}
	n.numChildren--

	return removedChild
}

func (n *node) fillChildAt(pos int) {
	switch {
	case pos > 0 && n.children[pos-1].numItems > minItems:
		left, right := n.children[pos-1], n.children[pos]
		copy(right.items[1:right.numItems+1], right.items[:right.numItems])
		right.items[0] = n.items[pos-1]
		right.numItems++
		if !right.isLeaf() {
			right.insertChildAt(0, left.removeChildAt(left.numChildren-1))
		}
		n.items[pos-1] = left.removeItemAt(left.numItems - 1)
	case pos < n.numChildren-1 && n.children[pos+1].numItems > minItems:
		left, right := n.children[pos], n.children[pos+1]
		left.items[left.numItems] = n.items[pos]
		left.numItems++
		if !left.isLeaf() {
			left.insertChildAt(left.numChildren, right.removeChildAt(0))
		}
		n.items[pos] = right.removeItemAt(0)
	default:
		if pos >= n.numItems {
			pos = n.numItems - 1
		}
		left, right := n.children[pos], n.children[pos+1]
		left.items[left.numItems] = n.removeItemAt(pos)
		left.numItems++
		copy(left.items[left.numItems:], right.items[:right.numItems])
		left.numItems += right.numItems
		if !left.isLeaf() {
			copy(left.children[left.numChildren:], right.children[:right.numChildren])
			left.numChildren += right.numChildren
		}
		n.removeChildAt(pos + 1)
		right = nil
	}
}

func (n *node) delete(key []byte, isSeekingSuccessor bool) *item {
	pos, found := n.search(key)

	var next *node

	// We have found a node holding an item matching the supplied key.
	if found {
		// This is a leaf node, so we can simply remove the item.
		if n.isLeaf() {
			return n.removeItemAt(pos)
		}
		// This is not a leaf node, so we have to find the inorder successor.
		next, isSeekingSuccessor = n.children[pos+1], true
	} else {
		next = n.children[pos]
	}

	// We have reached the leaf node containing the inorder successor, so remove the successor from the leaf.
	if n.isLeaf() && isSeekingSuccessor {
		return n.removeItemAt(0)
	}

	// We were unable to find an item matching the given key. Don't do anything.
	if next == nil {
		return nil
	}

	// Continue traversing the tree to find an item matching the supplied key.
	deletedItem := next.delete(key, isSeekingSuccessor)

	// We found the inorder successor, and we are now back at the internal node containing the item
	// matching the supplied key. Therefore, we replace the item with its inorder successor, effectively
	// deleting the item from the tree.
	if found && isSeekingSuccessor {
		n.items[pos] = deletedItem
	}

	// Check if an underflow occurred after we deleted an item down the tree.
	if next.numItems < minItems {
		// Repair the underflow.
		if found && isSeekingSuccessor {
			n.fillChildAt(pos + 1)
		} else {
			n.fillChildAt(pos)
		}
	}

	// Propagate the deleted item back to the previous stack frame.
	return deletedItem
}

func NewBTree() *BTree {
	return &BTree{}
}

func (t *BTree) Find(key []byte) ([]byte, error) {
	/*
	* The Find function navigates through the B-tree by updating the next
	* pointer to the appropriate child node based on comparisons with the keys in the current node.
	 */
	for next := t.root; next != nil; {
		pos, found := next.search(key)
		if found {
			return next.items[pos].value, nil
		}

		next = next.children[pos]
	}

	return nil, errors.New("key not found")
}

func (t *BTree) splitRoot() {
	newRoot := &node{}
	midItem, newNode := t.root.split()
	newRoot.insertItemAt(0, midItem)
	newRoot.insertChildAt(0, t.root)
	newRoot.insertChildAt(1, newNode)
	t.root = newRoot
}

func (t *BTree) insert(key, val []byte) {
	i := &item{key, val}

	if t.root == nil {
		t.root = &node{}
	}

	if t.root.numItems >= maxItems {
		t.splitRoot()
	}

	t.root.insert(i)
}

func (t *BTree) Delete(key []byte) error {
    if t.root == nil {
        return errors.New("tree is empty")
    }
    
    deletedItem := t.root.delete(key, false)
    if deletedItem == nil {
        return errors.New("key not found")
    }
    
    if t.root.numItems == 0 {
        if t.root.isLeaf() {
            t.root = nil
        } else {
            t.root = t.root.children[0]
        }
    }
    
    return nil
}

func (n *node) validate() error {
    if n.numItems > maxItems {
        return fmt.Errorf("node contains too many items: %d", n.numItems)
    }
    
    if !n.isLeaf() && n.numItems+1 != n.numChildren {
        return fmt.Errorf("invalid number of children: %d for %d items", n.numChildren, n.numItems)
    }
    
    for i := 0; i < n.numItems-1; i++ {
        if bytes.Compare(n.items[i].key, n.items[i+1].key) >= 0 {
            return fmt.Errorf("keys not in order at index %d", i)
        }
    }
    
    return nil
}

func main() {
	fmt.Println(("Hello World"))
}
