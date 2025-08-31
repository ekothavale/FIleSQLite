#include "parser.h"

typedef struct parser {
    char query[2048];
    int querylen;
    int cursor;
} parser;

parser p;

void initParser() {
    p.cursor = 0;
}

// checks if at end of string
bool atEnd() {
    return p.query[p.cursor] == '\0' || p.cursor == p.querylen;
}

// gets next token in input
token getNextToken(char* input) {
    ;
}

// is letter or underscore
static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

// is number
static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static char nextChar() {
  p.cursor++;
  return p.query[p.cursor-1];
}

static char peek() {
  return p.query[p.cursor];
}

static char peekNext() {
  if (atEnd()) return '\0';
  return p.query[p.cursor+1];
}

static bool match(char expected) {
  if (atEnd()) return false;
  if (p.query[p.cursor] != expected) return false;
  p.cursor++;
  return true;
}

// checks if character is whitespace
bool isWhitespace(char c) {
    char* skipped = "\n\t ";
    if (strstr(skipped, c) == NULL) return true;
    return false;
}

// returns literal token with number type and package
bool processNumber(char c) {
    if (48 <= c && c <= 57) {

        return true;
    }
    return false;
}

static TokenType checkKeyword(int start, int length,
    const char* rest, TokenType type) {
  if (p.query[p.cursor] - p.query[0] == start + length &&
      memcmp(p.query[0] + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static keywordType() {
    switch(p.query[p.cursor]) {
        case 'S': return checkKeyword(1, 5, "ELECT", TOKEN_SELECT);
    }
}

token scanToken() {

}

token* parse(char* input) {
    initParser();
    while (p.cursor < p.querylen) {
        char i = p.query[p.cursor];
        if (isWhitespace(i)) {
            continue;
            p.cursor++;
        }
        if (processNumber(i)) {
            continue;
            p.cursor++;
        }
    }
}