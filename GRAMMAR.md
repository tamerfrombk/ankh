### Grammar

```
program               → declaration* EOF

declaration           → statement

statement             → expression_statement
                        | variable_declaration 
                        | assignment
                        | print_statement
                        | block
                        | if_statement
                        | while_statement
                        | for_statement

variable_declaration  → "let" assignment
assignment            → identifier "=" expression newline
expression_statement  → expression newline
print_statement       → "print" expression newline
block                 → "{" declaration* "}"
if_statement          → "if" expression block ( "else" block )?
while_statement       → "while" expression block
for_statement         → "for" assignment? ";" expression? ";" assignment? block

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