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

(block_expression)		@local.scope
(lambda_expression)		@local.scope

; These forms intoduce a scope that captures their pattern arguments
(definition pattern: (low_application_pattern fn: (low_identifier_pattern) @function)) @local.scope
(definition pattern: (binary_pattern operator: (*) @function)) @local.scope
(definition pattern: (unary_pattern  operator: (*) @function)) @local.scope

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

(tuple (fields (field name: (high_identifier_pattern) @local.definition)))

(data (constructors (binary_type operator: (*) @local.definition @function)))
(data (constructors (unary_type  operator: (*) @local.definition @function)))
(data (constructors (application_type fn: (high_identifier_type) @local.definition @function)))
(data (constructors (low_identifier_type) @local.definition))
(data (constructors (high_identifier_type) @local.definition))
