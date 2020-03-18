function binary(op, expr) {
  return seq(field('lhs', expr), field('operator', op), field('rhs', expr))
}

function left (n, op, expr) { return prec.left (n, binary(op, expr)) }
function right(n, op, expr) { return prec.right(n, binary(op, expr)) }

function prefix (n, op, expr) { return prec(n, seq(field('operator', op), field('operand', expr))) }
function postfix(n, op, expr) { return prec(n, seq(field('operand', expr), field('operator', op))) }

function unary_ops($, expr) {
  // !!! add all operators
  return choice(
    prefix (6, $.mul_op,   expr),
    prefix (4, $.plus_op,  expr),
    postfix(2, $.comma_op, expr))
}

function binary_ops($, expr) {
  return choice(
    left (5, $.mul_op,   expr),
    left (3, $.plus_op,  expr),
    right(1, $.comma_op, expr))
}

module.exports = grammar({
  name: 'wake',

  word: $ => $.low_identifier,
  extras: $ => [ /[ \t\xa0\u1680\u2000-\u200a\u202f\u205f\u2029]/, $._eol ],

  externals: $ => [
    $._eol,
    $._indent,
    $._dedent
  ],

  rules: {
    source_file: $ => repeat(choice(
      $.tuple,
      $.data,
      $.topic,
      $.publish,
      $.top_definition,
      $.top_target,
      $.package,
      $.import,
      $.export)),

    // Types --------------------------------------------------------------------------

    // !!! COLON
    _full_type: $ => choice(
      $._term_type,
      $.application_type,
      $.binary_type,
      $.unary_type),

    _term_type: $ => choice(
      seq('(', $._full_type, ')'),
      $._identifier),

    application_type: $ => seq($.high_identifier, repeat1($._term_type)),
    unary_type:  $ => unary_ops ($, $._full_type),
    binary_type: $ => binary_ops($, $._full_type),

    // Patterns -----------------------------------------------------------------------

    _full_pattern: $ => choice(
      $._term_pattern,
      $.application_pattern,
      $.binary_pattern,
      $.unary_pattern,
      $.hole),

    _def_pattern: $ => choice(
      $._full_pattern,
      $.low_application_pattern),

    _term_pattern: $ => choice(
      seq('(', $._full_pattern, ')'),
      $._identifier,
      $._literal),

    term_patterns: $ => repeat1($._term_pattern),

    application_pattern: $ => seq($.high_identifier, repeat1($._term_pattern)),
    low_application_pattern: $ => seq($.low_identifier, repeat1($._term_pattern)),
    unary_pattern:  $ => unary_ops ($, $._full_pattern),
    binary_pattern: $ => binary_ops($, $._full_pattern),

    // Expressions --------------------------------------------------------------------

    _full_expression: $ => choice(
      $._operator_expression,
      $.lambda_expression,
      $.if_expression),

    // !!! COLON
    _operator_expression: $ => choice(
      $._term_expression,
      $.application_expression,
      $.binary_expression,
      $.unary_expression,
      $.prim_expression,
      $.subscribe_expression,
      $.match_expression),

    _term_expression: $ => choice(
      seq('(', $.block_expression, ')'),
      $._identifier,
      $._literal,
      alias($.dot_expression, $.binary_expression)),

    lambda_expression: $ => seq(
      '\\',
      field('pattern', $._term_pattern),
      field('expression', $._full_expression)),

    if_expression: $ => seq(
      'if',   field('condition', $.block_expression),
      'then', field('then',      $.block_expression),
      'else', field('else',      $.block_expression)),

    application_expression: $ => seq($._term_expression, repeat1($._term_expression)),
    unary_expression:  $ => unary_ops ($, $._operator_expression),
    binary_expression: $ => binary_ops($, $._operator_expression),
    dot_expression: $ => left(3, $.dot_op, $._term_expression),

    prim_expression: $ => seq('prim', field('name', $.string)),
    subscribe_expression: $ => seq('subscribe', field('id', $.low_identifier)),

    _block_body: $ => seq(
      field('preamble', repeat(choice($.definition, $.import, $.target))),
      field('expression', $._full_expression)),

    block_expression: $ => choice(
      field('expression', $._full_expression),
      seq($._indent, $._eol, $._block_body, $._dedent)),

    // Match expression ---------------------------------------------------------------

    // !!! multimatch
    match_expression: $ => seq(
      'match',
      field('arg', $._term_expression),
      $._indent,
      field('rules', $.match_rules),
      $._dedent),

    match_rules: $ => repeat1($.match_rule),

    match_rule: $ => seq(
      $._eol,
      field('pattern', $._full_pattern),
      field('guard', optional(alias($.match_guard, $.block_expression))),
      '=',
      field('block', $.block_expression)),

    match_guard: $ => choice(
      seq('if', $._indent, $._eol, $._block_body, $._dedent, $._eol),
      seq('if', field('expression', $._full_expression))),

    // Statements ---------------------------------------------------------------------

    global_flag: $ => 'global',
    export_flag: $ => 'export',

    top_definition: $ => seq(
      field('global', optional($.global_flag)),
      field('export', optional($.export_flag)),
      field('definition', $.definition)),

    top_target: $ => seq(
      field('global', optional($.global_flag)),
      field('export', optional($.export_flag)),
      field('target', $.target)),

    definition: $ => seq(
      'def', field('pattern', $._def_pattern),
      '=',   field('block', $.block_expression),
      $._eol),

    target: $ => seq(
      'target', field('pattern', $._def_pattern),
      field('extra_terms', optional(seq('\\', $.term_patterns))),
      '=', field('block', $.block_expression),
      $._eol),

    topic: $ => seq(
      'topic',  $.low_identifier,
      ':',      $._full_type,
      $._eol),

    publish: $ => seq(
      'publish', $.low_identifier,
      '=',       $.block_expression,
      $._eol),

    type_params: $ => repeat1($.low_identifier),

    tuple: $ => seq(
      field('global', optional($.global_flag)),
      field('export', optional($.export_flag)),
      'tuple',
      field('name', $.high_identifier),
      field('params', optional($.type_params)),
      '=',
      $._indent,
      field('fields', $.fields),
      $._dedent, $._eol),

    fields: $ => repeat1($.field),
    field: $ => seq(
      $._eol,
      field('global', optional($.global_flag)),
      field('export', optional($.export_flag)),
      field('name', $.high_identifier),
      ':',
      field('type', $._full_type)),

    // data type definitions do not allow recursion:
    _data_type: $ => choice(
      $.high_identifier,
      alias($.data_application_type, $.application_type),
      alias($.data_binary_type, $.binary_type),
      alias($.data_unary_type, $.unary_type)),
    data_application_type: $ => seq($.high_identifier, repeat1($.low_identifier)),
    data_binary_type: $ => binary_ops($, $.low_identifier),
    data_unary_type: $ => unary_ops($, $.low_identifier),

    constructors: $ => repeat1(seq($._eol, $._full_type)),
    constructor1: $ => $._full_type,
    data: $ => seq(
      field('global', optional($.global_flag)),
      field('export', optional($.export_flag)),
      'data', field('type', $._data_type),
      '=', field('constructors', choice(
        seq($._indent, $.constructors, $._dedent),
        alias($.constructor1, $.constructors))),
      $._eol),

    // Packages -----------------------------------------------------------------------

    package: $ => seq('package', field('name', $.low_identifier), $._eol),

    def_kind:   $ => 'def',
    type_kind:  $ => 'type',
    topic_kind: $ => 'topic',
    _kind: $ => choice($.def_kind, $.type_kind, $.topic_kind),

    unary_arity:  $ => 'unary',
    binary_arity: $ => 'binary',
    _arity: $ => choice($.unary_arity, $.binary_arity),

    ideq: $ => seq(
      field('to', $.low_identifier),
      optional(seq('=', field('from', $.low_identifier)))),
    ideqs: $ => repeat1($.ideq),

    import: $ => seq(
      'from', 
      field('package', $.low_identifier),
      'import',
      field('kind', optional($._kind)),
      field('arity', optional($._arity)),
      field('ideqs', choice($.hole, $.ideqs)),
      $._eol), 

    export: $ => seq(
      'from', 
      field('package', $.low_identifier),
      'export',
      field('kind', $._kind),
      field('arity', optional($._arity)),
      field('ideqs', $.ideqs),
      $._eol),

    // Lexer --------------------------------------------------------------------------

    _literal: $ => choice($.integer, $.double, $.regexp, $.string, $.here),

    here: $ => 'here',

    integer: $ => {
      const decimal = '([1-9][0-9_]*)';
      const octal   = '(0[0-7_]*)';
      const hex     = '(0x[0-9a-fA-F_]+)';
      const binary  = '(0b[01_]+)';
      return new RegExp([decimal, octal, hex, binary].join('|'));
    },

    double:  $ => {
      const double10  = '([0-9][0-9_]*\.[0-9_]+([eE][+-]?[0-9_]+)?)';
      const double10e = '([0-9][0-9_]*[eE][+-]?[0-9_]+)';
      const double16  = '(0x[0-9a-fA-F_]+\.[0-9a-fA-F_]+([pP][+-]?[0-9a-fA-F_]+)?)';
      const double16e = '(0x[0-9a-fA-F_]+[pP][+-]?[0-9a-fA-F_]+)';
      return new RegExp([double10, double10e, double16, double16e].join('|'));
    },
    regexp:  $ => /`[a-z]`/,
    string:  $ => /"[a-z]"/,
    // !!!:
    dot_op:   $ => '.',
    comma_op: $ => ',',
    plus_op:  $ => /[-+]/,
    mul_op:   $ => '*',

    hole: $ => '_',
    low_identifier:  $ => /[a-z][a-zA-Z0-9_]*/,
    high_identifier: $ => /[A-Z][a-zA-Z0-9_]*/,
    _identifier: $ => choice($.low_identifier, $.high_identifier),
  }
});
