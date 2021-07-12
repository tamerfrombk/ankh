### Grammar

```
program               → declaration* EOF

declaration           → statement | function_declaration
function_declaration  → "fn" identifier "(" params? ")" block
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

variable_declaration  → "let" identifier "=" expression semicolon
assignment            → identifier ( "=" | "+=" | "-=" | "*=" | "/=" ) expression semicolon
inc_dec               → ( "++" | "--" ) expression semicolon
expression_statement  → expression semicolon
print_statement       → "print" expression semicolon
block                 → "{" declaration* "}"
if_statement          → "if" expression block ( "else" ( block | if_statement ) )?
while_statement       → "while" expression block
for_statement         → "for" ( variable_declaration | semicolon ) ( expression | semicolon ) statement? block
return_statement      → "return" expression? semicolon

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
                        | dictionary

literal               → number | string | "true" | "false" | "nil"
lambda                → fn "(" params? ")" block
command               → "$" "(" [alnum]+ ")"
dictionary            → "{" entries? "}"
entries               → entry ( "," entry )*
entry                 → key ":" expression
key                   → identifier | ( "[" expression "]" )
identifier            → (_ | [a-z][A-Z]) (_ | [a-z][A-Z] | [0-9])*
string                → "[ascii]*"
number                → [0-9]\.?[0-9]+
newline               → "\n" | "\r\n"
semicolon             → ";"

```
