#ifndef TABLEIO_H
#define TABLEIO_H

#include <stdio.h>

#define MAGIC 0xFACE3419
#define METALEN 14

typedef struct table {
	FILE* source;
	u_int64_t cursor;
	u_int64_t free;
	u_int64_t root;
	int metalen;
	int pageRows;
	int pageRowLen;
	int nodeRows;
	int nodeRowLen;
	int pageNodeRatio;
	int pageSize;
	int nodeSize;
	int M;
}table;

/*
each record is a row of data in a table
each element of the record is proceeded by an escape character representing its c type
\s - string
\i - int
\u - u_int32
\l - long
\d - double
\b - bool
\! - NULL
'\ - intentional backslash as a part of a string
*/ 

#endif