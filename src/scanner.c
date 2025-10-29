#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <wctype.h>

enum TokenType {
  BLOCK_COMMENT,
};

typedef struct {
  unsigned depth;
} Scanner;

void *tree_sitter_candid_external_scanner_create() {
  return ts_calloc(1, sizeof(Scanner));
}

void tree_sitter_candid_external_scanner_destroy(void *payload) {
  ts_free((Scanner *)payload);
}

unsigned tree_sitter_candid_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *scanner = (Scanner *)payload;
    buffer[0] = (char)scanner->depth;
  return 1;
}

void tree_sitter_candid_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *scanner = (Scanner *)payload;
    scanner->depth = 0;
    if (length == 1) {
        Scanner *scanner = (Scanner *)payload;
        scanner->depth = buffer[0];
    }
}

static inline int advance_and_peek(TSLexer *lexer, bool skip) {
  lexer->advance(lexer, skip);
  return lexer->lookahead;
}

static bool skip_block_comment_start(TSLexer *lexer) {
  if (lexer->lookahead != '/') return false;
  if (advance_and_peek(lexer, false) != '*') return false;
  advance_and_peek(lexer, false);
  return true;
}

static bool consume_block_comment_body(Scanner *scanner, TSLexer *lexer) {
  scanner->depth = 1;

  for (;;) {
    if (lexer->eof(lexer)) {
      lexer->mark_end(lexer);
      return false;
    }

    if (lexer->lookahead == '/') {
      if (advance_and_peek(lexer, false) == '*') {
        advance_and_peek(lexer, false);
        scanner->depth++;
        continue;
      }
      continue;
    }

    if (lexer->lookahead == '*') {
      if (advance_and_peek(lexer, false) == '/') {
        advance_and_peek(lexer, false);
        if (--scanner->depth == 0) {
          lexer->mark_end(lexer);
          return true;
        }
        continue;
      }
      continue;
    }

    bool is_newline = lexer->lookahead == '\n';
    advance_and_peek(lexer, is_newline);
  }
}

bool tree_sitter_candid_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;
  if (!scanner || !valid_symbols[BLOCK_COMMENT]) return false;

  if (!skip_block_comment_start(lexer)) return false;
  if (!consume_block_comment_body(scanner, lexer)) return false;

  lexer->result_symbol = BLOCK_COMMENT;
  return true;
}
