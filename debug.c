/*
Copyright (c) 2026 Ethan Kothavale

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "debug.h"
#include "chunk.h"
#include "bplus.h"

char* stringTokens[] = {"SELECT", "IDENTIFIER", "NUMBER", "STRING", "STAR", "PLUS", "MINUS", "SLASH", "BANG", "EQUALS", "QUESTION", "RIGHT_PAREN", "LEFT_PAREN", "COMMA", "DOT", "AND", "PIPE", "UNKNOWN"};

void printTokenizedQuery(TokenizedQuery* tquery) {
    for (int i = 0; i < tquery->count; i++) {
        Token* token = &tquery->tokens[i];
        printf("Token %d: Type=%s, Payload=%p, Address=%p\n", i, stringTokens[token->type], (void*) token->payload, (void*) token);
    }
	printf("Total tokens: %d\n", tquery->count);
}

void printChunk(Chunk* chunk) {
    for (int i = 0; i < chunk->count; i++) {
        printf("Chunk %d: Value=%d, Address=%p\n", i, chunk->code[i], (void*) &chunk->code[i]);
    }
	printf("Total chunks: %d\n", chunk->count);
}

btree_node* generateTestBPlusTree() {
    // Create the root node
    btree_node* root = malloc(sizeof(btree_node));
    root->isLeaf = false;
    root->parent = NULL;
    root->prev = NULL;
    root->next = NULL;
    root->childCount = 3; // Root has 3 children

    // Create internal nodes
    btree_node* internalNodes[3];
    for (int i = 0; i < 3; i++) {
        internalNodes[i] = malloc(sizeof(btree_node));
        internalNodes[i]->isLeaf = false;
        internalNodes[i]->parent = root;
        internalNodes[i]->prev = (i > 0) ? internalNodes[i - 1] : NULL;
        internalNodes[i]->next = NULL;
        if (i > 0) internalNodes[i - 1]->next = internalNodes[i];
        root->children[i] = internalNodes[i];
    }

    // Fibonacci sequence initialization
    uint32_t num = 1;

    // Create leaf nodes
    int leafNodeCounts[3] = {2, 2, 2}; // Number of leaf nodes per internal node
    btree_node* prevLeaf = NULL;
    for (int i = 0; i < 3; i++) {
        internalNodes[i]->childCount = leafNodeCounts[i];
        int maxPageNumber = 0; // Track max page number for internal node
        for (int j = 0; j < leafNodeCounts[i]; j++) {
            btree_node* leaf = malloc(sizeof(btree_node));
            leaf->isLeaf = true;
            leaf->parent = internalNodes[i];
            leaf->prev = prevLeaf;
            leaf->next = NULL;
            if (prevLeaf) prevLeaf->next = leaf;
            prevLeaf = leaf;
            internalNodes[i]->children[j] = leaf;

            // Create pages for the leaf node
            int pageCount = (j % 3) + 1; // Variable, nonzero number of pages
            leaf->childCount = pageCount;
            for (int k = 0; k < pageCount; k++) {
                slotted_page* p = malloc(sizeof(slotted_page));
                p->header.pageNum = num;
                num += 50; // increment pageNums by 50

                p->header.numRecords = 0;
                p->header.parent = leaf;
                leaf->children[k] = p;

                // Update maxPageNumber for the leaf node
                if (p->header.pageNum > maxPageNumber) {
                    maxPageNumber = p->header.pageNum;
                }
            }

            // Set the key range for the leaf node
            leaf->keys[0] = ((slotted_page*)leaf->children[0])->header.pageNum;
            leaf->keys[1] = ((slotted_page*)leaf->children[leaf->childCount - 1])->header.pageNum;
            leaf->maxPageNumber = maxPageNumber; // Update maxPageNumber for the leaf
        }

        // Set the keys and maxPageNumber for the internal node
        internalNodes[i]->keys[0] = ((btree_node*)internalNodes[i]->children[0])->keys[1];
        if (internalNodes[i]->childCount > 1) {
            internalNodes[i]->keys[1] = ((btree_node*)internalNodes[i]->children[1])->keys[1];
        }
        internalNodes[i]->maxPageNumber = maxPageNumber;
    }

    // Set keys and maxPageNumber for the root node
    root->keys[0] = internalNodes[0]->keys[1]; // Pages <= key in the left subtree
    root->keys[1] = internalNodes[1]->keys[1]; // Pages <= key in the middle subtree
    root->maxPageNumber = internalNodes[2]->maxPageNumber; // Max page number in the entire tree

    return root;
}

void printIntArray(int* arr, int length) {
    if(arr == NULL) {
        printf("Array is NULL\n");
        return;
    }
    printf("[%d", arr[0]);
    if (length == 1) return;
    for (int i = 1; i < length; i++) {
        printf(", %d", arr[i]);
    }
    printf("]\n");
}

/* UNTESTED
incomplete
*/
void printNode(btree_node* n) {
    printf(n->isLeaf ? "Leaf" : "Internal");
    printf(" node with\nKeys:");
    for (int i = 0; i < n->childCount-1; i++) {
        printf(" %d", n->keys[i]);
    }
    printf("\n%d Children:", n->childCount);
    for (int i = 0; i < n->childCount; i++) {
        printf(" %p", n->children[i]);
    }
    printf("\nMaximum Page Number: %d\n", n->maxPageNumber);
    printf("Parent: %p\n", n->parent);
    printf("Previous: %p\n", n->prev);
    printf("Next: %p\n\n", n->next);
}

