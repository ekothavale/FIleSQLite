#include <stdarg.h>
#include <sys/stat.h>
#include "../common.h"
#include "../debug.h"
#include "parser.h"
#include "generator.h"
#include "schema.h"
#include "vm.h"

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

void initVM(hashtable* schema) {
	resetStack();
	for (int i = 0; i < MAX_SCANNERS; i++) {
		vm.scanners[i].open = false;
		vm.scanners[i].tbl = NULL;
	}
	vm.results.rows     = NULL;
	vm.results.count    = 0;
	vm.results.capacity = 0;
	vm.results.cols     = 0;
	vm.schema           = schema;
	vm.results.print	= false;
}

void freeVM() {
	for (int i = 0; i < MAX_SCANNERS; i++) {
		if (vm.scanners[i].open) {
			commit(vm.scanners[i].tbl);
			fclose(vm.scanners[i].tbl->source);
			freeTable(vm.scanners[i].tbl);
			if (vm.scanners[i].leafNode) free(vm.scanners[i].leafNode);
			if (vm.scanners[i].page) freeSPage(vm.scanners[i].page);
			vm.scanners[i].tbl = NULL;
			vm.scanners[i].leafNode = NULL;
			vm.scanners[i].page = NULL;
			vm.scanners[i].open = false;
		}
	}
	if (vm.results.rows) {
		for (int i = 0; i < vm.results.count; i++) free(vm.results.rows[i]);
		free(vm.results.rows);
		vm.results.rows  = NULL;
		vm.results.count = 0;
	}
}

static void openScanner(const char* tablename) {
	for (int i = 0; i < MAX_SCANNERS; i++) {
		if (!vm.scanners[i].open) {
			table* t = malloc(sizeof(table));
			if (!loadTable((char*)tablename, t)) {
				free(t);
				return;
			}
			vm.scanners[i].tbl      = t;
			vm.scanners[i].open     = true;
			vm.scanners[i].started  = false;
			vm.scanners[i].atEnd    = false;
			vm.scanners[i].leafNode = NULL;
			vm.scanners[i].leafAddr = 0;
			vm.scanners[i].childIdx = 0;
			vm.scanners[i].page     = NULL;
			vm.scanners[i].pageAddr = 0;
			vm.scanners[i].slotIdx  = 0;
			return;
		}
	}
	printf("Error: no free scanner slots available\n");
}

static void closeScanner(scanner* c) {
	if (!c->open) return;
	commit(c->tbl);
	fclose(c->tbl->source);
	freeTable(c->tbl);
	if (c->leafNode) { free(c->leafNode); c->leafNode = NULL; }
	if (c->page)     { freeSPage(c->page); c->page = NULL; }
	c->tbl     = NULL;
	c->open    = false;
	c->started = false;
	c->atEnd   = false;
}

static bool tableAlreadyExists(const char* tablename) {
	char* dir = TABLE_DIRECTORY;
	char* ext = TABLE_EXTENSION;
	int pathLen = strlen(dir) + strlen(tablename) + strlen(ext);
	char* path = malloc(pathLen + 1);
	snprintf(path, pathLen + 1, "%s%s%s", dir, tablename, ext);
	struct stat st;
	bool exists = stat(path, &st) == 0;
	free(path);
	return exists;
}

/*
determines if two values are equal
*/
static bool equal(value a, value b) {
	if (a.type != b.type) return false;
	switch (a.type) {
		case VAL_BOOL: return a.as.boolean == b.as.boolean;
		case VAL_FLOAT: return a.as.floating == b.as.floating;
		case VAL_INT: return a.as.integer == b.as.integer;
		case VAL_NULL: return true;
		case VAL_TEXT: return strcmp(a.as.text, b.as.text) ? 0 : 1;
		case VAL_U32: return a.as.u32 == b.as.u32;
		default: {
			runtimeError("Equality not supported for type %i", a.type);
			break;
		}
	}
}

