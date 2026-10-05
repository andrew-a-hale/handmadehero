#include "handmade.h"

internal void RenderWeirdGradient(OffscreenBuffer *Buffer, int BlueOffset,
                                  int GreenOffset) {
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for (int y = 0; y < Buffer->Height; ++y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int x = 0; x < Buffer->Width; ++x) {
      uint8_t Blue = x + BlueOffset;
      uint8_t Green = y + GreenOffset;
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal void OutputGameSound(GameSoundOutputBuffer *SoundOutput, int ToneHz) {
  local_persist float t;
  int ToneVolume = 3000;
  int WavePeriod = SoundOutput->SamplesPerSecond / ToneHz;
  int16_t *SampleOut = (int16_t *)SoundOutput->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundOutput->SampleCount;
       ++SampleIndex) {
    float SineValue = sinf(t);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    t += TAU * 1.0f / (float)WavePeriod;
    if (t > TAU) {
      t -= TAU;
    }
  }
}

internal void GameUpdateAndRender(GameMemory *Memory, GameInput *Input,
                                  OffscreenBuffer *Buffer,
                                  GameSoundOutputBuffer *SoundBuffer) {
  Assert(sizeof(GameState) <= Memory->PermanentStorageSize);
  GameState *State = (GameState *)Memory->PermanentStorage;

  if (!Memory->IsInitialised) {
    char *Filename = __FILE__;

    DEBUGReadFileResult File = DEBUGPlatformReadEntireFile(Filename);
    if (File.Contents) {
      DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
      DEBUGPlatformFreeFileMemory(File.Contents);
    }

    State->ToneHz = 256;
    Memory->IsInitialised = true;
  }

  GameControllerInput *Input0 = &Input->Controllers[0];
  if (Input0->IsAnalog) {
    State->ToneHz += (int)(Input0->EndX * 128.0f);
    State->BlueOffset += (int)(Input0->EndY * 4.0f);
  } else {
    if (Input0->AButton.EndedDown) {
      State->GreenOffset += 1;
    }
  }

  OutputGameSound(SoundBuffer, State->ToneHz);
  RenderWeirdGradient(Buffer, State->GreenOffset, State->BlueOffset);
}
