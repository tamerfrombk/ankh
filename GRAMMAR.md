### Grammar

```
program               → declaration* EOF

declaration           → assignment | statement
assignment            → identifier "=" expression newline

statement             → expression_statement 
                        | print_statement
                        | block
                        | if_statement
                        | while_statement

expression_statement  → expression newline
print_statement       → "print" expression newline
block                 → "{" declaration* "}"
if_statement          → "if" expression block ( "else" block )?
while_statement       → "while" expression block

expression            → or_expression
or_expression         → and_expression ( "||" and_expression )*
and_expression        → equality ( "&&" equality )*
equality              → comparison ( ( "!=" | "==" ) comparison )* 
comparison            → term ( ( ">" | ">=" | "<" | "<=" ) term )* 
term                  → factor ( ( "-" | "+" ) factor )* 
factor                → unary ( ( "/" | "*" ) unary )* 
unary                 → ( "!" | "-" ) unary | primary 
primary               → number | string 
                        | "true" | "false" | "nil" 
                        | "(" expression ")" 
                        | identifier

identifier            → (_ | [a-z][A-Z]) (_ | [a-z][A-Z] | [0-9])*
string                → "[ascii]*"
number                → [0-9]\.?[0-9]+
newline               → "\n" | "\r\n"

```