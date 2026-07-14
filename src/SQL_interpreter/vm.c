#include "../common.h"
#include "../debug.h"
#include "parser.h"
#include "vm.h"

VM vm;

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

static void resetStack(){
	vm.stackTop = vm.stack;
}

void initVM() {
	resetStack();
	for (int i = 0; i < MAX_SCANNERS; i++) {
		vm.scanners[i].open = false;
		vm.scanners[i].tbl = NULL;
	}
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

/*
Push value to VM's stack
*/
void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

/*
Pop value from top of VM's stack
*/
Value pop() {
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

static interpret_result run() {
	#define READ_BYTE() (*vm.ip++)
	#define READ_TWO_BYTES() (((uint16_t) *vm.ip++ << 8) | (uint16_t) *vm.ip++)
	#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
	// somewhat dubious macro that takes operators as arguments
	#define BINARY_OP(op) \
		do { \
			int64_t b = pop().as.integer; \
			int64_t a = pop().as.integer; \
			push(INTEGER_VAL(a op b)); \
		} while (false)

	for (;;) {
		#ifdef DEBUG_TRACE_EXECUTION
			printf("        ");
			for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
				printf("[ ");
				printValue(*slot);
				printf(" ]");
			}
			printf("\n");
			disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
		#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_HALT: {
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			}
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			case OP_ADD: BINARY_OP(+); break;
			case OP_SUBTRACT: BINARY_OP(-); break;
			case OP_MULTIPLY: BINARY_OP(*); break;
			case OP_DIVIDE: BINARY_OP(/); break;
			case OP_NEGATE: {
				Value v = pop();
				if (v.type != VAL_INT && v.type != VAL_FLOAT) {
					printf("Error: attempted to negate a non-number at bytecode level\n");
				}
				if (v.type == VAL_INT) {
					push(INTEGER_VAL(-v.as.integer));
				} else if (v.type == VAL_FLOAT) {
					push(FLOAT_VAL(v.as.floating));
				}
				break;
			}
			case OP_EQUAL: {
				break;
			}
			case OP_NOT_EQUAL: {
				break;
			}
			case OP_LESS: {
				break;
			}
			case OP_LESS_EQUAL: {
				break;
			}
			case OP_GREATER: {
				break;
			}
			case OP_GREATER_EQUAL: {
				break;
			}
			case OP_LIKE: {
				break;
			}
			case OP_IS_NULL: {
				break;
			}
			case OP_NOT_NULL: {
				break;
			}
			case OP_NOT: {
				push(!pop().as.boolean);
				break;
			}
			case OP_OPEN_SCAN: {
				Value v = pop();
				openScanner(*v.as.text);
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
				Value v;
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
		}
	}
	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
}

interpret_result interpret(const char* source) {
	Chunk chunk;
	initChunk(&chunk);

	if (!compile(source, &chunk)) {
	freeChunk(&chunk);
	return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	interpret_result result = run();

	freeChunk(&chunk);
	return result;
}