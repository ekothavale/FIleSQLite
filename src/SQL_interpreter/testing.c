#include "testing.h"

// ##########################################################################################################################################
// ##########################################################################################################################################
// Chunk Tests

// --- initChunk ---

void test_init_chunk() {
    chunk c;
    initChunk(&c);
    assert(c.code     == NULL);
    assert(c.lines    == NULL);
    assert(c.capacity == 0);
    assert(c.count    == 0);
    assert(c.constants.count == 0);
}

// --- writeChunk ---

void test_write_chunk_stores_byte() {
    chunk c;
    initChunk(&c);
    writeChunk(&c, OP_HALT, 1);
    assert(c.count    == 1);
    assert(c.code[0]  == OP_HALT);
    assert(c.capacity >= 1);
    freeChunk(&c);
}

void test_write_chunk_stores_line() {
    chunk c;
    initChunk(&c);
    writeChunk(&c, OP_ADD, 42);
    assert(c.lines    != NULL);
    assert(c.lines[0] == 42);
    assert(c.count    == 1);
    freeChunk(&c);
}

void test_write_chunk_multiple() {
    chunk c;
    initChunk(&c);
    writeChunk(&c, OP_CONSTANT, 1);
    writeChunk(&c, 0,           1);  // operand byte
    writeChunk(&c, OP_HALT,     1);
    assert(c.count    == 3);
    assert(c.code[0]  == OP_CONSTANT);
    assert(c.code[1]  == 0);
    assert(c.code[2]  == OP_HALT);
    freeChunk(&c);
}

void test_write_chunk_triggers_grow() {
    // GROW_CAPACITY(0)=8, so bytes 1-8 fill the first allocation;
    // the 9th write triggers GROW_CAPACITY(8)=16
    chunk c;
    initChunk(&c);
    for (int i = 0; i < 9; i++) writeChunk(&c, OP_POP, i + 1);
    assert(c.count    == 9);
    assert(c.capacity == 16);
    assert(c.code[8]  == OP_POP);
    freeChunk(&c);
}

// --- addConstant ---

void test_add_constant_returns_index() {
    chunk c;
    initChunk(&c);
    int i0 = addConstant(&c, INTEGER_VAL(10));
    int i1 = addConstant(&c, INTEGER_VAL(20));
    assert(i0 == 0);
    assert(i1 == 1);
    assert(c.constants.count == 2);
    freeChunk(&c);
}

void test_add_constant_stores_value() {
    chunk c;
    initChunk(&c);
    addConstant(&c, INTEGER_VAL(99));
    assert(c.constants.count              == 1);
    assert(c.constants.values[0].type     == VAL_INT);
    assert(c.constants.values[0].as.integer == 99);
    freeChunk(&c);
}

void test_add_constant_multiple() {
    chunk c;
    initChunk(&c);
    addConstant(&c, BOOL_VAL(true));
    addConstant(&c, FLOAT_VAL(3.14));
    addConstant(&c, NULL_VAL(0));
    assert(c.constants.count          == 3);
    assert(c.constants.values[0].type == VAL_BOOL);
    assert(c.constants.values[1].type == VAL_FLOAT);
    assert(c.constants.values[2].type == VAL_NULL);
    freeChunk(&c);
}

// --- freeChunk ---

void test_free_chunk_resets_code() {
    chunk c;
    initChunk(&c);
    writeChunk(&c, OP_ADD,  1);
    writeChunk(&c, OP_HALT, 1);
    freeChunk(&c);
    assert(c.code     == NULL);
    assert(c.lines    == NULL);
    assert(c.count    == 0);
    assert(c.capacity == 0);
}

void test_free_chunk_clears_constants() {
    chunk c;
    initChunk(&c);
    addConstant(&c, INTEGER_VAL(1));
    addConstant(&c, BOOL_VAL(false));
    freeChunk(&c);
    assert(c.constants.count    == 0);
    assert(c.constants.capacity == 0);
    assert(c.constants.values   == NULL);
}

void test_free_chunk_idempotent() {
    // freeChunk on a never-written chunk must not crash
    // (FREE_ARRAY with capacity=0 calls free(NULL) which is safe)
    chunk c;
    initChunk(&c);
    freeChunk(&c);
    assert(c.code  == NULL);
    assert(c.count == 0);
    assert(c.constants.values == NULL);
}

