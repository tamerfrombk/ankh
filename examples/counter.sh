#!/usr/bin/env fak

def makeCounter() {
  i := 0
  def count() {
    ++i
    print i

    return
  }

  return count
}

counter := makeCounter()
counter() #1
counter() #2
