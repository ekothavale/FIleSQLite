#ifndef TESTING_H
#define TESTING_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bplus.h"
#include "page.h"

void test_btree();

/* Slotted-page CRUD tests */
void test_page_add();
void test_page_read();
void test_page_delete();
void test_page_update();
void test_page_multiple_records();
void test_page();   /* runs all of the above */

#endif