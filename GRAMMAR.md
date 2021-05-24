### Grammar

```
program               → declaration* EOF

declaration           → statement | function_declaration
function_declaration  → "def" identifier "(" params? ")" block
params                → identifier ( "," identifier )*

statement             → expression_statement
                        | variable_declaration 
                        | assignment
                        | print_statement
                        | block
                        | if_statement
                        | while_statement
                        | for_statement
                        | return_statement

variable_declaration  → identifier ":=" assignment
assignment            → identifier ( "=" | "+=" | "-=" | "*=" | "/=" ) expression newline
expression_statement  → expression newline
print_statement       → "print" expression newline
block                 → "{" declaration* "}"
if_statement          → "if" expression block ( "else" block )?
while_statement       → "while" expression block
for_statement         → "for" variable_declaration? ";" expression? ";" assignment? block
return_statement      → "return" expression?

expression            → or_expression
or_expression         → and_expression ( "||" and_expression )*
and_expression        → equality ( "&&" equality )*
equality              → comparison ( ( "!=" | "==" ) comparison )* 
comparison            → term ( ( ">" | ">=" | "<" | "<=" ) term )* 
term                  → factor ( ( "-" | "+" ) factor )* 
factor                → unary ( ( "/" | "*" ) unary )* 
unary                 → ( "!" | "-" ) unary | call
call                  → primary "(" args? ")"
args                  → expression ( "," expression )* 
primary               → number | string 
                        | "true" | "false" | "nil" 
                        | "(" expression ")" 
                        | identifier

identifier            → (_ | [a-z][A-Z]) (_ | [a-z][A-Z] | [0-9])*
string                → "[ascii]*"
number                → [0-9]\.?[0-9]+
newline               → "\n" | "\r\n"

```