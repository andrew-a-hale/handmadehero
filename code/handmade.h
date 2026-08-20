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

struct GameSoundOutputBuffer {
  int SamplesPerSecond;
  int SampleCount;
  int16_t *Samples;
};

internal void GameUpdateAndRender(OffscreenBuffer *Buffer, int XOffset, int YOffset, GameSoundOutputBuffer *SoundBuffer, int ToneHz);

#define HANDMADE_H
#endif
