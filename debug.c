#include "debug.h"
#include "chunk.h"

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

node* generateTestBPlusTree() {
    // Create the root node
    node* root = malloc(sizeof(node));
    root->isLeaf = false;
    root->parent = NULL;
    root->prev = NULL;
    root->next = NULL;
    root->childCount = 3; // Root has 3 children

    // Create internal nodes
    node* internalNodes[3];
    for (int i = 0; i < 3; i++) {
        internalNodes[i] = malloc(sizeof(node));
        internalNodes[i]->isLeaf = false;
        internalNodes[i]->parent = root;
        internalNodes[i]->prev = (i > 0) ? internalNodes[i - 1] : NULL;
        internalNodes[i]->next = NULL;
        if (i > 0) internalNodes[i - 1]->next = internalNodes[i];
        root->children[i] = internalNodes[i];
    }

    // Fibonacci sequence initialization
    int fib1 = 1, fib2 = 1;

    // Create leaf nodes
    int leafNodeCounts[3] = {2, 2, 2}; // Number of leaf nodes per internal node
    node* prevLeaf = NULL;
    for (int i = 0; i < 3; i++) {
        internalNodes[i]->childCount = leafNodeCounts[i];
        int maxPageNumber = 0; // Track max page number for internal node
        for (int j = 0; j < leafNodeCounts[i]; j++) {
            node* leaf = malloc(sizeof(node));
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
                page* p = malloc(sizeof(page));
                p->pageNum = fib2; // Assign Fibonacci number as page number
                int nextFib = fib1 + fib2;
                fib1 = fib2;
                fib2 = nextFib;

                p->numRecords = 0;
                p->parent = leaf;
                leaf->children[k] = p;

                // Update maxPageNumber for the leaf node
                if (p->pageNum > maxPageNumber) {
                    maxPageNumber = p->pageNum;
                }
            }

            // Set the key range for the leaf node
            leaf->keys[0] = ((page*)leaf->children[0])->pageNum;
            leaf->keys[1] = ((page*)leaf->children[leaf->childCount - 1])->pageNum;
            leaf->maxPageNumber = maxPageNumber; // Update maxPageNumber for the leaf
        }

        // Set the keys and maxPageNumber for the internal node
        internalNodes[i]->keys[0] = ((node*)internalNodes[i]->children[0])->keys[1];
        if (internalNodes[i]->childCount > 1) {
            internalNodes[i]->keys[1] = ((node*)internalNodes[i]->children[1])->keys[1];
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
/*
void printPage(page* p) {
	printf("Page %d\n", p->pageNum);
	printf("Parent node: %p\n", p->parent);
	printf("Used slots: %d\nUsed memory: %dB\n", p->totalUsedSlots, p->usedMem);
	printf("Slot array:\n");
	for (int i = 0; i < p->totalUsedSlots - 1; i++) {
		printf("%d | ", p->slotarr[i]);
	}
	printf("%d\n", p->slotarr[p->totalUsedSlots-1]);
	printf("Stack Top: %d\n", p->valsOffset);
	printf("Memory:\n");
	for (int l = p->valsOffset; l > 0; l--) {
		printf("%d | ", p->cells[NUM_VALS - 1 - l]);
	}
	printf("%d\n", p->cells[NUM_VALS-1]);
	printf("\n");
}*/

void printPage(page* p) {
    printf("Page %d\n", p->pageNum);
	printf("Parent node: %p\n", p->parent);
	printf("Used slots: %d\n", p->numRecords);
	printf("Slot array:\n");
	for (int i = 0; i < p->numRecords - 1; i++) {
		printf("%d | ", p->records[i]);
	}
    printf("%d\n", p->records[p->numRecords-1]);
}

/* UNTESTED
incomplete
*/
void printNode(node* n) {
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

void printTree(node* root, int level) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }

    // Indentation for the current level
    for (int i = 0; i < level; i++) {
        printf("    ");
    }

    // Print the current node
    printf("Node at level %d: ", level);
    if (root->isLeaf) {
        printf("[Leaf Node] Keys: ");
        for (int i = 0; i < root->childCount; i++) {
            printf("%d ", root->keys[i]);
        }
        printf("\n");

        // Print the pages this leaf node points to
        for (int i = 0; i < root->childCount; i++) {
            page* p = (page*)root->children[i];
            if (p != NULL) {
                for (int j = 0; j < level + 1; j++) {
                    printf("    ");
                }
                printf("Page %d\n", p->pageNum);
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
            printTree((node*)root->children[i], level + 1);
        }
    }
}