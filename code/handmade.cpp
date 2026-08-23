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

internal void GameUpdateAndRender(GameInput *Input,
                                  OffscreenBuffer *Buffer,
                                  GameSoundOutputBuffer *SoundBuffer) {
  // TODO: allow sample offset for more platform options
  local_persist int GreenOffset = 0;
  local_persist int BlueOffset = 0;
  local_persist int ToneHz = 256;

  GameControllerInput *Input0 = &Input->Controllers[0];
  if (Input0->IsAnalog) {
    ToneHz += (int)(Input0->EndX * 128.0f);
    BlueOffset += (int)(Input0->EndY * 4.0f);
  } else {
    if (Input0->AButton.EndedDown) {
      GreenOffset += 1;
    }
  }

  OutputGameSound(SoundBuffer, ToneHz);
  RenderWeirdGradient(Buffer, GreenOffset, BlueOffset);
}
