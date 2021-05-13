# This function calculates the nth fibonnaci number recursively
def fib(n) {
    # base case
    if n <= 1 { return n }

    # general case
    x2 = fib(n - 2)
    x1 = fib(n - 1)

    return x2 + x1
}

# this will calculate the first 10 fibonacci numbers
for i = 2; i < 3; i = i + 1 {
    print fib(2)
}