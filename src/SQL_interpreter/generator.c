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

#include "generator.h"
#include "common.h"
#include "hashtable.h"

static uint32_t reverse_bits32(uint32_t x) {
    x = (x >> 16) | (x << 16);
    x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
    x = ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4);
    x = ((x & 0xCCCCCCCC) >> 2) | ((x & 0x33333333) << 2);
    x = ((x & 0xAAAAAAAA) >> 1) | ((x & 0x55555555) << 1);
    return x;
}

static uint32_t pkToIk(uint32_t pk, uint32_t num_internal_keys) {
    uint32_t reversed = reverse_bits32(pk);
    return (uint32_t)(((uint64_t)reversed * num_internal_keys) >> 32);
}

#define MAX_IDENT_LEN 256

/*
copies tok's source text into buf as a null-terminated string
*/
static void tokenToStr(token tok, char* buf) {
    int len = tok.length < MAX_IDENT_LEN - 1 ? tok.length : MAX_IDENT_LEN - 1;
    memcpy(buf, tok.start, len);
    buf[len] = '\0';
}

/*
looks up a table's schema entry by name; returns NULL if not found
*/
static schema* lookupSchema(const char* tname, hashtable* ht) {
    uint32_t hash = hashString(tname, (int)strlen(tname));
    return readHT(hash, ht);
}

/*
returns the 0-based index of colname within s->cols, or -1 if not found
*/
static int lookupColIdx(const char* colname, schema* s) {
    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->cols[i], colname) == 0) return i;
    }
    return -1;
}

/*
adds v to the chunk's constant pool and emits OP_CONSTANT + its index
*/
static void writeConst(chunk* c, value v) {
    uint8_t idx = (uint8_t)addConstant(c, v);
    writeChunk(c, OP_CONSTANT, 0);
    writeChunk(c, idx, 0);
}

/*
emits op followed by two 0xFF placeholder bytes for the jump offset.
returns the position of the placeholder so it can be patched later with patchJump()
*/
static int emitJump(chunk* c, opcode op) {
    writeChunk(c, op, 0);
    writeChunk(c, 0xFF, 0);
    writeChunk(c, 0xFF, 0);
    return c->count - 2;
}

/*
back-patches the 2-byte forward offset at pos to reach the current code position.
assumes OP_JUMP fix is applied (vm.ip += (int16_t)offset)
*/
static void patchJump(chunk* c, int pos) {
    int16_t jump = (int16_t)(c->count - (pos + 2));
    c->code[pos]     = (uint8_t)((uint16_t)jump >> 8);
    c->code[pos + 1] = (uint8_t)((uint16_t)jump & 0xFF);
}

/*
emits op with a signed 2-byte offset that reaches target, which may be behind current position.
assumes OP_JUMP fix is applied (vm.ip += (int16_t)offset)
*/
static void emitBackJump(chunk* c, opcode op, int target) {
    writeChunk(c, op, 0);
    int16_t jump = (int16_t)(target - (c->count + 2));
    writeChunk(c, (uint8_t)((uint16_t)jump >> 8), 0);
    writeChunk(c, (uint8_t)((uint16_t)jump & 0xFF), 0);
}

/* AST Node Types:
	TYPE_QUERY,
	TYPE_SELECT_STMT,
	TYPE_INSERT_STMT,
	TYPE_UPDATE_STMT,
	TYPE_DELETE_STMT,
	TYPE_CREATE_STMT,
	TYPE_DROP_STMT,
	TYPE_ALTER_STMT,
	TYPE_WHERE_CLAUSE,
	TYPE_GROUP_CLAUSE,
	TYPE_HAVING_CLAUSE,
	TYPE_ORDER_CLAUSE,
	TYPE_LIMIT_CLAUSE,
	TYPE_EXPR,
	TYPE_OR_EXPR,
	TYPE_AND_EXPR,
	TYPE_NOT_EXPR,
	TYPE_COMPARISON,
	TYPE_ADDITIVE,
	TYPE_MULTIPLICATIVE,
	TYPE_UNARY,
	TYPE_PRIMARY,
	TYPE_IDENTIFIER,
	TYPE_LIST_NODE,
	TYPE_SELECT_ITEM,
	TYPE_COL_DEF,
	TYPE_ASSIGNMENT

    Opcodes:
    OP_SELECT,
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LIKE,
    OP_IS_NULL,
    OP_NOT_NULL,
    OP_NOT,
    OP_JUMP,
    OP_JUMP_FALSE,
    OP_JUMP_TRUE,
    OP_OPEN_SCAN,
    OP_CLOSE_SCAN,
    OP_NEXT,
    OP_REWIND,
    OP_COLUMN,
    OP_EMIT_ROW,
    OP_INSERT_ROW,
    OP_UPDATE_COL,
    OP_DELETE_ROW,
    OP_CREATE_TABLE,
    OP_DROP_TABLE,
    OP_SORT,
    OP_LIMIT,
    OP_HALT
*/

