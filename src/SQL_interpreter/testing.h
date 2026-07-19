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

#ifndef TESTING_INTERPRETER_H
#define TESTING_INTERPRETER_H

#include <assert.h>
#include <stdio.h>
#include "lexer.h"
#include "parser.h"

/* Lexer tests */
void test_lexer();
void test_scan_single_char_tokens();
void test_scan_two_char_tokens();
void test_scan_single_ops();
void test_scan_keywords();
void test_scan_identifier();
void test_scan_number_integer();
void test_scan_number_float();
void test_scan_number_no_trailing_decimal();
void test_scan_string();
void test_scan_empty_string();
void test_scan_unterminated_string();
void test_scan_skips_whitespace();
void test_scan_skips_comment();
void test_scan_line_tracking();
void test_init_tokenized();
void test_add_token_count();
void test_add_token_stored();
void test_add_multiple_tokens();
void test_lex_query_simple();
void test_lex_query_empty();
void test_lex_query_string_and_number();

/* Parser tests */
void test_parser();
void test_parse_select_star();
void test_parse_select_columns();
void test_parse_select_where();
void test_parse_select_distinct();
void test_parse_select_limit();
void test_parse_select_order_by();
void test_parse_select_group_by();
void test_parse_select_expr_alias();
void test_parse_insert_with_cols();
void test_parse_insert_without_cols();
void test_parse_update();
void test_parse_update_no_where();
void test_parse_delete_with_where();
void test_parse_delete_no_where();
void test_parse_create_table();
void test_parse_create_index();
void test_parse_create_unique_index();
void test_parse_drop_table();
void test_parse_drop_view();
void test_parse_alter_add();
void test_parse_alter_drop_column();
void test_parse_alter_alter_column();
void test_parse_expr_arithmetic();
void test_parse_expr_logical();

#endif