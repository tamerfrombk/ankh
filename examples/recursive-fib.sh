# This function calculates the nth fibonnaci number recursively
def fib(n) {
    # base case
    if n <= 1 { return n }

    # general case
    x2 = fib(n - 2)
    print x2
    
    x1 = fib(n - 1)
    print x1

    return x2 + x1
}

def factorial(n) {
    if n <= 1 { return n }

    return n * factorial(n - 1)
}

# this will calculate the first 10 fibonacci numbers
for i = 0; i < 10; i = i + 1 {
    print fib(i)
}