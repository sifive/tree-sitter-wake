const rules = {
  source_file: $ => repeat($.description),
  simple_identifier: $ => /[a-zA-Z_]\w*/,
  description: $ => choice(),
  comment: $ => token(choice(
    seq('//', /.*/),
    seq(
      '/*',
      /[^*]*\*+([^/*][^*]*\*+)*/,
      '/'
    )
  ))
};

module.exports = grammar({
  name: 'wake',
  word: $ => $.simple_identifier,
  rules: rules,
  extras: $ => [/\s/, $.comment]
});