// --- master ---

void test_chunk() {
    test_init_chunk();
    test_write_chunk_stores_byte();
    test_write_chunk_stores_line();
    test_write_chunk_multiple();
    test_write_chunk_triggers_grow();
    test_add_constant_returns_index();
    test_add_constant_stores_value();
    test_add_constant_multiple();
    test_free_chunk_resets_code();
    test_free_chunk_clears_constants();
    test_free_chunk_idempotent();
    printf("All chunk tests passed.\n");
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// Value Tests

// --- initValueArray ---

void test_init_value_array() {
    ValueArray arr;
    initValueArray(&arr);
    assert(arr.values   == NULL);
    assert(arr.capacity == 0);
    assert(arr.count    == 0);
}

// --- writeValueArray ---

void test_write_value_array_stores_value() {
    ValueArray arr;
    initValueArray(&arr);
    writeValueArray(&arr, INTEGER_VAL(7));
    assert(arr.count               == 1);
    assert(arr.values[0].type      == VAL_INT);
    assert(arr.values[0].as.integer == 7);
    freeValueArray(&arr);
}

void test_write_value_array_multiple_types() {
    ValueArray arr;
    initValueArray(&arr);
    writeValueArray(&arr, BOOL_VAL(true));
    writeValueArray(&arr, FLOAT_VAL(1.5));
    writeValueArray(&arr, NULL_VAL(0));
    assert(arr.count                  == 3);
    assert(arr.values[0].type         == VAL_BOOL);
    assert(arr.values[0].as.boolean   == true);
    assert(arr.values[1].type         == VAL_FLOAT);
    assert(arr.values[2].type         == VAL_NULL);
    freeValueArray(&arr);
}

void test_write_value_array_triggers_grow() {
    // GROW_CAPACITY(0)=8 on first write; 9th write causes GROW_CAPACITY(8)=16
    ValueArray arr;
    initValueArray(&arr);
    for (int i = 0; i < 9; i++) writeValueArray(&arr, INTEGER_VAL(i));
    assert(arr.count                == 9);
    assert(arr.capacity             == 16);
    assert(arr.values[8].as.integer == 8);
    freeValueArray(&arr);
}

// --- freeValueArray ---

void test_free_value_array_resets() {
    ValueArray arr;
    initValueArray(&arr);
    writeValueArray(&arr, INTEGER_VAL(1));
    writeValueArray(&arr, BOOL_VAL(false));
    freeValueArray(&arr);
    assert(arr.values   == NULL);
    assert(arr.count    == 0);
    assert(arr.capacity == 0);
}

void test_free_value_array_idempotent() {
    // freeValueArray on a never-written array must not crash
    ValueArray arr;
    initValueArray(&arr);
    freeValueArray(&arr);
    assert(arr.values   == NULL);
    assert(arr.count    == 0);
    assert(arr.capacity == 0);
}

// --- printValue ---
// printValue writes to stdout via printf("%g", value.as.floating).
// We redirect fd 1 through a tmpfile to capture and inspect the output.

static char* capture_printValue(value v) {
    FILE* tmp = tmpfile();
    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    dup2(fileno(tmp), STDOUT_FILENO);
    printValue(v);
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    rewind(tmp);
    static char buf[128];
    size_t n = fread(buf, 1, sizeof(buf) - 1, tmp);
    buf[n] = '\0';
    fclose(tmp);
    return buf;
}

void test_print_value_float() {
    char* out = capture_printValue(FLOAT_VAL(3.14));
    assert(strlen(out)        >  0);
    assert(strstr(out, "3.14") != NULL);
    assert(out[0]             != '\0');
}

void test_print_value_zero() {
    char* out = capture_printValue(FLOAT_VAL(0.0));
    assert(strlen(out)      >  0);
    assert(strstr(out, "0") != NULL);
    assert(out[0]           != '\0');
}

void test_print_value_large() {
    char* out = capture_printValue(FLOAT_VAL(1000.0));
    assert(strlen(out)         >  0);
    assert(strstr(out, "1000") != NULL);
    assert(out[0]              != '\0');
}

// --- master ---

void test_value() {
    test_init_value_array();
    test_write_value_array_stores_value();
    test_write_value_array_multiple_types();
    test_write_value_array_triggers_grow();
    test_free_value_array_resets();
    test_free_value_array_idempotent();
    test_print_value_float();
    test_print_value_zero();
    test_print_value_large();
    printf("All value tests passed.\n");
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// Hash table Tests

// ##########################################################################################################################################
// ##########################################################################################################################################
// Schema Tests

// ##########################################################################################################################################
// ##########################################################################################################################################
// Lexer tests

// --- initLexer / scanToken ---

void test_scan_single_char_tokens() {
    initLexer("(,;.+-*/{}");
    assert(scanToken().type == TOKEN_LEFT_PAREN);
    assert(scanToken().type == TOKEN_COMMA);
    assert(scanToken().type == TOKEN_SEMICOLON);
    assert(scanToken().type == TOKEN_DOT);
    assert(scanToken().type == TOKEN_PLUS);
    assert(scanToken().type == TOKEN_MINUS);
    assert(scanToken().type == TOKEN_STAR);
    assert(scanToken().type == TOKEN_SLASH);
    assert(scanToken().type == TOKEN_LEFT_BRACE);
    assert(scanToken().type == TOKEN_RIGHT_BRACE);
    assert(scanToken().type == TOKEN_EOF);
}

void test_scan_two_char_tokens() {
    initLexer("!= == <= >=");
    token t;
    t = scanToken(); assert(t.type == TOKEN_BANG_EQUAL  && t.length == 2);
    t = scanToken(); assert(t.type == TOKEN_EQUAL_EQUAL && t.length == 2);
    t = scanToken(); assert(t.type == TOKEN_LESS_EQUAL  && t.length == 2);
    t = scanToken(); assert(t.type == TOKEN_GREATER_EQUAL && t.length == 2);
    assert(scanToken().type == TOKEN_EOF);
}

void test_scan_single_ops() {
    // Single-char variants when the second char doesn't match
    initLexer("! = < >");
    assert(scanToken().type == TOKEN_BANG);
    assert(scanToken().type == TOKEN_EQUAL);
    assert(scanToken().type == TOKEN_LESS);
    assert(scanToken().type == TOKEN_GREATER);
    assert(scanToken().type == TOKEN_EOF);
}

void test_scan_keywords() {
    initLexer("select from where between truncate");
    assert(scanToken().type == TOKEN_SELECT);
    assert(scanToken().type == TOKEN_FROM);
    assert(scanToken().type == TOKEN_WHERE);
    assert(scanToken().type == TOKEN_BETWEEN);
    assert(scanToken().type == TOKEN_TRUNCATE);
    assert(scanToken().type == TOKEN_EOF);
}

void test_scan_identifier() {
    // Near-misses that must not match keywords
    initLexer("selector fromage wheres myTable");
    token t;
    t = scanToken(); assert(t.type == TOKEN_IDENTIFIER && t.length == 8);
    t = scanToken(); assert(t.type == TOKEN_IDENTIFIER && t.length == 7);
    t = scanToken(); assert(t.type == TOKEN_IDENTIFIER && t.length == 6);
    t = scanToken(); assert(t.type == TOKEN_IDENTIFIER && strncmp(t.start, "myTable", 7) == 0);
    assert(scanToken().type == TOKEN_EOF);
}

// --- number() ---

void test_scan_number_integer() {
    initLexer("42");
    token t = scanToken();
    assert(t.type   == TOKEN_NUMBER);
    assert(t.length == 2);
    assert(strncmp(t.start, "42", 2) == 0);
}

void test_scan_number_float() {
    initLexer("3.14");
    token t = scanToken();
    assert(t.type   == TOKEN_NUMBER);
    assert(t.length == 4);
    assert(strncmp(t.start, "3.14", 4) == 0);
}

void test_scan_number_no_trailing_decimal() {
    // "3." → NUMBER("3") then DOT (no fractional digit after dot)
    initLexer("3.");
    token t = scanToken();
    assert(t.type   == TOKEN_NUMBER);
    assert(t.length == 1);
    assert(scanToken().type == TOKEN_DOT);
}

// --- string() ---

void test_scan_string() {
    initLexer("'hello'");
    token t = scanToken();
    assert(t.type   == TOKEN_STRING);
    assert(t.length == 7);
    assert(strncmp(t.start, "'hello'", 7) == 0);
}

void test_scan_empty_string() {
    initLexer("''");
    token t = scanToken();
    assert(t.type   == TOKEN_STRING);
    assert(t.length == 2);
}

void test_scan_unterminated_string() {
    initLexer("'hello");
    token t = scanToken();
    assert(t.type == TOKEN_ERROR);
    assert(strncmp(t.start, "Unterminated string.", t.length) == 0);
}

// --- skipWhitespace / line tracking ---

void test_scan_skips_whitespace() {
    initLexer("   \t\r select");
    token t = scanToken();
    assert(t.type   == TOKEN_SELECT);
    assert(t.length == 6);
}

void test_scan_skips_comment() {
    // Everything after '--' until newline is skipped
    initLexer("-- this is a comment\nselect");
    token t = scanToken();
    assert(t.type == TOKEN_SELECT);
    assert(t.line == 2);
}

void test_scan_line_tracking() {
    initLexer("select\nfrom\ntable");
    token t;
    t = scanToken(); assert(t.type == TOKEN_SELECT && t.line == 1);
    t = scanToken(); assert(t.type == TOKEN_FROM   && t.line == 2);
    t = scanToken(); assert(t.type == TOKEN_TABLE  && t.line == 3);
}

// --- initTokenized ---

void test_init_tokenized() {
    tokenized t;
    initTokenized(&t);
    assert(t.capacity == INITIAL_TOKEN_COUNT);
    assert(t.count    == 0);
    assert(t.tokens   != NULL);
    free(t.tokens);
}

// --- addToken ---

void test_add_token_count() {
    tokenized t;
    initTokenized(&t);
    token tok = { TOKEN_SELECT, "select", 6, 1 };
    addToken(&t, tok);
    assert(t.count == 1);
    free(t.tokens);
}

void test_add_token_stored() {
    tokenized t;
    initTokenized(&t);
    token tok = { TOKEN_FROM, "from", 4, 2 };
    addToken(&t, tok);
    assert(t.tokens[0].type   == TOKEN_FROM);
    assert(t.tokens[0].length == 4);
    assert(t.tokens[0].line   == 2);
    free(t.tokens);
}

void test_add_multiple_tokens() {
    tokenized t;
    initTokenized(&t);
    token tok1 = { TOKEN_SELECT,     "select", 6, 1 };
    token tok2 = { TOKEN_STAR,       "*",      1, 1 };
    token tok3 = { TOKEN_IDENTIFIER, "users",  5, 1 };
    addToken(&t, tok1);
    addToken(&t, tok2);
    addToken(&t, tok3);
    assert(t.count == 3);
    assert(t.tokens[0].type == TOKEN_SELECT);
    assert(t.tokens[1].type == TOKEN_STAR);
    assert(t.tokens[2].type == TOKEN_IDENTIFIER);
    free(t.tokens);
}

// --- lexQuery ---

void test_lex_query_simple() {
    // keywords are lowercase-only; uppercase letters become TOKEN_IDENTIFIER
    tokenized t = lexQuery("select * from users;");
    assert(t.count == 6); // select, *, from, users, ;, EOF
    assert(t.tokens[0].type == TOKEN_SELECT);
    assert(t.tokens[1].type == TOKEN_STAR);
    assert(t.tokens[2].type == TOKEN_FROM);
    assert(t.tokens[3].type == TOKEN_IDENTIFIER);
    assert(t.tokens[4].type == TOKEN_SEMICOLON);
    assert(t.tokens[5].type == TOKEN_EOF);
    free(t.tokens);
}

void test_lex_query_empty() {
    tokenized t = lexQuery("");
    assert(t.count == 1);
    assert(t.tokens[0].type == TOKEN_EOF);
    free(t.tokens);
}

void test_lex_query_string_and_number() {
    tokenized t = lexQuery("42 'hello'");
    assert(t.count == 3); // 42, 'hello', EOF
    assert(t.tokens[0].type == TOKEN_NUMBER);
    assert(t.tokens[1].type == TOKEN_STRING);
    assert(t.tokens[2].type == TOKEN_EOF);
    free(t.tokens);
}

// --- master ---

void test_lexer() {
    test_scan_single_char_tokens();
    test_scan_two_char_tokens();
    test_scan_single_ops();
    test_scan_keywords();
    test_scan_identifier();
    test_scan_number_integer();
    test_scan_number_float();
    test_scan_number_no_trailing_decimal();
    test_scan_string();
    test_scan_empty_string();
    test_scan_unterminated_string();
    test_scan_skips_whitespace();
    test_scan_skips_comment();
    test_scan_line_tracking();
    test_init_tokenized();
    test_add_token_count();
    test_add_token_stored();
    test_add_multiple_tokens();
    test_lex_query_simple();
    test_lex_query_empty();
    test_lex_query_string_and_number();
    printf("All lexer tests passed.\n");
}


// ##########################################################################################################################################
// ##########################################################################################################################################
// Parser tests

static void freeAST(ast_node* node) {
    if (!node) return;
    for (int i = 0; i < 7; i++) freeAST(node->children[i]);
    free(node);
}

// Convenience: lex a query and compile it in one call.
// Caller must freeAST(ast) and free(t.tokens) after use.
#define COMPILE(src, t, ast) \
    tokenized t = lexQuery(src); \
    ast_node* ast = compile(t)

// --- compile: SELECT ---

void test_parse_select_star() {
    COMPILE("select * from users", t, ast);
    assert(ast->type == TYPE_QUERY);
    ast_node* sel = ast->children[0];
    assert(sel->type == TYPE_SELECT_STMT);
    // select list is a listNode whose first child is the star item (flag=true)
    assert(sel->children[0]->children[0]->flag == true);
    freeAST(ast); free(t.tokens);
}

void test_parse_select_columns() {
    COMPILE("select name, age from users", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->type == TYPE_SELECT_STMT);
    ast_node* first = sel->children[0];
    assert(first->type == TYPE_LIST_NODE);
    assert(first->children[0]->type == TYPE_SELECT_ITEM);
    assert(first->children[1] != NULL);  // second column present
    freeAST(ast); free(t.tokens);
}

void test_parse_select_where() {
    COMPILE("select * from users where id = 1", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->children[2] != NULL);
    assert(sel->children[2]->type == TYPE_WHERE_CLAUSE);
    ast_node* cmp = sel->children[2]->children[0];
    assert(cmp->type == TYPE_COMPARISON);
    assert(cmp->tok.type == TOKEN_EQUAL);
    freeAST(ast); free(t.tokens);
}

void test_parse_select_distinct() {
    COMPILE("select distinct * from users", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->type == TYPE_SELECT_STMT);
    assert(sel->flag == true);                        // DISTINCT
    assert(sel->children[1]->type == TYPE_IDENTIFIER);
    freeAST(ast); free(t.tokens);
}

void test_parse_select_limit() {
    COMPILE("select * from users limit 10", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->children[6] != NULL);
    assert(sel->children[6]->type == TYPE_LIMIT_CLAUSE);
    assert(sel->children[6]->tok.type == TOKEN_NUMBER);
    assert(strncmp(sel->children[6]->tok.start, "10", 2) == 0);
    freeAST(ast); free(t.tokens);
}

void test_parse_select_order_by() {
    COMPILE("select * from users order by age desc", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->children[5] != NULL);
    assert(sel->children[5]->type == TYPE_ORDER_CLAUSE);
    ast_node* item = sel->children[5]->children[0];
    assert(item->type == TYPE_LIST_NODE);
    assert(item->flag == true);   // DESC
    freeAST(ast); free(t.tokens);
}

void test_parse_select_group_by() {
    COMPILE("select * from users group by dept having count > 1", t, ast);
    ast_node* sel = ast->children[0];
    assert(sel->children[3] != NULL);
    assert(sel->children[3]->type == TYPE_GROUP_CLAUSE);
    assert(sel->children[4] != NULL);
    assert(sel->children[4]->type == TYPE_HAVING_CLAUSE);
    freeAST(ast); free(t.tokens);
}

void test_parse_select_expr_alias() {
    COMPILE("select name as n from users", t, ast);
    ast_node* sel = ast->children[0];
    ast_node* sel_item = sel->children[0]->children[0];  // listNode → selectItem
    assert(sel_item->type == TYPE_SELECT_ITEM);
    assert(sel_item->flag == false);           // not a star
    assert(sel_item->children[1] != NULL);     // alias present
    assert(sel_item->children[1]->type == TYPE_IDENTIFIER);
    freeAST(ast); free(t.tokens);
}

// --- compile: INSERT ---

void test_parse_insert_with_cols() {
    COMPILE("insert into users (id, name) values (1, 'alice')", t, ast);
    ast_node* ins = ast->children[0];
    assert(ins->type == TYPE_INSERT_STMT);
    assert(ins->flag == true);               // explicit column list
    assert(ins->children[1] != NULL);        // col list
    assert(ins->children[2] != NULL);        // val list
    freeAST(ast); free(t.tokens);
}

void test_parse_insert_without_cols() {
    COMPILE("insert into users values (1, 'alice')", t, ast);
    ast_node* ins = ast->children[0];
    assert(ins->type == TYPE_INSERT_STMT);
    assert(ins->flag == false);              // no explicit column list
    assert(ins->children[1] == NULL);        // no col list
    assert(ins->children[2] != NULL);        // val list present
    freeAST(ast); free(t.tokens);
}

// --- compile: UPDATE ---

void test_parse_update() {
    COMPILE("update users set name = 'alice' where id = 1", t, ast);
    ast_node* upd = ast->children[0];
    assert(upd->type == TYPE_UPDATE_STMT);
    assert(upd->children[0]->type == TYPE_IDENTIFIER);  // table name
    assert(upd->children[1] != NULL);       // assignment list
    assert(upd->children[2] != NULL);       // where clause
    freeAST(ast); free(t.tokens);
}

void test_parse_update_no_where() {
    COMPILE("update users set active = 0", t, ast);
    ast_node* upd = ast->children[0];
    assert(upd->type == TYPE_UPDATE_STMT);
    assert(upd->children[1] != NULL);       // assignment list
    assert(upd->children[2] == NULL);       // no where clause
    ast_node* asgn = upd->children[1]->children[0];
    assert(asgn->type == TYPE_ASSIGNMENT);
    freeAST(ast); free(t.tokens);
}

// --- compile: DELETE ---

void test_parse_delete_with_where() {
    COMPILE("delete from users where id = 1", t, ast);
    ast_node* del = ast->children[0];
    assert(del->type == TYPE_DELETE_STMT);
    assert(del->children[0]->type == TYPE_IDENTIFIER);
    assert(del->children[1] != NULL);
    assert(del->children[1]->type == TYPE_WHERE_CLAUSE);
    freeAST(ast); free(t.tokens);
}

void test_parse_delete_no_where() {
    COMPILE("delete from users", t, ast);
    ast_node* del = ast->children[0];
    assert(del->type == TYPE_DELETE_STMT);
    assert(del->children[0]->type == TYPE_IDENTIFIER);
    assert(del->children[1] == NULL);        // no where clause
    freeAST(ast); free(t.tokens);
}

// --- compile: CREATE ---

void test_parse_create_table() {
    COMPILE("create table users (id int, name text)", t, ast);
    ast_node* crt = ast->children[0];
    assert(crt->type == TYPE_CREATE_STMT);
    assert(crt->tok.type == TOKEN_TABLE);
    assert(crt->children[0]->type == TYPE_IDENTIFIER);  // table name
    ast_node* first_col = crt->children[1]->children[0];
    assert(first_col->type == TYPE_COL_DEF);
    freeAST(ast); free(t.tokens);
}

void test_parse_create_index() {
    COMPILE("create index idx on users (id)", t, ast);
    ast_node* crt = ast->children[0];
    assert(crt->type == TYPE_CREATE_STMT);
    assert(crt->tok.type == TOKEN_INDEX);
    assert(crt->flag == false);              // not UNIQUE
    assert(crt->children[0]->type == TYPE_IDENTIFIER);  // index name
    freeAST(ast); free(t.tokens);
}

void test_parse_create_unique_index() {
    COMPILE("create unique index idx on users (id)", t, ast);
    ast_node* crt = ast->children[0];
    assert(crt->type == TYPE_CREATE_STMT);
    assert(crt->tok.type == TOKEN_INDEX);
    assert(crt->flag == true);               // UNIQUE
    assert(crt->children[0]->type == TYPE_IDENTIFIER);
    freeAST(ast); free(t.tokens);
}

// --- compile: DROP ---

void test_parse_drop_table() {
    COMPILE("drop table users", t, ast);
    ast_node* drp = ast->children[0];
    assert(drp->type == TYPE_DROP_STMT);
    assert(drp->tok.type == TOKEN_TABLE);
    assert(drp->children[0]->type == TYPE_IDENTIFIER);
    freeAST(ast); free(t.tokens);
}

void test_parse_drop_view() {
    COMPILE("drop view myview", t, ast);
    ast_node* drp = ast->children[0];
    assert(drp->type == TYPE_DROP_STMT);
    assert(drp->tok.type == TOKEN_VIEW);
    assert(drp->children[0]->type == TYPE_IDENTIFIER);
    freeAST(ast); free(t.tokens);
}

// --- compile: ALTER ---

void test_parse_alter_add() {
    COMPILE("alter table users add name text", t, ast);
    ast_node* alt = ast->children[0];
    assert(alt->type == TYPE_ALTER_STMT);
    assert(alt->tok.type == TOKEN_ADD);
    assert(alt->children[0]->type == TYPE_IDENTIFIER);  // table name
    assert(alt->children[1]->type == TYPE_COL_DEF);     // new column
    freeAST(ast); free(t.tokens);
}

void test_parse_alter_drop_column() {
    COMPILE("alter table users drop column name", t, ast);
    ast_node* alt = ast->children[0];
    assert(alt->type == TYPE_ALTER_STMT);
    assert(alt->tok.type == TOKEN_DROP);
    assert(alt->children[1]->type == TYPE_IDENTIFIER);  // dropped column name
    assert(alt->children[2] == NULL);
    freeAST(ast); free(t.tokens);
}

void test_parse_alter_alter_column() {
    COMPILE("alter table users alter column name varchar", t, ast);
    ast_node* alt = ast->children[0];
    assert(alt->type == TYPE_ALTER_STMT);
    assert(alt->tok.type == TOKEN_ALTER);
    assert(alt->children[1]->type == TYPE_IDENTIFIER);  // column name
    assert(alt->children[2]->type == TYPE_IDENTIFIER);  // new type
    freeAST(ast); free(t.tokens);
}

// --- compile: expressions ---

void test_parse_expr_arithmetic() {
    COMPILE("select a + b from t", t, ast);
    ast_node* sel_item = ast->children[0]->children[0]->children[0];
    assert(sel_item->type == TYPE_SELECT_ITEM);
    ast_node* add = sel_item->children[0];
    assert(add->type == TYPE_ADDITIVE);
    assert(add->tok.type == TOKEN_PLUS);
    freeAST(ast); free(t.tokens);
}

void test_parse_expr_logical() {
    COMPILE("select * from t where x > 5 and y < 10", t, ast);
    ast_node* whr = ast->children[0]->children[2];
    assert(whr->type == TYPE_WHERE_CLAUSE);
    ast_node* and_node = whr->children[0];
    assert(and_node->type == TYPE_AND_EXPR);
    assert(and_node->children[0]->type == TYPE_COMPARISON);
    assert(and_node->children[1]->type == TYPE_COMPARISON);
    freeAST(ast); free(t.tokens);
}

// --- master ---

void test_parser() {
    test_parse_select_star();
    test_parse_select_columns();
    test_parse_select_where();
    test_parse_select_distinct();
    test_parse_select_limit();
    test_parse_select_order_by();
    test_parse_select_group_by();
    test_parse_select_expr_alias();
    test_parse_insert_with_cols();
    test_parse_insert_without_cols();
    test_parse_update();
    test_parse_update_no_where();
    test_parse_delete_with_where();
    test_parse_delete_no_where();
    test_parse_create_table();
    test_parse_create_index();
    test_parse_create_unique_index();
    test_parse_drop_table();
    test_parse_drop_view();
    test_parse_alter_add();
    test_parse_alter_drop_column();
    test_parse_alter_alter_column();
    test_parse_expr_arithmetic();
    test_parse_expr_logical();
    printf("All parser tests passed.\n");
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// Generator Tests

// ##########################################################################################################################################
// ##########################################################################################################################################
// VM Tests

