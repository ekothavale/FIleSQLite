// NOTE: function scanNumber assumes max length of number literal is 255 characters
// NOTE: function parse assumes max length of query is 2048 tokens

#include "parser.h"

typedef struct scanner {
    char query[2048];
    int querylen;
    int cursor;
} scanner;

scanner s;

void initParser() {
    s.cursor = 0;
}

// checks if at end of string
bool atEnd() {
    return s.query[s.cursor] == '\0' || s.cursor == s.querylen;
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
  s.cursor++;
  return s.query[s.cursor-1];
}

static char peek() {
  return s.query[s.cursor];
}

static char peekNext() {
  if (atEnd()) return '\0';
  return s.query[s.cursor+1];
}

static bool match(char expected) {
  if (atEnd()) return false;
  if (s.query[s.cursor] != expected) return false;
  s.cursor++;
  return true;
}

// checks if character is whitespace
bool isWhitespace(char c) {
    char* skipped = "\n\t ";
    if (strstr(skipped, c) == NULL) return true;
    return false;
}

// returns literal token with number type and payload
Token scanNumber() {
    char buffer[256];
    int i = 0;
    while((isDigit(peek()) || peek() == '.') && i < 255) {
        buffer[i++] = nextChar();
    }
    buffer[i] = '\0';
    Token token = {.type = TOKEN_NUMBER, .payload = malloc(i+1)};
    strncpy(token.payload, buffer, i+1);
    return token;
}

static TokenType checkKeyword(int start, int length,
    const char* rest, TokenType type) {
  if (s.query[s.cursor] - s.query[0] == start + length &&
      memcmp(s.query[0] + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

char* scanKeyword(int length) {
    char buffer[length + 2];
    for (int i = 0; i < length + 1; i++) {
        buffer[i] = nextChar();
    }
    buffer[length+1] = '\0';
    return strdup(buffer);
}

char* scanIdentifier() {
    char buffer[256];
    int i = 0;
    while(isAlpha(peek()) || isDigit(peek())) {
        buffer[i++] = nextChar();
    }
    buffer[i] = '\0';
    return strdup(buffer);
}

static Token scanAlpha() {
    Token out;
    switch(s.query[s.cursor]) {
        case 'S': {
            out.type = checkKeyword(1, 5, "ELECT", TOKEN_SELECT);
            out.payload = scanKeyword(5);
            break;
        }
        default: {
            out.type = TOKEN_IDENTIFIER;
            out.payload = scanIdentifier();
        }
    }
    return out;
}

Token* parse(char* input) {
    Token* tokens = malloc(2048 * sizeof(Token));
    int tokenCount = 0;
    initParser();
    while (s.cursor < s.querylen) {
        char i = s.query[s.cursor];
        if (isWhitespace(i)) {
            s.cursor++;
            continue;
        }
        if (isDigit(i)) {
            tokens[tokenCount++] = scanNumber();
        }
        if (isAlpha(i)) {
            tokens[tokenCount++] = scanAlpha();
        }
    }
}

// parse:
/*
loop through the string input until reaching end
skip whitespace
if alpha:
    create a token if keyword
    if not recognized - literal
if numeric:
    numeric literal
*/