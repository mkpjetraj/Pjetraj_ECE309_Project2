# Design Log — Project 2

## Process

I started with creating a repository skeleton from the provided repository, with the three core headers matching the spec having “stub” function bodies so the code would build before everything was filled in. I filled in message.h (using inline because there is no message.cpp) and then conversation, including the main special member functions and functionality for append operations. I built sentinel scanner to withhold the last parts of a message and to check for the sentinel. Finally, when all was built and working, I made the 12 tests, one function each which are called in main() of tests_p2.cpp. I had to learn how to use “assert” to build the tests but it wasn’t so bad.

## Growth factor and amortized cost

Conversation makes a plain array (Message* data_, size_, capacity_). When append is called and size_ == capacity_ it allocates a new array of twice the size of the current array, moves the old messages, and frees the old array. Capacity grows as orders of 2: 1, 2, 4, 8…

The expensive part of this operation is copying each message over when the array is full. I picked doubling because it's the simplest factor with O(1) cost. Growing by +1 each time the array is full would copy everything on every append which would be very expensive. For n appends, you would have to have copied everything on every append up to (n-1), which would be (n^2 - n) / 2 copies, yielding O(n^2) complexity. With doubling, each time the array gets full you have to copy what is in it, but you also buy an equal amount of storage to expand into. Each copy would still be O(n) complex, but that is paid for over the course of each append between the last copy and the next. To reach n messages, copying happens at sizes 1, 2, 4, ..., 2^x where 2^x < n, and the copies move 1 + 2 + 4 + ... + 2^k = 2^(k+1) − 1 < 2n elements in total. Including the n amount of writes for the new messages themselves, n appends cost less than 3n , so each append is O(1) on average.

## Rule of Five evidence

- Constructor: empty conversation owns no memory: Conversation() {}
- Destructor: delete[] data_, matches new[]. Safe on empty object because delete[] nullptr does nothing
- Copy constructor: allocates array and copies each message, never shares memory with the original. Copying just pointer would cause a double free.
- Copy assignment: copy and swap (safer - handles copy and doesn't lose data if the copy fails)
- Move constructor / assignment: steals the pointer and leaves the source as nullptr with cleared data 

- Tests 3 and 4 check copy and move for safety and functionaloty, ASAN would catch double free or leak

## Sentinel scanner: bounded pending_ proof

Let S be the sentinel length (20 for <|end_conversation|>). In each feed, the scanner searches pending_ + chunk. If sentinel is found, it returns everything before and clears pending_. Otherwise, it returns all but the last keep = min(text.size(), S − 1) characters and stores those as the new pending_.

- pending_.size() is always ≤ S − 1 after each call. It starts empty, and it's only assigned an empty string or a string of length ≤ S − 1.
- S − 1 is enough because a sentinel crossing a chunk boundary has at least one character in the new chunk, so at most S − 1 are at the end of the old text.
- each feed works on at MOST 19 + (chunk size) chars no matter how long the whole reply is. 
- If the sentinel is found, pending_ is cleared, and the harness stops feeding the scanner. Each reply gets its own scanner, so pending_ never carries over.
- Test 9 feeds 1,000,000 characters of <|end_ one at a time and checks to ensure that (chars fed − chars returned) never goes over 19.

## What I would change 
Scanner always holds back 19 characters (S -1 ) even when they aren't the start of the sentinel. In test 6, `feed("Hello, ")` returns nothing because all the characters get held back. It would probably be more complex, but a better version would only hold back the characters if they match the beginning of the sentinel. 