void printTreeHelper(btree_node* root, int level) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }

    // Indentation for the current level
    for (int i = 0; i < level; i++) {
        printf("    ");
    }

    // Print the current node with its address
    printf("Node at level %d (Address: %p): ", level, (void*)root);
    if (root->isLeaf) {
        printf("[Leaf Node] Keys: ");
        for (int i = 0; i < root->childCount; i++) {
            printf("%d ", root->keys[i]);
        }
        printf("\n");

        // Print the pages this leaf node points to
        for (int i = 0; i < root->childCount; i++) {
            slotted_page* p = (slotted_page*)root->children[i];
            if (p != NULL) {
                for (int j = 0; j < level + 1; j++) {
                    printf("    ");
                }
                printf("Page %d\n", p->header.pageNum);
            }
        }
    } else {
        printf("[Internal Node] Keys: ");
        for (int i = 0; i < root->childCount - 1; i++) {
            printf("%d ", root->keys[i]);
        }
        printf("\n");

        // Recursively print child nodes
        for (int i = 0; i < root->childCount; i++) {
            printTreeHelper((btree_node*)root->children[i], level + 1);
        }
    }
}

void printTree(tree* root) {
    printTreeHelper(root->root, 1);
}


bool checkTreePointersHelper(btree_node* root) {
    if (root == NULL) {
        return true; // An empty tree is valid
    }

    for (int i = 0; i < root->childCount; i++) {
        void* child = root->children[i];
        if (child == NULL) {
            continue; // Skip null children
        }

        if (root->isLeaf) {
            // If the root is a leaf, its children are pages
            slotted_page* p = (slotted_page*)child;
            if (p->header.parent != root) {
                printf("Error: Page %d has incorrect parent pointer. Expected parent: %p, Actual parent: %p\n", 
                       p->header.pageNum, (void*)root, (void*)p->header.parent);
                return false;
            }
        } else {
            // If the root is an internal node, its children are nodes
            btree_node* n = (btree_node*)child;
            if (n->parent != root) {
                printf("Error: Node at %p has incorrect parent pointer. Expected parent: %p, Actual parent: %p\n", 
                       (void*)n, (void*)root, (void*)n->parent);
                return false;
            }

            // Recursively check the subtree
            if (!checkTreePointersHelper(n)) {
                return false;
            }
        }
    }

    return true; // All checks passed
}

bool checkTreePointers(tree* t) {
    btree_node* root = t->root;
    return checkTreePointersHelper(root);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// SLOTTED PAGE PRETTY-PRINTERS

static const char* datatypeStr(datatype t) {
    switch (t) {
        case T_INT:    return "INT";
        case T_STRING: return "STR";
        case T_DATE:   return "DATE";
        case T_TIME:   return "TIME";
        default:       return "???";
    }
}

/*
Prints a single entry in the form TYPE(value), e.g. STR(Alice) or INT(42).
*/
void printEntry(entry* e) {
    if (e == NULL || e->data == NULL) {
        printf("<NULL>");
        return;
    }
    printf("%s(%s)", datatypeStr(e->type), e->data);
}

/*
Prints a single slot's metadata.
*/
void printSPSlot(sp_slot* s) {
    if (s == NULL) {
        printf("<NULL slot>\n");
        return;
    }
    printf("{ ID=%-4u  ptr=%-4u  len=%-4u }", s->ID, s->ptr, s->len);
}

/*
Pretty-prints a slotted_page showing its header, the slot directory, and
the contents of every record stored in the page.

Layout printed:
  === Slotted Page #N ===
  header fields ...
  --- Slot Directory ---
  idx   ID     ptr    len
  ...
  --- Records ---
  [ID=N]  entry0, entry1, ...
*/
void printSlottedPage(slotted_page* p) {
    if (p == NULL) {
        printf("[NULL slotted_page]\n");
        return;
    }

    printf("=== Slotted Page #%u ===\n", p->header.pageNum);
    printf("  parent    : %llu\n", (unsigned long long)p->header.parent);
    printf("  numRecords: %u\n",   p->header.numRecords);
    printf("  numEntries: %u\n",   p->header.numEntries);
    printf("  usedData  : %u bytes\n\n", p->header.usedData);

    printf("  --- Slot Directory ---\n");
    printf("  %4s  %6s  %6s  %6s\n", "idx", "ID", "ptr", "len");
    printf("  %4s  %6s  %6s  %6s\n", "----", "------", "------", "------");
    for (uint32_t i = 0; i < p->header.numRecords; i++) {
        sp_slot* s = &p->slots[i];
        printf("  %4u  %6u  %6u  %6u\n", i, s->ID, s->ptr, s->len);
    }

    printf("\n  --- Records ---\n");
    for (uint32_t i = 0; i < p->header.numRecords; i++) {
        sp_slot* s = &p->slots[i];
        printf("  [ID=%-4u]  ", s->ID);
        for (uint32_t j = 0; j < s->len; j++) {
            if (j > 0) printf(", ");
            printEntry(&p->entries[s->ptr + j]);
        }
        printf("\n");
    }
    printf("======================\n");
}