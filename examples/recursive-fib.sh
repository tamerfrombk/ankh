#!/usr/bin/env fak

# This function calculates the nth fibonnaci number recursively
def fib(n) {
    # base case
    if n <= 1 { return n }

    # general case
    return fib(n - 2) + fib(n - 1)
}

# this will calculate the first 10 fibonacci numbers
for i := 0; i < 10; ++i {
    print fib(i)
}