static bool lessThan(value a, value b) {
	if (a.type != b.type) {
		runtimeError("Operands of less than comparison must have the same type");
		return false;
	}
	switch (a.type) {
		case VAL_BOOL: return a.as.boolean < b.as.boolean;
		case VAL_FLOAT: return a.as.floating < b.as.floating;
		case VAL_INT: return a.as.integer < b.as.integer;
		case VAL_NULL: return false;
		//case VAL_TEXT: return strcmp(a.as.text, b.as.text) ? 1 : 0;
		case VAL_U32: return a.as.u32 < b.as.u32;
		default: {
			runtimeError("Less than not supported for type %i", a.type);
			return false;
		}
	}
}

static bool greaterThan(value a, value b) {
	if (a.type != b.type) {
		runtimeError("Operands of greater than comparison must have the same type");
		return false;
	}
	switch (a.type) {
		case VAL_BOOL: return a.as.boolean > b.as.boolean;
		case VAL_FLOAT: return a.as.floating > b.as.floating;
		case VAL_INT: return a.as.integer > b.as.integer;
		case VAL_NULL: return false;
		//case VAL_TEXT: return strcmp(a.as.text, b.as.text) ? 1 : 0;
		case VAL_U32: return a.as.u32 > b.as.u32;
		default: {
			runtimeError("Greater than not supported for type %i", a.type);
			return false;
			break;
		}
	}
}

