#include <string.h>
#include <stdlib.h>
#include <tree_sitter/parser.h>

enum TokenType {
  EOL,
  INDENT,
  DEDENT,
  COMMENT
};

// No more than 32 levels deep or 100 characters total.
struct Indentation {
  unsigned char depth;
  unsigned char stack[32];
  int32_t tabkind[100];
};

void *tree_sitter_wake_external_scanner_create() {
  struct Indentation *i = malloc(sizeof(struct Indentation));
  i->depth = 0;
  return i;
}

void tree_sitter_wake_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_wake_external_scanner_serialize(void *payload, char *buffer) {
  struct Indentation *i = payload;
  unsigned out = i->depth;
  memcpy(buffer, &i->stack[0], out);
  return out;
}

void tree_sitter_wake_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  struct Indentation *i = payload;
  i->depth = length;
  memcpy(&i->stack[0], buffer, length);
}

static bool is_lws(int32_t c) {
  return c == ' ' || c == '\t' || c == 0xa0 || c == 0x1680 ||
         (c >= 0x2000 && c <= 0x200a) ||
         c == 0x202f || c == 0x205f || c == 0x3000;
}
static bool is_nl(int32_t c) {
  return c == '\n' || c == '\v' || c == '\f' || c == '\r' ||
         c == 0x85 || c == 0x2028 || c == 0x2029;
}

bool tree_sitter_wake_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  struct Indentation *i = payload;
  bool comment = false;

  // By default, consume nothing.
  // This is necessary to make it possible to produce INDENT/DEDENT out of nothing.
  lexer->mark_end(lexer);

  // Skip any trailing whitespace on the line
  while (is_lws(lexer->lookahead)) lexer->advance(lexer, false);

empty:
  // Skip over comment contents
  if (valid_symbols[COMMENT] && lexer->lookahead == '#') {
    comment = true;
    do lexer->advance(lexer, false);
    while (lexer->lookahead && !is_nl(lexer->lookahead));
    lexer->mark_end(lexer);
  }

  // If there is still stuff on the current line, there is no EOL/INDENT/DEDENT/COMMENT.
  if (!is_nl(lexer->lookahead)) return false;

  // Skip the leading whitespace
  do lexer->advance(lexer, false);
  while (is_lws(lexer->lookahead));

  // Allow empty lines
  if (is_nl(lexer->lookahead) || lexer->lookahead == '#') goto empty;

  if (comment) {
    lexer->result_symbol = COMMENT;
    return true;
  }

  // How deeply indented are we?
  unsigned newdepth = lexer->get_column(lexer);

  // How deeply indented WERE we?
  unsigned olddepth = i->depth ? i->stack[i->depth-1] : 0;

  if (valid_symbols[INDENT] && newdepth > olddepth && newdepth < 256) {
    if (i->depth < sizeof(i->stack)) {
      i->stack[i->depth] = newdepth;
      ++i->depth;
      lexer->result_symbol = INDENT;
      return true;
    } else {
      // forbid too many levels of nested scope
      return false;
    }
  }

  if (valid_symbols[DEDENT] && newdepth < olddepth) {
    --i->depth;
    lexer->result_symbol = DEDENT;
    return true;
  }

  if (valid_symbols[EOL] && newdepth == olddepth) {
    lexer->mark_end(lexer);
    return true;
  }

  return false;
}
