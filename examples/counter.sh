#!/usr/bin/env fak

fn makeCounter() {
  let i = 0
  fn count() {
    ++i
    print i
  }

  return count
}

let counter = makeCounter()
counter() #1
counter() #2
