#!/usr/bin/env fak

fn join(a, b, c, joiner) {
    return joiner(joiner(a, b), c)
}

let joiner = fn (a, b) { return a + b }

print join("this ", "rellly ", "works!", joiner)

# OR

print join("this ", "also ", "works!", fn (a, b) {
    return a + b
})
