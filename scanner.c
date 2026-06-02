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

// NOTE: function scanNumber assumes max length of number literal is 255 characters
// NOTE: function parse assumes max length of query is 2048 tokens

#include "scanner.h"

typedef struct scanner {
    char query[2048];
    int querylen;
    int cursor;
} scanner;

scanner s;

void initParser(int querylen, char* query) {
    if (querylen >= 2048) {
        printf("Error: Exceeded maximum query length of 2047 characters\n");
        return;
    }
    s.cursor = 0;
    s.querylen = querylen;
    strncpy(s.query, query, querylen);
    s.query[querylen] = '\0';
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
    switch(c) {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            return true;
        default:
            return false;
    }
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
  if (memcmp(s.query + s.cursor + start, rest, length) == 0) {
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
            out.payload = NULL;
            s.cursor += 6;
            break;
        }
        case 'I': {
            out.type = checkKeyword(1, 5, "NSERT", TOKEN_INSERT);
            out.payload = NULL;
            s.cursor += 6;
            break;
        }
        default: {
            out.type = TOKEN_IDENTIFIER;
            out.payload = scanIdentifier();
        }
    }
    return out;
}

static Token scanSymbol() {
    Token out;
    switch(s.query[s.cursor]) {
        case '+':
            out.type = TOKEN_PLUS;
            out.payload = NULL;
            s.cursor++;
            break;
        case '-':
            out.type = TOKEN_MINUS;
            out.payload = NULL;
            s.cursor++;
            break;
        case '*':
            out.type = TOKEN_STAR;
            out.payload = NULL;
            s.cursor++;
            break;
        case '!':
            out.type = TOKEN_BANG;
            out.payload = NULL;
            s.cursor++;
            break;
        case '(':
            out.type = TOKEN_LEFT_PAREN;
            out.payload = NULL;
            s.cursor++;
            break;
        case ')':
            out.type = TOKEN_RIGHT_PAREN;
            out.payload = NULL;
            s.cursor++;
            break;
        case '/':
            out.type = TOKEN_SLASH;
            out.payload = NULL;
            s.cursor++;
            break;
        case '=':
            out.type = TOKEN_EQUALS;
            out.payload = NULL;
            s.cursor++;
            break;
        case '?':
            out.type = TOKEN_QUESTION;
            out.payload = NULL;
            s.cursor++;
            break;
        case ',':
            out.type = TOKEN_COMMA;
            out.payload = NULL;
            s.cursor++;
            break;
        case '.':
            out.type = TOKEN_DOT;
            out.payload = NULL;
            s.cursor++;
            break;
        case '&':
            out.type = TOKEN_AND;
            out.payload = NULL;
            s.cursor++;
            break;
        case '|':
            out.type = TOKEN_PIPE;
            out.payload = NULL;
            s.cursor++;
            break;
        default:
            printf("Unrecognized symbol: %c\n", s.query[s.cursor]);
            out.type = TOKEN_UNKNOWN;
            out.payload = malloc(2);
            out.payload[0] = s.query[s.cursor];
            out.payload[1] = '\0';
            s.cursor++;
    }
    return out;
}

TokenizedQuery* scan(char* input) {
    TokenizedQuery* tokens = malloc(sizeof(TokenizedQuery));
    tokens->tokens = allocTokens(10);
    tokens->capacity = 10;
    tokens->count = 0;
    int tokenCount = 0;
    initParser(strlen(input), input);
    while (s.cursor < s.querylen) {
        char i = s.query[s.cursor];
        if (isWhitespace(i)) {
            s.cursor++;
            continue;
        }
        else if (isDigit(i)) {
            tokens->tokens[tokenCount++] = scanNumber();
        }
        else if (isAlpha(i)) {
            tokens->tokens[tokenCount++] = scanAlpha();
        }
        else {
            tokens->tokens[tokenCount++] = scanSymbol();
        }
    }
    tokens->count = tokenCount;
    return tokens;
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