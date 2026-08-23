#if !defined(HANDMADE_H)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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

internal void GameUpdateAndRender(GameInput *Input,
                                  OffscreenBuffer *Buffer,
                                  GameSoundOutputBuffer *SoundBuffer);

#define HANDMADE_H
#endif
