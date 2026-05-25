/*
API for reading and writing with database tables

Layout of a table file:

 FACE3419 | Metadata Length | Num page rows | Num node rows | Page rows per node row |
 Pages per row | Nodes per row | Page size | Node size | Pointer to free space | Root pointer |
 M |

Addressing:

 All nodes and pages are referenced by the distance of their first byte from the start of the file
 Represented by an unsigned 64-bit integer
*/

#include "tableIO.h"
#include <stdbool.h>

bool loadMeta(FILE* file, table* table, char* fname) {
	int buf[METALEN];
	fread(&buf, 4, METALEN, file);
	if(buf[0] != MAGIC) {
		printf("Error: failed to open table %s because the file was of an incorrect type\n", fname);
		return false;
	}
	table->source = file;
	table->cursor = 0;
	table->metalen = buf[1];
	table->pageRows = buf[2];
	table->nodeRows = buf[3];
	table->pageNodeRatio = buf[4];
	table->pageRowLen = buf[5];
	table->nodeRowLen = buf[6];
	table->pageSize = buf[7];
	table->nodeSize = buf[8];
	table->free = (long) buf[9] << 32 + (long) buf[10];
	table->root = (long) buf[11] << 32 + (long) buf[12];
	table->M = buf[13];
}

bool writeMeta(FILE* file, table* table) {
	int buf = {
		MAGIC,
		table->metalen,
		table->pageRows,
		table->nodeRows,
		table->pageNodeRatio,
		table->pageRowLen,
		table->nodeRowLen,
		table->pageSize,
		table->nodeSize,
		(int) table->free >> 32,
		(int) table->free & 0xFFFFFFFF,
		(int) table->root >> 32,
		(int) table->root & 0xFFFFFFFF,
		table->M

	};
	fwrite(buf, 4, METALEN, file);
}

bool loadTable(char* fname, table* table) {
	FILE* tfile = fopen(fname, "rb+");
	if (!tfile) {
		printf("Error: Failed to open table %s\n", fname);
	}
	loadMeta(tfile, table, fname);
}

/*
loads a table's cursor into a node
assumes the current location of the cursor is a valid node
*/
void loadNode(table* t) {
	;
}

/*
loads a nodes's cursor into a node
assumes the current location of the cursor is a valid node
*/
void loadPage(table* t) {
	;
}


void loadLeft(node n) {
	;
}

void loadRight(node n) {
	;
}

void loadParent(node n) {
	;
}