### Grammar
```
newline := \n | \r\n
string := " any valid characters* "
statement := expression newline
expression := string | export_expression
identifier := [a-zA-Z][a-zA-Z0-9]*
export_expression := export assignment_expression
assignment_expression := identifier "=" expression
```