#!/usr/bin/env ankhsh

# This function calculates the nth fibonnaci number recursively
fn fib(n) {
    # base case
    if n <= 1 { return n }

    # general case
    return fib(n - 2) + fib(n - 1)
}

# this will calculate the first 10 fibonacci numbers
for let i = 0; i < 10; ++i {
    print fib(i)
}
