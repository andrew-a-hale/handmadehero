#if !defined(HANDMADE_H)

// Services that the platform provides layer to the game

// Services that the game provides to the platform layer

// Handles Input, Bitmap Buffer and Sound Buffer, Timing
struct OffscreenBuffer {
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

void GameUpdateAndRender(OffscreenBuffer *Buffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif
