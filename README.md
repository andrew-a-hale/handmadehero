# Handmade Hero

Upto: Day 15

https://guide.handmadehero.org/
https://davidgow.net/handmadepenguin/default.html

## Concepts

- Graphics
  - Backbuffer
  - Pitch (Bytes in a Row + Padding)
  - Stride (Bit Depth * Row Size + Padding, almost the same as pitch)

- Sound
  - Skipped RingBuffer as it's not required in SDL

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

