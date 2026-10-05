/*  TODO:
  - Save game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() (for multiple monitor)
  - Fullscreen Support
  - Cursor Visibility
  - QueryCancelAutoplay
  - ActivateApp
  - Blit Speed Improvements
  - Hardware Acceleration
  - GetKeyboardLayout (intl. wasd)
*/

#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define PI 3.14159265359f
#define TAU 2.0f * PI

#define internal static
#define local_persist static
#define global_variable static

#include "handmade.cpp"
#include "handmade.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_haptic.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include "sdl_handmade.h"

#define MAX_CONTROLLERS 4

global_variable int FPS = 60;
global_variable bool GlobalRunning;
global_variable SDLOffscreenBuffer GlobalBackBuffer;
global_variable SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
global_variable SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

inline uint32_t SafeTruncateUInt64(uint64_t Value) {
  Assert(Value <= 0xFFFFFFFF);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

internal DEBUGReadFileResult DEBUGPlatformReadEntireFile(char *Filename) {
  DEBUGReadFileResult Result = {};
  int FileHandle = open(Filename, O_RDONLY);
  if (FileHandle == -1) {
    return Result;
  }

  struct stat FileStatus;
  if (fstat(FileHandle, &FileStatus) == -1) {
    close(FileHandle);
    return Result;
  }

  Result.ContentsSize = SafeTruncateUInt64(FileStatus.st_size);
  Result.Contents = malloc(Result.ContentsSize);
  if (!Result.Contents) {
    Result.ContentsSize = 0;
    close(FileHandle);
    return Result;
  }

  uint32_t BytesToRead = Result.ContentsSize;
  uint8_t *NextByteLocation = (uint8_t*)Result.Contents;
  while (BytesToRead) {
    uint32_t BytesRead = read(FileHandle, NextByteLocation, BytesToRead);
    if (BytesRead == -1) {
      DEBUGPlatformFreeFileMemory(Result.Contents);
      Result.Contents = 0;
      Result.ContentsSize = 0;
      close(FileHandle);
      return Result;
    }
    BytesToRead -= BytesRead;
    NextByteLocation += BytesRead;
  }

  close(FileHandle);
  return Result;
}

internal void DEBUGPlatformFreeFileMemory(void *Memory) {
  free(Memory);
}

internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory) {
  int FileHandle = open(Filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (FileHandle == -1) return false;
  uint32_t BytesToWrite = MemorySize;
  uint8_t *NextByteLocation = (uint8_t*)Memory;
  while (BytesToWrite) {
    uint32_t BytesWritten = write(FileHandle, NextByteLocation, BytesToWrite);
    if (BytesWritten == -1) {
      close(FileHandle);
      return false;
    }
    BytesToWrite -= BytesWritten;
    NextByteLocation += BytesWritten;
  }

  close(FileHandle);
  return true;
}

struct SDLWindowDimension {
  int Width;
  int Height;
};

SDLWindowDimension SDLGetWindowDimension(SDL_Window *Window) {
  SDLWindowDimension Result;
  SDL_GetWindowSize(Window, &Result.Width, &Result.Height);
  return Result;
}

internal void SDLResizeTexture(SDLOffscreenBuffer *Buffer,
                               SDL_Renderer *Renderer, int Width, int Height) {
  if (Buffer->Pixels) {
    munmap(Buffer->Pixels, Buffer->TextureWidth * Buffer->TextureHeight *
                               Buffer->BytesPerPixel);
  }

  if (Buffer->Texture) {
    SDL_DestroyTexture(Buffer->Texture);
  }

  Buffer->TextureWidth = Width;
  Buffer->TextureHeight = Height;
  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Width * Buffer->BytesPerPixel;

  Buffer->Texture = SDL_CreateTexture(
      Renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      Buffer->TextureWidth, Buffer->TextureHeight);
  Buffer->Pixels = mmap(
      0, Buffer->TextureWidth * Buffer->TextureHeight * Buffer->BytesPerPixel,
      PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

internal void SDLUpdateWindow(SDLOffscreenBuffer *Buffer, SDL_Window *Window,
                              SDL_Renderer *Renderer) {
  SDL_UpdateTexture(Buffer->Texture, 0, Buffer->Pixels,
                    Buffer->TextureWidth * Buffer->BytesPerPixel);
  SDL_RenderCopy(Renderer, Buffer->Texture, 0, 0);
  SDL_RenderPresent(Renderer);
}

internal void SDLProcessControllerButtonInput(GameButtonState *OldState,
                                              GameButtonState *NewState,
                                              SDL_GameController *handle,
                                              SDL_GameControllerButton Button) {
  NewState->EndedDown = SDL_GameControllerGetButton(handle, Button);
  NewState->HalfTransitionCount =
      (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal bool HandleEvent(SDLOffscreenBuffer *Buffer, SDL_Event *Event) {
  switch (Event->type) {

  case SDL_QUIT: {
    return true;
  } break;

  case SDL_WINDOWEVENT: {
    switch (Event->window.event) {

    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
      SDL_Renderer *Renderer = SDL_GetRenderer(Window);
      SDLWindowDimension WindowDimension = {
          .Width = Event->window.data1,
          .Height = Event->window.data2,
      };
      SDLResizeTexture(Buffer, Renderer, WindowDimension.Width,
                       WindowDimension.Height);
      SDLUpdateWindow(Buffer, Window, Renderer);
    } break;

    case SDL_WINDOWEVENT_FOCUS_GAINED: {
    } break;

    case SDL_WINDOWEVENT_EXPOSED: {
      SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
      SDL_Renderer *Renderer = SDL_GetRenderer(Window);
      SDLWindowDimension WindowDimension = SDLGetWindowDimension(Window);
      SDLResizeTexture(Buffer, Renderer, WindowDimension.Width,
                       WindowDimension.Height);
      SDLUpdateWindow(Buffer, Window, Renderer);
    } break;
    }
    break;
  } break;
  case SDL_CONTROLLERBUTTONDOWN:
  case SDL_CONTROLLERBUTTONUP:
  case SDL_KEYDOWN:
  case SDL_KEYUP: {
    SDL_Keycode KeyCode = Event->key.keysym.sym;
    bool IsDown(Event->key.state == SDL_PRESSED);
    bool WasDown = false;
    if (Event->key.state == SDL_RELEASED) {
      WasDown = true;
    } else if (Event->key.repeat != 0) {
      WasDown = true;
    }

    if (Event->key.repeat == 0) {
      if (KeyCode == SDLK_UP || KeyCode == SDLK_w) {
      } else if (KeyCode == SDLK_DOWN || KeyCode == SDLK_s) {
      } else if (KeyCode == SDLK_LEFT || KeyCode == SDLK_a) {
      } else if (KeyCode == SDLK_RIGHT || KeyCode == SDLK_d) {
      } else if (KeyCode == SDLK_ESCAPE) {
        printf("ESCAPE: ");
        if (IsDown) {
          printf("IsDown");
        }
        if (WasDown) {
          printf("WasDown");
        }
        printf("\n");
      } else if (KeyCode == SDLK_SPACE) {
        printf("SPACE: ");
        if (IsDown) {
          printf("IsDown");
        }
        if (WasDown) {
          printf("WasDown");
        }
        printf("\n");
      }
    }

    bool AltKeyWasDown = (Event->key.keysym.mod & KMOD_ALT);
    if ((KeyCode == SDLK_F4 || KeyCode == SDLK_q) && AltKeyWasDown) {
      return true;
    }
  } break;
  }

  return false;
}

void SDLOpenGameControllers() {
  int nJoysticks = SDL_NumJoysticks();
  int controllerIndex = 0;
  for (int joystickIndex = 0; joystickIndex < nJoysticks; ++joystickIndex) {
    if (!SDL_IsGameController(joystickIndex)) {
      continue;
    }
    if (controllerIndex >= MAX_CONTROLLERS) {
      break;
    }

    SDL_GameController *controller = SDL_GameControllerOpen(joystickIndex);
    ControllerHandles[controllerIndex] = controller;

    RumbleHandles[controllerIndex] =
        SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(controller));

    // remove if rumble is not supported
    if (SDL_HapticRumbleInit(RumbleHandles[controllerIndex]) != 0) {
      SDL_HapticClose(RumbleHandles[controllerIndex]);
      RumbleHandles[controllerIndex] = 0;
    }

    controllerIndex++;
  }
}

void SDLCloseGameControllers() {
  for (int i = 0; i < MAX_CONTROLLERS; i++) {
    if (ControllerHandles[i]) {
      SDL_GameControllerClose(ControllerHandles[i]);
      if (RumbleHandles[i]) {
        SDL_HapticClose(RumbleHandles[i]);
      }
    }
  }
}

internal void SDLInitAudio(int SamplesPerSecond, int BufferSize) {
  SDL_AudioSpec AudioSettings = {0};

  AudioSettings.freq = SamplesPerSecond;
  AudioSettings.format = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples = BufferSize;

  if (SDL_OpenAudio(&AudioSettings, 0) != 0) {
    printf("FAILED: SDL_OPENAUDIO");
  }

  if (AudioSettings.format != AUDIO_S16LSB) {
    printf("FAILED: AudioSettings format AUDIO_S16LSB not used");
    SDL_CloseAudio();
  }
}

internal void SDLFillAudioBuffer(SDLSoundOutput *SoundOutput, int BytesToWrite,
                                 GameSoundOutputBuffer *SoundBuffer) {
  if (SDL_QueueAudio(1, SoundBuffer->Samples, BytesToWrite) != 0) {
    printf("FAILED: SDL_QUEUEAUDIO");
  }
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC |
               SDL_INIT_AUDIO) != 0) {
    printf("FAILED: SDL_INIT: %s", SDL_GetError());
    return 1;
  };

  SDLOpenGameControllers();

  SDLSoundOutput SoundOutput = {0};
  SoundOutput.SamplesPerSecond = 48000;
  SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
  SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
  SoundOutput.TargetQueueBytes =
      SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample;
  SDLInitAudio(SoundOutput.SamplesPerSecond, SoundOutput.LatencySampleCount *
                                                 SoundOutput.BytesPerSample /
                                                 FPS);
  SDL_PauseAudio(0);
  int16_t *Samples = (int16_t *)calloc(SoundOutput.LatencySampleCount,
                                       SoundOutput.BytesPerSample);

  SDL_Window *Window =
      SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 800, 450, SDL_WINDOW_RESIZABLE);

  if (!Window) {
    printf("FAILED: SDL_CREATEWINDOW: %s", SDL_GetError());
    return 1;
  }

  SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);
  if (!Renderer) {
    printf("FAILED: SDL_CREATERENDERER: %s", SDL_GetError());
    return 1;
  }

  GameInput Input[2] = {};
  GameInput *OldInput = &Input[0];
  GameInput *NewInput = &Input[1];

  uint64_t PerfCountFrequency = SDL_GetPerformanceFrequency();
  uint64_t LastCounter = SDL_GetPerformanceCounter();
  uint64_t LastCycleCount = __rdtsc();
  GlobalRunning = true;
  while (GlobalRunning) {
    SDL_Event Event;

    while (SDL_PollEvent(&Event)) {
      if (HandleEvent(&GlobalBackBuffer, &Event)) {
        GlobalRunning = false;
      }
    }

    // Input
    int MaxControllerCount = MAX_CONTROLLERS;
    if (MaxControllerCount > ArrayCount(NewInput->Controllers)) {
      MaxControllerCount = ArrayCount(NewInput->Controllers);
    }
    for (int controllerIndex = 0; controllerIndex < MaxControllerCount;
         ++controllerIndex) {
      SDL_GameController *handle = ControllerHandles[controllerIndex];
      GameControllerInput *OldController =
          &OldInput->Controllers[controllerIndex];
      GameControllerInput *NewController =
          &NewInput->Controllers[controllerIndex];

      SDLProcessControllerButtonInput(&(OldController->Up),
                                      &(NewController->Up), handle,
                                      SDL_CONTROLLER_BUTTON_DPAD_UP);
      SDLProcessControllerButtonInput(&(OldController->Down),
                                      &(NewController->Down), handle,
                                      SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      SDLProcessControllerButtonInput(&(OldController->Left),
                                      &(NewController->Left), handle,
                                      SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      SDLProcessControllerButtonInput(&(OldController->Right),
                                      &(NewController->Right), handle,
                                      SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

      SDLProcessControllerButtonInput(&(OldController->LeftShoulder),
                                      &(NewController->LeftShoulder), handle,
                                      SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
      SDLProcessControllerButtonInput(&(OldController->RightShoulder),
                                      &(NewController->RightShoulder), handle,
                                      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      SDLProcessControllerButtonInput(&(OldController->AButton),
                                      &(NewController->AButton), handle,
                                      SDL_CONTROLLER_BUTTON_A);
      SDLProcessControllerButtonInput(&(OldController->XButton),
                                      &(NewController->XButton), handle,
                                      SDL_CONTROLLER_BUTTON_X);
      SDLProcessControllerButtonInput(&(OldController->BButton),
                                      &(NewController->BButton), handle,
                                      SDL_CONTROLLER_BUTTON_B);
      SDLProcessControllerButtonInput(&(OldController->YButton),
                                      &(NewController->YButton), handle,
                                      SDL_CONTROLLER_BUTTON_Y);

      NewController->IsAnalog = true;
      NewController->StartX = OldController->EndX;
      NewController->StartY = OldController->EndY;

      int16_t leftStickX =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTX);
      if (leftStickX < 0) {
        NewController->EndX = leftStickX / -32768.0f;
      } else {
        NewController->EndX = leftStickX / 32767.0f;
      }

      NewController->MinX = NewController->MaxX = OldController->EndX;

      int16_t leftStickY =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTY);
      if (leftStickY < 0) {
        NewController->EndY = leftStickY / 32768.0f;
      } else {
        NewController->EndY = leftStickY / 32767.0f;
      }

      NewController->MinY = NewController->MaxY = OldController->EndY;

      float rightStick =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_RIGHTX);
    }

    OffscreenBuffer Buffer = {};
    Buffer.Memory = GlobalBackBuffer.Pixels;
    Buffer.Width = GlobalBackBuffer.TextureWidth;
    Buffer.Height = GlobalBackBuffer.TextureHeight;
    Buffer.Pitch = GlobalBackBuffer.Pitch;

    GameSoundOutputBuffer SoundBuffer = {};
    int BytesToWrite = SoundOutput.TargetQueueBytes - SDL_GetQueuedAudioSize(1);
    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
    SoundBuffer.Samples = Samples;

#if HANDMADE_INTERNAL
    void *BaseAddress = (void *)Terabytes((uint64_t)2);
#else
    void *BaseAddress = 0;
#endif
    GameMemory Memory = {};
    Memory.PermanentStorageSize = (uint64_t)Megabytes(64);
    Memory.TransientStorageSize = (uint64_t)Gigabytes(4);

    uint64_t totalSize =
        Memory.PermanentStorageSize + Memory.TransientStorageSize;
    Memory.PermanentStorage =
        mmap(BaseAddress, totalSize, PROT_READ | PROT_WRITE,
             MAP_ANON | MAP_PRIVATE, -1, 0);

    Memory.TransientStorage =
        (uint8_t *)(Memory.PermanentStorage) + Memory.PermanentStorageSize;

    GameUpdateAndRender(&Memory, NewInput, &Buffer, &SoundBuffer);

    SDLFillAudioBuffer(&SoundOutput, BytesToWrite, &SoundBuffer);
    SDLUpdateWindow(&GlobalBackBuffer, Window, Renderer);

    // Performance
    uint64_t EndCycleCount = __rdtsc();
    uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
    float MCPF = ((float)CyclesElapsed / (1000.0f * 1000.0f));

    uint64_t EndCounter = SDL_GetPerformanceCounter();
    uint64_t CounterElapsed = EndCounter - LastCounter;
    float MSPerFrame =
        1000.0f * (float)CounterElapsed / (float)PerfCountFrequency;
    float MeasuredFPS = (float)PerfCountFrequency / (float)CounterElapsed;

    printf("Elapsed:%0.2fms FPS:%0.2f MCPF:%0.2f\n", MSPerFrame, MeasuredFPS,
           MCPF);

    LastCycleCount = EndCycleCount;
    LastCounter = EndCounter;

    GameInput *Temp = NewInput;
    NewInput = OldInput;
    OldInput = Temp;
  }

  SDLCloseGameControllers();
  SDL_CloseAudio();
  SDL_Quit();
  return 0;
}
