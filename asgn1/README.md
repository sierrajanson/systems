#Assignment 1 directory

This directory contains source code and other files for Assignment 1.

Use this README document to store notes about design, testing, and
questions you have while developing your assignment.


This assignment was more difficult than I initially expected, and I completely changed my implementation of memory.c while writing.


Some key notes:
I read the input in pieces, with a read for the command (get or set), the filename, and the message/message length for set. 
However, I initially did this with scanf, as it technically wasn't buffered. As I found myself having to make many exceptions with scanf due to errors, I eventually switched to read with stdin. My implementation for set and get is to have an outer while loop reading in stdin (or the file), and a while loop to write out the bytes written one it had reached the buffer size. 

Initially I read in characters at a time. However I found that my programs would take FOREVER to run for the large binary files. After a few fruitless hours of experimentation and debugging, after checking that it was okay with this class's academic policy, I asked an online GPT for ways I could make writing with write() and a buffer faster, and it recommended reading in chunks instead of reading a character at a time. I implemented this change for get and set and it drastically reduced the runtime of my program. I am curious to know why there is such a time discrephancy: I believe it is mainly due to a reduced number of system calls and reduced amount of overhead from if statements. 

Author: Sierra Janson
Date: 10/20/2024

