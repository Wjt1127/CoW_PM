# hybrid-memory-allocator
Library for DRAM/NVM hybrid memory allocator

TODO:
     1. Modify the pbuddy_malloc for CoW

Difficulty:
     How to record the address of the POT table(use fd?)
Solution:
     Add a offset in allocator structure, and make the allocator big enough to allocate obj.