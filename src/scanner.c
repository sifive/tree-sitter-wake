#include <string.h>
#include <stdlib.h>
#include <tree_sitter/parser.h>

#define MAX_HERE_PREFIX 8

enum TokenType {
  EOL,
  INDENT,
  DEDENT,
  COMMENT,
  STRING_START,
  STRING_MIDDLE,
  STRING_END,
  STRING_SIMPLE
};

// No more than 32 levels deep or 100 characters total.
struct Indentation {
  unsigned char depth;
  unsigned char stack[32];
  int32_t heredoc[MAX_HERE_PREFIX];
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
  unsigned here = sizeof(i->heredoc);
  unsigned stack = i->depth;
  memcpy(buffer, &i->heredoc[0], here);
  memcpy(buffer+here, &i->stack[0], stack);
  return here+stack;
}

void tree_sitter_wake_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  struct Indentation *i = payload;
  unsigned here = sizeof(i->heredoc);
  if (length == 0) {
    i->depth =0;
  } else {
    i->depth = length - here;
    memcpy(&i->heredoc[0], buffer, here);
    memcpy(&i->stack[0], buffer+here, length-here);
  }
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

static bool is_str_special(int32_t c) {
  return c == '"' || c == '\\' || c == '{';
}

static void skip_till_close(TSLexer *lexer, struct Indentation *i) {
  unsigned x = 0;
  unsigned p;
  int32_t ring[MAX_HERE_PREFIX], flat[MAX_HERE_PREFIX];

  // Measure the required escape string length
  for (p = 0; p < MAX_HERE_PREFIX; ++p)
    if (!i->heredoc[p]) break;

  // Clear ring
  memset(ring, 0, sizeof(ring));

top:
  while (!is_str_special(lexer->lookahead)) {
    ring[x] = lexer->lookahead;
    if (++x == MAX_HERE_PREFIX) x = 0;
    lexer->advance(lexer, false);
  }

  // Re-align the last few charactersbefore the special character
  memcpy(&flat[0], &ring[x], (MAX_HERE_PREFIX-x)*sizeof(int32_t));
  memcpy(&flat[MAX_HERE_PREFIX-x], &ring[0], x*sizeof(int32_t));

  // If the prefix does not match, scan again.
  if (memcmp(&i->heredoc[0], &flat[MAX_HERE_PREFIX-p], p*sizeof(int32_t)) != 0) {
    lexer->advance(lexer, false);
    goto top;
  }

  // If this was an escape, process it and continue
  if (lexer->lookahead == '\\') {
    lexer->advance(lexer, false);
    lexer->advance(lexer, false);
    goto top;
  }
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
  if ((valid_symbols[STRING_START] || valid_symbols[STRING_SIMPLE]) && lexer->lookahead == '"') {
    lexer->advance(lexer, false);
    memset(&i->heredoc[0], 0, sizeof(i->heredoc));
    for (int j = 0; j < MAX_HERE_PREFIX && !is_nl(lexer->lookahead) && !is_str_special(lexer->lookahead); ++j) {
      i->heredoc[j] = lexer->lookahead;
      lexer->advance(lexer, false);
    }
    // If not multiline, clear the heredoc prefix
    if (!is_nl(lexer->lookahead)) {
      memset(&i->heredoc[0], 0, sizeof(i->heredoc));
    }
    skip_till_close(lexer, i);
    if (lexer->lookahead == '"') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      lexer->result_symbol = STRING_SIMPLE;
      return valid_symbols[STRING_SIMPLE];
    }
    if (lexer->lookahead == '{') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      lexer->result_symbol = STRING_START;
      return valid_symbols[STRING_START];
    }
  }

  if ((valid_symbols[STRING_MIDDLE] || valid_symbols[STRING_END]) && lexer->lookahead == '}') {
    skip_till_close(lexer, i);
    if (lexer->lookahead == '"') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      lexer->result_symbol = STRING_END;
      return valid_symbols[STRING_END];
    }
    if (lexer->lookahead == '{') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      lexer->result_symbol = STRING_MIDDLE;
      return valid_symbols[STRING_MIDDLE];
    }
  }

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
