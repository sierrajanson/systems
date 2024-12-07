#Assignment 3 directory

This directory contains source code and other files for Assignment 3.

Use this README document to store notes about design, testing, and
questions you have while developing your assignment.


To explain my implementation, I left in detail comments on my code files, particularly on the rwlock section.
I used mutexes to protect the inner functions in queue.c. If memory was allocated I ensured that the resulting pointer was not NULL.
My queue would block if full, and wait if another function had the lock. For RWLock, I utilized locks and condition variables (to signal/broadcast, for waiting readers)
to control the multiple threads.


I developed my files one at a time and simply, allowing for a much more efficient code process than usual. I first built a single-threaded queue, briefly creating a main() and Makefile to 
test that queue creation, appending, removing, and queue freeing worked correctly. Then I worked on RWLock (as I thought at first I had to utilize the RWLock in my queue section).
This was harder for me to test, and I ended up mainly using the test scripts provided by the class to help debug. Then lastly, after my RWLock, I worked on making the queue multithreaded.


