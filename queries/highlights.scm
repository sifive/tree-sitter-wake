; highlights.scm

"binary"	@keyword
"data"		@keyword
"def"		@keyword
"else"		@keyword
"export"	@keyword
"from"		@keyword
"global"	@keyword
"here"		@keyword
"if"		@keyword
"import"	@keyword
"match"		@keyword
"package"	@keyword
"prim"		@keyword
"publish"	@keyword
"subscribe"	@keyword
"target"	@keyword
"then"		@keyword
"topic"		@keyword
"tuple"		@keyword
"type"		@keyword
"unary"		@keyword

; Truly special in wake
"\"		@punctuation.delimiter
"="		@punctuation.delimiter
":"		@punctuation.delimiter
"("		@punctuation.bracket
")"		@punctuation.bracket

(comment)	@comment

; These definitions intoduce a function scope that captures their pattern arguments
(definition pattern: (low_application_pattern fn: (low_identifier_pattern) @function)) @local.scope
(definition pattern: (binary_pattern operator: (*) @function)) @local.scope
(definition pattern: (unary_pattern  operator: (*) @function)) @local.scope

(tuple name: (high_identifier_type) @constructor)
(tuple (fields (field name: (high_identifier_pattern) @attribute)))

(data type: (binary_type operator: (*) @function))
(data type: (unary_type  operator: (*) @function))
(data type: (application_type  fn: (*) @function))
;(data type: (high_identifier_type) @local.definition)

(data (constructors (binary_type operator: (*) @constructor)))
(data (constructors (unary_type  operator: (*) @constructor)))
(data (constructors (application_type fn: (high_identifier_type) @constructor)))
(data (constructors (high_identifier_type) @constructor))

(block_expression)		@local.scope
(lambda_expression)		@local.scope

(low_identifier_pattern)	@local.definition
(high_identifier_pattern)	@local.reference
(low_identifier_expression)	@local.reference
(high_identifier_expression)	@local.reference

(low_identifier_type)		@type
(high_identifier_type)		@type
(application_type)		@type
(binary_type)			@type
(unary_type)			@type

(low_identifier_pattern)	@pattern
(high_identifier_pattern)	@pattern
(application_pattern)		@pattern
(low_application_pattern)	@pattern
(binary_pattern)		@pattern
(unary_pattern)			@pattern
(hole_pattern)			@pattern
; literals?

(integer)			@number
(double)			@number
(regexp)			@string.special
(single_string)			@string
(double_string)			@string

; Operators are less interesting than the other properties called out above
(dot_op)		@operator
(composition_op)	@operator
(unary_fn_op)		@operator
(exponent_op)		@operator
(muldiv_op)		@operator
(addsub_op)		@operator
(comparison_op)		@operator
(inequality_op)		@operator
(and_op)		@operator
(or_op)			@operator
(currency_op)		@operator
(lr_arrow_op)		@operator
(bi_arrow_op)		@operator
(quantifier_op)		@operator
(colon_op)		@operator
(comma_op)		@punctuation.delimiter ; not actually special, but people think it is
