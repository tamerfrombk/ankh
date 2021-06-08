#!/usr/bin/env fak

def join(a, b, c, joiner) {
    return joiner(joiner(a, b), c)
}

print join("this ", "really ", "works!", def (a, b) {
    return a + b
})