/*
Push value to VM's stack
*/
void push(value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

/*
Pop value from top of VM's stack
*/
value pop() {
	vm.stackTop--;
	return *vm.stackTop;
}

/*
Scans forward through pages and leaf nodes until finding one with records.
Returns true if a valid row is now current, false if the table is exhausted.
*/
static bool loadFirstValidPage(scanner* s) {
	table* t = s->tbl;
	while (true) {
		while (s->childIdx < s->leafNode->childCount) {
			s->pageAddr = s->leafNode->children[s->childIdx];
			readPage(s->pageAddr, s->page, t);
			s->slotIdx = 0;
			if (s->page->header.numRecords > 0) return true;
			s->childIdx++;
		}
		if (s->leafNode->next == 0) {
			s->atEnd = true;
			return false;
		}
		s->leafAddr = s->leafNode->next;
		readNode(s->leafAddr, s->leafNode, t);
		s->childIdx = 0;
	}
}

/*
Advances the scanner to the next row.
On the first call (started == false), walks from root to the first row.
Returns true if a valid row is now current, false if the table is exhausted.
*/
static bool advanceScanner(scanner* s) {
	if (s->atEnd) return false;
	table* t = s->tbl;

	if (!s->started) {
		if (!s->leafNode) s->leafNode = malloc(sizeof(node));
		if (!s->page) {
			s->page = malloc(sizeof(slotted_page));
			s->page->slots   = NULL;
			s->page->entries = NULL;
		}
		// walk down to leftmost leaf
		address nAddr = t->root;
		readNode(nAddr, s->leafNode, t);
		while (!s->leafNode->isLeaf) {
			nAddr = s->leafNode->children[0];
			readNode(nAddr, s->leafNode, t);
		}
		s->leafAddr = nAddr;
		s->childIdx = 0;
		s->started  = true;
		return loadFirstValidPage(s);
	}

	// advance within current page
	s->slotIdx++;
	if (s->slotIdx < s->page->header.numRecords) return true;

	// move to next page
	s->childIdx++;
	return loadFirstValidPage(s);
}

/*
Converts a VM value to a heap-allocated storage engine entry.
Caller must eventually free e.data (or transfer ownership to a slotted_page).
VAL_FLOAT is stored as raw IEEE 754 bytes under T_STRING — no T_FLOAT type exists yet.
*/
static entry valueToEntry(value v) {
	entry e;
	switch (v.type) {
		case VAL_INT: {
			int32_t raw = (int32_t)v.as.integer;
			e.type = T_INT;
			e.size = sizeof(int32_t);
			e.data = malloc(e.size);
			memcpy(e.data, &raw, e.size);
			break;
		}
		case VAL_TEXT: {
			e.type = T_STRING;
			e.size = (uint32_t)strlen(v.as.text);
			e.data = malloc(e.size);
			memcpy(e.data, v.as.text, e.size);
			break;
		}
		case VAL_BOOL: {
			int32_t raw = v.as.boolean ? 1 : 0;
			e.type = T_INT;
			e.size = sizeof(int32_t);
			e.data = malloc(e.size);
			memcpy(e.data, &raw, e.size);
			break;
		}
		case VAL_FLOAT: {
			e.type = T_STRING;
			e.size = sizeof(double);
			e.data = malloc(e.size);
			memcpy(e.data, &v.as.floating, e.size);
			break;
		}
		default: { // VAL_NULL
			e.type = T_INT;
			e.size = 0;
			e.data = NULL;
			break;
		}
	}
	return e;
}

/*
SQL LIKE pattern matching: % matches any sequence of characters, _ matches exactly one.
Returns true if str matches pattern.
*/
static bool likeMatch(const char* str, const char* pattern) {
	if (*pattern == '\0') return *str == '\0';
	if (*pattern == '%') {
		do {
			if (likeMatch(str, pattern + 1)) return true;
		} while (*str++ != '\0');
		return false;
	}
	if (*str == '\0') return false;
	if (*pattern == '_' || *pattern == *str) return likeMatch(str + 1, pattern + 1);
	return false;
}

static interpret_result run() {
	#define READ_BYTE() (*vm.ip++)
	#define READ_TWO_BYTES() (((uint16_t) *vm.ip++ << 8) | (uint16_t) *vm.ip++)
	#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
	// somewhat dubious macro that takes operators as arguments
	#define BINARY_OP(op) \
		do { \
			value b = pop(); \
			value a = pop(); \
			if (a.type == VAL_FLOAT && b.type == VAL_FLOAT) push(FLOAT_VAL(a.as.floating op b.as.floating)); \
			else if (a.type == VAL_FLOAT) push(FLOAT_VAL(a.as.floating op b.as.integer)); \
			else if (b.type == VAL_FLOAT) push(FLOAT_VAL(a.as.integer op b.as.floating)); \
			else push(INTEGER_VAL(a.as.integer op b.as.integer)); \
		} while (false)

	for (;;) {
		#ifdef DEBUG_TRACE_EXECUTION
			printf("        ");
			for (value* slot = vm.stack; slot < vm.stackTop; slot++) {
				printf("[ ");
				printValue(*slot);
				printf(" ]");
			}
			printf("\n");
			disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
		#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			case OP_TRUE: {
				value v = BOOL_VAL(true);
				push(v);
				break;
			}
			case OP_FALSE: {
				value v = BOOL_VAL(false);
				push(v);
				break;
			}
			case OP_NULL: {
				value v = NULL_VAL();
				push(v);
				break;
			}
			case OP_POP: pop(); break;
			case OP_ADD: BINARY_OP(+); break;
			case OP_SUBTRACT: BINARY_OP(-); break;
			case OP_MULTIPLY: BINARY_OP(*); break;
			case OP_DIVIDE: BINARY_OP(/); break;
			case OP_NEGATE: {
				value v = pop();
				if (v.type != VAL_INT && v.type != VAL_FLOAT) {
					runtimeError("Operand must be a number");
				}
				if (v.type == VAL_INT) {
					push(INTEGER_VAL(-v.as.integer));
				} else if (v.type == VAL_FLOAT) {
					push(FLOAT_VAL(-v.as.floating));
				}
				break;
			}
			case OP_EQUAL: {
				push(BOOL_VAL(equal(pop(), pop())));
				break;
			}
			case OP_NOT_EQUAL: {
				push(BOOL_VAL(!equal(pop(), pop())));
				break;
			}
			case OP_LESS: {
				push(BOOL_VAL(lessThan(pop(), pop())));
				break;
			}
			case OP_LESS_EQUAL: {
				push(BOOL_VAL(!greaterThan(pop(), pop())));
				break;
			}
			case OP_GREATER: {
				push(BOOL_VAL(greaterThan(pop(), pop())));
				break;
			}
			case OP_GREATER_EQUAL: {
				push(BOOL_VAL(!lessThan(pop(), pop())));
				break;
			}
			case OP_LIKE: {
				value pattern = pop();
				value str = pop();
				if (str.type == VAL_NULL || pattern.type == VAL_NULL) {
					push(NULL_VAL(0));
				} else {
					push(BOOL_VAL(likeMatch(str.as.text, pattern.as.text)));
				}
				break;
			}
			case OP_IS_NULL: {
				push(BOOL_VAL(pop().type == VAL_NULL));
				break;
			}
			case OP_NOT_NULL: {
				push(BOOL_VAL(!(pop().type == VAL_NULL)));
				break;
			}
			case OP_NOT: {
				push(BOOL_VAL(!pop().as.boolean));
				break;
			}
			case OP_JUMP: {
				uint16_t offset = READ_TWO_BYTES();
				vm.ip += (int16_t)offset;
				break;
			}
			case OP_JUMP_FALSE: {
				uint16_t offset = READ_TWO_BYTES();
				value v = pop();
				if ((v.type == VAL_BOOL && !v.as.boolean) || v.type == VAL_NULL) {
					vm.ip += (int16_t)offset;
				}
				break;
			}
			case OP_JUMP_TRUE: {
				uint16_t offset = READ_TWO_BYTES();
				value v = pop();
				if (v.type == VAL_BOOL && v.as.boolean) {
					vm.ip += (int16_t)offset;
				}
				break;
			}
			case OP_OPEN_SCAN: {
				value v = pop();
				openScanner(v.as.text);
				break;
			}
			case OP_CLOSE_SCAN: {
				closeScanner(&vm.scanners[0]);
				break;
			}
			case OP_NEXT: {
				uint16_t offset = READ_TWO_BYTES();
				if (!advanceScanner(&vm.scanners[0])) {
					vm.ip += (int16_t)offset;
				}
				break;
			}
			case OP_REWIND: {
				scanner* s = &vm.scanners[0];
				s->started  = false;
				s->atEnd    = false;
				s->childIdx = 0;
				s->slotIdx  = 0;
				break;
			}
			case OP_COLUMN: {
				uint8_t col_idx = READ_BYTE();
				scanner* s = &vm.scanners[0];
				sp_slot slot = s->page->slots[s->slotIdx];
				entry e = s->page->entries[slot.ptr + col_idx];
				value v;
				switch (e.type) {
					case T_INT: {
						int32_t raw;
						memcpy(&raw, e.data, sizeof(int32_t));
						v.type = VAL_INT;
						v.as.integer = (int64_t)raw;
						break;
					}
					case T_STRING:
					case T_DATE:
					case T_TIME: {
						char* str = malloc(e.size + 1);
						memcpy(str, e.data, e.size);
						str[e.size] = '\0';
						v.type = VAL_TEXT;
						v.as.text = str;
						break;
					}
				}
				push(v);
				break;
			}
			case OP_EMIT_ROW: {
				uint8_t count = READ_BYTE();
				// pop in reverse so row[0] is the leftmost column
				value* row = malloc(count * sizeof(value));
				for (int i = count - 1; i >= 0; i--) row[i] = pop();
				for (int i = 0; i < count; i++) {
					if (i > 0) printf(" | ");
					printValue(row[i]);
				}
				printf("\n");
				// grow result buffer
				if (vm.results.capacity == 0) {
					vm.results.capacity = 8;
					vm.results.rows = malloc(vm.results.capacity * sizeof(value*));
					vm.results.cols = count;
				} else if (vm.results.count == vm.results.capacity) {
					vm.results.capacity *= 2;
					vm.results.rows = realloc(vm.results.rows, vm.results.capacity * sizeof(value*));
				}
				// transfer ownership of the row array (including any VAL_TEXT pointers) to vm.results
				vm.results.rows[vm.results.count++] = row;
				break;
			}
			case OP_INSERT_ROW: {
				uint8_t count = READ_BYTE();
				scanner* s = &vm.scanners[0];
				table* t = s->tbl;
				// stack top is the last column, so pop in reverse to preserve column order
				entry* entries = malloc(count * sizeof(entry));
				uint32_t totalSize = 0;
				for (int i = count - 1; i >= 0; i--) {
					entries[i] = valueToEntry(pop());
					totalSize += entries[i].size;
				}
				// find the last page in the tree by reading maxPageNumber from the root node
				node rootNode;
				readNode(t->root, &rootNode, t);
				address pageAddr = findPage(rootNode.maxPageNumber, t);
				loadPage(pageAddr, t);
				slotted_page* page = t->page;
				sp_record r = { .entries = entries, .len = count, .size = totalSize };
				if (hasSpace(page, totalSize)) {
					uint32_t slotID = (page->header.numRecords > 0)
						? page->slots[page->header.numRecords - 1].ID + 1
						: 1;
					addRecord(page, slotID, r);
					markPage(pageAddr, page, t);
				} else {
					// allocate a new page; copy page dimensions from the full page
					uint32_t newPageNum = rootNode.maxPageNumber + 1;
					address newAddr = findAndInsert(newPageNum, t);
					slotted_page* newPage = makeSPage(newPageNum,
						page->header.maxSlots,
						page->header.maxEntries,
						page->header.arrCap);
					addRecord(newPage, 1, r);
					markPage(newAddr, newPage, t);
					// newPage lives until commit writes it; freed at process cleanup
				}
				free(entries);  // page owns the data pointers; release only the metadata array
				break;
			}
			case OP_UPDATE_COL: {
				uint8_t col_idx = READ_BYTE();
				scanner* s = &vm.scanners[0];
				table* t = s->tbl;
				sp_slot slot = s->page->slots[s->slotIdx];
				entry* target = &s->page->entries[slot.ptr + col_idx];
				free(target->data);
				*target = valueToEntry(pop());
				markPage(s->pageAddr, s->page, t);
				break;
			}
			case OP_DELETE_ROW: {
				scanner* s = &vm.scanners[0];
				table* t = s->tbl;
				sp_slot slot = s->page->slots[s->slotIdx];
				deleteRecord(s->page, slot.ID);
				markPage(s->pageAddr, s->page, t);
				// step back so the next OP_NEXT's increment lands on the record that shifted into this slot
				s->slotIdx = (s->slotIdx > 0) ? s->slotIdx - 1 : (uint32_t)(-1);
				break;
			}
			case OP_CREATE_TABLE: {
				// schema entry is pre-populated in vm.schema by the compiler before execution
				uint8_t schemaIdx = READ_BYTE();
				uint32_t hash = vm.chunk->constants.values[schemaIdx].as.u32;
				schema* s = readHT(hash, vm.schema);
				if (!s) {
					printf("Error: schema not found for CREATE TABLE\n");
					break;
				}
				// if the table already exists, do nothing
				if (tableAlreadyExists(s->tablename)) {
					printf("Tried to create table %s but it already exists\n", s->tablename);
					break;
				}
				// otherwise create the table
				table* t = createTree(s->tablename, 1);
				if (t) {
					fclose(t->source);
					freeTable(t);
				}
				saveSchema(vm.schema);
				break;
			}
			case OP_DROP_TABLE: {
				uint8_t nameIdx = READ_BYTE();
				const char* name = vm.chunk->constants.values[nameIdx].as.text;
				uint32_t hash = hashString(name, (int)strlen(name));
				deleteHT(hash, vm.schema);
				table* t = malloc(sizeof(table));
				if (loadTable((char*)name, t)) {
					deleteTable(t);  // closes file, removes .tbl, frees t
				} else {
					free(t);
					printf("Error: table '%s' not found\n", name);
				}
				saveSchema(vm.schema);
				break;
			}
			case OP_SET_RESULT: {
				vm.results.print = true;
				break;
			}
			case OP_HALT: {
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			}
		}
	}
	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
}

result_buffer interpret(const char* source) {
	chunk c;
	hashtable* schema = loadSchema();
	if (!schema) {
		vm.results.ir = INTERPRET_LOAD_ERROR;
		return vm.results;
	}
	initVM(schema);
	initChunk(&c);
	tokenized t = lexQuery(source);
	ast_node* root = compile(t);
	if (!root) {
		vm.results.ir = INTERPRET_COMPILE_ERROR;
		return vm.results;
	}
	generate(root, &c, schema);

	/*if (!compile(source, &chunk)) {
	freeChunk(&chunk);
	return INTERPRET_COMPILE_ERROR;
	}*/

	vm.chunk = &c;
	vm.ip = vm.chunk->code;

	vm.results.ir = run();

	freeChunk(&c);
	return vm.results;
}