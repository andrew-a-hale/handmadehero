# Handmade Hero

Upto: Day 16

https://guide.handmadehero.org/
https://davidgow.net/handmadepenguin/default.html

## Concepts

- Graphics
  - Backbuffer
  - Pitch (Bytes in a Row + Padding)
  - Stride (Bit Depth * Row Size + Padding, almost the same as pitch)

- Sound
  - Skipped RingBuffer as it's not required in SDL

- IO
  - Don't Overwrite
  - Don't use variables that haven't been initialised! 
    - printf changed the stack memory and made it look like the uninitialised variable was okay, but it was reading garbage.

- Platform
  - Input
  - Message Handling
  - Unity Build
  - Cross Platform Support
    - Abstract the game: The game has much less flexiblity then the OS, the other side of the API Boundary, so option 2 is preferred

- C
  - Pointer Arithmetic
  - Memory Management (mmap, munmap, malloc, free)
  - rdtsc / query performance counters
  - Include Guard / Idempotent Include
  - C Union

