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
  if (!scanner) return 0;

  buffer[0] = (char)scanner->depth;
  return 1;
}

void tree_sitter_candid_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (!scanner) return;

  scanner->depth = 0;
  if (length == 1) {
    scanner->depth = buffer[0];
  }
}

static inline int advance_and_peek(TSLexer *lexer, bool skip) {
  lexer->advance(lexer, skip);
  return lexer->lookahead;
}

static inline void skip_leading_whitespace(TSLexer *lexer) {
  while (iswspace(lexer->lookahead)) {
    advance_and_peek(lexer, true);
  }
}

static inline bool consume_sequence(TSLexer *lexer, int first, int second) {
  if (lexer->lookahead != first) return false;
  if (advance_and_peek(lexer, false) != second) return false;
  advance_and_peek(lexer, false);
  return true;
}

static bool skip_block_comment_start(TSLexer *lexer) {
  skip_leading_whitespace(lexer);
  return consume_sequence(lexer, '/', '*');
}

static bool consume_block_comment_body(Scanner *scanner, TSLexer *lexer) {
  scanner->depth = 1;

  for (;;) {
    if (lexer->eof(lexer)) {
      lexer->mark_end(lexer);
      return false;
    }

    if (consume_sequence(lexer, '/', '*')) {
      scanner->depth++;
      continue;
    }

    if (consume_sequence(lexer, '*', '/')) {
      if (--scanner->depth == 0) {
        lexer->mark_end(lexer);
        return true;
      }
      continue;
    }

    advance_and_peek(lexer, false);
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
