### Grammar
```
statement      → expression newline
expression     → equality 
equality       → comparison ( ( "!=" | "==" ) comparison )* 
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* 
term           → factor ( ( "-" | "+" ) factor )* 
factor         → unary ( ( "/" | "*" ) unary )* 
unary          → ( "!" | "-" ) unary | primary 
primary        → number | string | "true" | "false" | "nil" | "(" expression ")" 

string         → "[ascii]*"
number         → [0-9]\.?[0-9]+
newline        → "\n" | "\r\n"
```