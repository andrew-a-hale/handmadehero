#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define Assert(Expression) if (!(Expression)) { *(int *)0 = 0; }
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)

// Services that the platform provides layer to the game
#if HANDMADE_INTERNAL
struct DEBUGReadFileResult {
  uint32_t ContentsSize;
  void *Contents;
};

internal DEBUGReadFileResult DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory);
#endif

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

struct GameButtonState {
  int HalfTransitionCount;
  bool EndedDown;
};

struct GameControllerInput {
  bool IsAnalog;
  float StartX;
  float StartY;

  float MinX;
  float MinY;

  float MaxX;
  float MaxY;

  float EndX;
  float EndY;

  union {
    GameButtonState Buttons[10];
    struct {
      GameButtonState Up;
      GameButtonState Down;
      GameButtonState Left;
      GameButtonState Right;
      GameButtonState LeftShoulder;
      GameButtonState RightShoulder;

      GameButtonState AButton;
      GameButtonState XButton;
      GameButtonState BButton;
      GameButtonState YButton;
    };
  };
};

struct GameInput {
  GameControllerInput Controllers[4];
};

struct GameState {
  int ToneHz;
  int GreenOffset;
  int BlueOffset;
};

struct GameMemory {
  bool IsInitialised;
  uint64_t PermanentStorageSize;
  void *PermanentStorage;

  uint64_t TransientStorageSize;
  void *TransientStorage;
};

internal void GameUpdateAndRender(GameMemory *Memory,
                                  GameInput *Input,
                                  OffscreenBuffer *Buffer,
                                  GameSoundOutputBuffer *SoundBuffer);

#define HANDMADE_H
#endif
