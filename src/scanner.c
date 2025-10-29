#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <stdint.h>
#include <string.h>
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

  memcpy(buffer, &scanner->depth, sizeof(scanner->depth));
  return sizeof(scanner->depth);
}

void tree_sitter_candid_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (!scanner) return;

  scanner->depth = 0;
  // Restore tracked nesting depth when the serialized size matches.
  if (length == sizeof(scanner->depth)) {
    memcpy(&scanner->depth, buffer, sizeof(scanner->depth));
  }
}

// Consume leading whitespace so comment detection always starts on syntax.
static inline void skip_leading_whitespace(TSLexer *lexer) {
  while (!lexer->eof(lexer) && iswspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }
}

// Advance past a specific character if it matches the current lookahead.
static inline bool consume_char(TSLexer *lexer, int32_t expected) {
  if (lexer->lookahead != expected) return false;
  lexer->advance(lexer, false);
  return true;
}

// Detect CR or LF without relying on locale-specific checks.
static inline bool is_line_break(int32_t character) {
  return character == '\n' || character == '\r';
}

// Skip a nested line comment inside a block comment, preserving newlines.
static void skip_nested_line_comment(TSLexer *lexer) {
  if (lexer->lookahead != '/') return;

  lexer->advance(lexer, false);

  while (!lexer->eof(lexer) && !is_line_break(lexer->lookahead)) {
    lexer->advance(lexer, false);
  }

  if (!lexer->eof(lexer)) {
    int32_t newline = lexer->lookahead;
    lexer->advance(lexer, false);
    if (newline == '\r' && lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
    }
  }
}

// Consume the `/ *` prefix once leading whitespace is removed.
static bool skip_block_comment_start(TSLexer *lexer) {
  skip_leading_whitespace(lexer);

  if (!consume_char(lexer, '/')) return false;
  if (!consume_char(lexer, '*')) return false;

  return true;
}

// Walk the body of the block comment while tracking nested depth.
static bool consume_block_comment_body(Scanner *scanner, TSLexer *lexer) {
  scanner->depth = 1;

  for (;;) {
    if (lexer->eof(lexer)) {
      lexer->mark_end(lexer);
      return false;
    }

    int32_t lookahead = lexer->lookahead;

    if (lookahead == '/') {
      lexer->advance(lexer, false);

      if (lexer->lookahead == '*') {
        lexer->advance(lexer, false);
        scanner->depth++;
        continue;
      }

      if (lexer->lookahead == '/') {
        skip_nested_line_comment(lexer);
        continue;
      }

      continue;
    }

    if (lookahead == '*') {
      lexer->advance(lexer, false);

      if (lexer->lookahead == '/') {
        lexer->advance(lexer, false);
        if (--scanner->depth == 0) {
          lexer->mark_end(lexer);
          return true;
        }
        continue;
      }

      continue;
    }

    lexer->advance(lexer, false);
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
