### Grammar

```
program               → declaration* EOF

declaration           → statement | function_declaration
function_declaration  → "def" identifier "(" params? ")" block
params                → identifier ( "," identifier )*

statement             → expression_statement
                        | variable_declaration
                        | assignment
                        | inc_dec
                        | print_statement
                        | block
                        | if_statement
                        | while_statement
                        | for_statement
                        | return_statement

variable_declaration  → identifier ":=" assignment
assignment            → identifier ( "=" | "+=" | "-=" | "*=" | "/=" ) expression newline
inc_dec               → ( "++" | "--" ) expression newline
expression_statement  → expression newline
print_statement       → "print" expression newline
block                 → "{" declaration* "}"
if_statement          → "if" expression block ( "else" block )?
while_statement       → "while" expression block
for_statement         → "for" variable_declaration? ";" expression? ";" statement? block
return_statement      → "return" expression?

expression            → or_expression
or_expression         → and_expression ( "||" and_expression )*
and_expression        → equality ( "&&" equality )*
equality              → comparison ( ( "!=" | "==" ) comparison )*
comparison            → term ( ( ">" | ">=" | "<" | "<=" ) term )*
term                  → factor ( ( "-" | "+" ) factor )*
factor                → unary ( ( "/" | "*" ) unary )*
unary                 → ( "!" | "-" ) unary | operable
operable              → primary ( "(" args? ")" | "[" expression "]" )
args                  → expression ( "," expression )*
primary               → literal
                        | "(" expression ")"
                        | identifier
                        | lambda
                        | command

literal               → number | string | "true" | "false" | "nil"
lambda                → def "(" params? ")" block
command               → "$" "(" [alnum]+ ")"
identifier            → (_ | [a-z][A-Z]) (_ | [a-z][A-Z] | [0-9])*
string                → "[ascii]*"
number                → [0-9]\.?[0-9]+
newline               → "\n" | "\r\n"

```
