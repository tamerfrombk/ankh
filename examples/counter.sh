#!/usr/bin/env fak

fn makeCounter() {
  i := 0
  fn count() {
    ++i
    print i
  }

  return count
}

counter := makeCounter()
counter() #1
counter() #2