static void munchExpr(ast_node* node, chunk* c, hashtable* schema) {
	switch(node->type) {
		case TYPE_EXPR: {
			break;
		}
		case TYPE_OR_EXPR: {
			break;
		}
		case TYPE_AND_EXPR: {
			break;
		}
		case TYPE_NOT_EXPR: {
			break;
		}
		case TYPE_COMPARISON: {
			break;
		}
		case TYPE_ADDITIVE: {
			break;
		}
		case TYPE_MULTIPLICATIVE: {
			break;
		}
		case TYPE_UNARY: {
			break;
		}
		case TYPE_PRIMARY: {
			break;
		}
	}
}

static void munchStmt(ast_node* node, chunk* c, hashtable* ht) {
	switch(node->type) {
		case TYPE_QUERY: {
			munchStmt(node->children[0], c, ht);
			writeChunk(c, OP_HALT, 0);
			break;
		}

		case TYPE_SELECT_STMT: {
			/*
			Bytecode layout:
			  CONSTANT <tname>       push table name string
			  OPEN_SCAN              open scanner on the table
			  REWIND                 reset scanner to start
			[loop_top:]
			  NEXT <exit_offset>     advance; if exhausted jump forward to exit
			  [WHERE expr]           evaluate filter condition (if present)
			  [JUMP_FALSE loop_top]  skip row if filter is false (backward)
			  [OP_COLUMN ...]        push each selected column (or matchExpr for expressions)
			  EMIT_ROW <col_count>   emit the result row
			  JUMP <loop_top>        loop back (backward)
			[exit:]
			  CLOSE_SCAN

			Requires vm.c fixes: (int16_t) cast in OP_JUMP and OP_JUMP_FALSE,
			and openScanner(v.as.text) in OP_OPEN_SCAN
			*/
			char tname[MAX_IDENT_LEN];
			tokenToStr(node->children[1]->tok, tname);
			schema* s = lookupSchema(tname, ht);

			writeConst(c, TEXT_VAL(strdup(tname)));
			writeChunk(c, OP_OPEN_SCAN, 0);
			writeChunk(c, OP_REWIND, 0);

			int loopTop = c->count;
			int nextPatch = emitJump(c, OP_NEXT);

			// WHERE: emit condition; jump back to loopTop (next row) if false
			if (node->children[2]) {
				munchExpr(node->children[2]->children[0], c, ht);
				emitBackJump(c, OP_JUMP_FALSE, loopTop);
			}

			// Emit selected columns
			int colCount = 0;
			ast_node* listIt = node->children[0];
			ast_node* firstItem = listIt ? listIt->children[0] : NULL;
			if (firstItem && firstItem->flag) {
				// SELECT *: emit OP_COLUMN for every schema column in order
				if (s) {
					for (int i = 0; i < s->count; i++) {
						writeChunk(c, OP_COLUMN, 0);
						writeChunk(c, (uint8_t)i, 0);
						colCount++;
					}
				}
			} else {
				// Named items: resolve simple column identifiers to OP_COLUMN;
				// fall back to matchExpr for computed expressions
				while (listIt) {
					ast_node* item = listIt->children[0];     // TYPE_SELECT_ITEM
					ast_node* itemExpr = item->children[0];   // the expression
					if (s && itemExpr && itemExpr->type == TYPE_PRIMARY &&
					    itemExpr->tok.type == TOKEN_IDENTIFIER) {
						char colname[MAX_IDENT_LEN];
						tokenToStr(itemExpr->tok, colname);
						int idx = lookupColIdx(colname, s);
						if (idx >= 0) {
							writeChunk(c, OP_COLUMN, 0);
							writeChunk(c, (uint8_t)idx, 0);
						}
					} else if (itemExpr) {
						munchExpr(itemExpr, c, ht);
					}
					colCount++;
					listIt = listIt->children[1];
				}
			}

			writeChunk(c, OP_EMIT_ROW, 0);
			writeChunk(c, (uint8_t)colCount, 0);
			emitBackJump(c, OP_JUMP, loopTop);

			patchJump(c, nextPatch);
			writeChunk(c, OP_CLOSE_SCAN, 0);
			break;
		}

		case TYPE_INSERT_STMT: {
			/*
			Bytecode layout:
			  CONSTANT <tname>         push table name string
			  OPEN_SCAN                open scanner
			  [val_0 ... val_n-1]      evaluate each value expression in order
			  INSERT_ROW <val_count>   pop val_count values and insert as new row
			  CLOSE_SCAN

			Note: if an explicit column list is present (node->flag == true), values
			are pushed in the order written and assumed to be in schema column order.
			Reordering to match an arbitrary column list is not yet supported.
			*/
			char tname[MAX_IDENT_LEN];
			tokenToStr(node->children[0]->tok, tname);

			writeConst(c, TEXT_VAL(strdup(tname)));
			writeChunk(c, OP_OPEN_SCAN, 0);

			int valCount = 0;
			ast_node* valIt = node->children[2];
			while (valIt) {
				munchExpr(valIt->children[0], c, ht);
				valCount++;
				valIt = valIt->children[1];
			}

			writeChunk(c, OP_INSERT_ROW, 0);
			writeChunk(c, (uint8_t)valCount, 0);
			writeChunk(c, OP_CLOSE_SCAN, 0);
			break;
		}

		case TYPE_UPDATE_STMT: {
			/*
			Bytecode layout:
			  CONSTANT <tname>       push table name string
			  OPEN_SCAN              open scanner
			  REWIND
			[loop_top:]
			  NEXT <exit_offset>     advance; jump to exit if done
			  [WHERE expr]           evaluate filter (if present)
			  [JUMP_FALSE loop_top]  skip row if filter is false
			  [for each assignment:]
			    [new_value expr]     evaluate right-hand side
			    UPDATE_COL <idx>     replace column at idx in current slot
			  JUMP <loop_top>        loop back
			[exit:]
			  CLOSE_SCAN
			*/
			char tname[MAX_IDENT_LEN];
			tokenToStr(node->children[0]->tok, tname);
			schema* s = lookupSchema(tname, ht);

			writeConst(c, TEXT_VAL(strdup(tname)));
			writeChunk(c, OP_OPEN_SCAN, 0);
			writeChunk(c, OP_REWIND, 0);

			int loopTop = c->count;
			int nextPatch = emitJump(c, OP_NEXT);

			if (node->children[2]) {
				munchExpr(node->children[2]->children[0], c, ht);
				emitBackJump(c, OP_JUMP_FALSE, loopTop);
			}

			ast_node* asgmtIt = node->children[1];
			while (asgmtIt) {
				ast_node* asgmt = asgmtIt->children[0];  // TYPE_ASSIGNMENT
				munchExpr(asgmt->children[1], c, ht);    // new value expression
				char colname[MAX_IDENT_LEN];
				tokenToStr(asgmt->children[0]->tok, colname);
				int idx = s ? lookupColIdx(colname, s) : 0;
				writeChunk(c, OP_UPDATE_COL, 0);
				writeChunk(c, (uint8_t)idx, 0);
				asgmtIt = asgmtIt->children[1];
			}

			emitBackJump(c, OP_JUMP, loopTop);
			patchJump(c, nextPatch);
			writeChunk(c, OP_CLOSE_SCAN, 0);
			break;
		}

		case TYPE_DELETE_STMT: {
			/*
			Bytecode layout:
			  CONSTANT <tname>       push table name string
			  OPEN_SCAN              open scanner
			  REWIND
			[loop_top:]
			  NEXT <exit_offset>     advance; jump to exit if done
			  [WHERE expr]           evaluate filter (if present)
			  [JUMP_FALSE loop_top]  skip row if filter is false
			  DELETE_ROW             delete current slot (vm steps slotIdx back)
			  JUMP <loop_top>        loop back
			[exit:]
			  CLOSE_SCAN
			*/
			char tname[MAX_IDENT_LEN];
			tokenToStr(node->children[0]->tok, tname);

			writeConst(c, TEXT_VAL(strdup(tname)));
			writeChunk(c, OP_OPEN_SCAN, 0);
			writeChunk(c, OP_REWIND, 0);

			int loopTop = c->count;
			int nextPatch = emitJump(c, OP_NEXT);

			if (node->children[1]) {
				munchExpr(node->children[1]->children[0], c, ht);
				emitBackJump(c, OP_JUMP_FALSE, loopTop);
			}

			writeChunk(c, OP_DELETE_ROW, 0);
			emitBackJump(c, OP_JUMP, loopTop);

			patchJump(c, nextPatch);
			writeChunk(c, OP_CLOSE_SCAN, 0);
			break;
		}

		case TYPE_CREATE_STMT: {
			if (node->tok.type == TOKEN_TABLE) {
				/*
				Bytecode layout:
				  CREATE_TABLE <schemaIdx>   look up schema in vm.schema by hash, then createTable

				The schema entry is built here and inserted into ht (which must alias vm.schema)
				so it is available to the VM at runtime when OP_CREATE_TABLE executes.
				*/
				char tname[MAX_IDENT_LEN];
				tokenToStr(node->children[0]->tok, tname);

				// Count columns in the col_def list
				int colCount = 0;
				ast_node* it = node->children[1];
				while (it) { colCount++; it = it->children[1]; }

				// Build schema entry and register it for the VM to find at runtime
				schema* s = malloc(sizeof(schema));
				s->tablename = strdup(tname);
				s->hash = hashString(tname, (int)strlen(tname));
				s->count = colCount;
				s->cols = malloc(colCount * sizeof(char*));
				it = node->children[1];
				for (int i = 0; i < colCount; i++) {
					ast_node* def = it->children[0];  // TYPE_COL_DEF
					char colname[MAX_IDENT_LEN];
					tokenToStr(def->children[0]->tok, colname);
					s->cols[i] = strdup(colname);
					it = it->children[1];
				}
				insertHT(s, ht);

				uint8_t schemaIdx = (uint8_t)addConstant(c, UINT_VAL(s->hash));
				writeChunk(c, OP_CREATE_TABLE, 0);
				writeChunk(c, schemaIdx, 0);
			} else {
				// CREATE INDEX and CREATE VIEW have no corresponding opcodes yet
				printf("Error: CREATE INDEX and CREATE VIEW are not yet supported\n");
			}
			break;
		}

		case TYPE_DROP_STMT: {
			if (node->tok.type == TOKEN_TABLE) {
				/*
				Bytecode layout:
				  DROP_TABLE <nameIdx>   reads name string from constants,
				                         removes schema from vm.schema, deletes B+ tree file

				Note: deleteHT is called by OP_DROP_TABLE in vm.c at runtime; do not
				call it here or the VM's lookup will fail.
				*/
				char tname[MAX_IDENT_LEN];
				tokenToStr(node->children[0]->tok, tname);

				uint8_t nameIdx = (uint8_t)addConstant(c, TEXT_VAL(strdup(tname)));
				writeChunk(c, OP_DROP_TABLE, 0);
				writeChunk(c, nameIdx, 0);
			} else {
				// DROP INDEX, VIEW, DATABASE have no corresponding opcodes yet
				printf("Error: DROP INDEX, DROP VIEW, and DROP DATABASE are not yet supported\n");
			}
			break;
		}

		case TYPE_ALTER_STMT: {
			printf("Error: ALTER TABLE is not yet supported\n");
			break;
		}

		case TYPE_WHERE_CLAUSE:
		case TYPE_HAVING_CLAUSE: {
			// Emit the filter expression; leaves a bool on the stack
			munchExpr(node->children[0], c, ht);
			break;
		}

		case TYPE_GROUP_CLAUSE:
		case TYPE_ORDER_CLAUSE:
		case TYPE_LIMIT_CLAUSE: {
			// Not yet implemented, require OP_LIMIT, OP_SORT
			break;
		}

		default:
			break;
	}
}

/*
takes the given AST and converts it to bytecode, filling a chunk
*/
void generate(ast_node* root, chunk* c, hashtable* schema) {
	munchStmt(root, c, schema);
}