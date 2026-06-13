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
#include <stdint.h>
#include <sys/mman.h>

#define internal static
#define local_persist static
#define global_variable static

#define MAX_CONTROLLERS 4
#define PI 3.14159265359f
#define TAU 2.0f * PI

struct SDLOffscreenBuffer {
  SDL_Texture *Texture;
  void *Pixels;
  int TextureWidth;
  int TextureHeight;
  int Pitch;
  int BytesPerPixel;
};

struct SDLSoundOutput {
  void *AudioBuffer;
  int SamplesPerSecond;
  int BytesPerSample;
  int TargetQueueBytes;
  int ToneVolume;
  int LatencySampleCount;
  float t;
};

global_variable int FPS = 60;
global_variable bool GlobalRunning;
global_variable SDLOffscreenBuffer GlobalBackBuffer;
global_variable SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
global_variable SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

// RENDER
global_variable int XOffset = 0;
global_variable int YOffset = 0;
global_variable int Speed = 10;
global_variable int ToneHz = 256;

struct SDL_WindowDimension {
  int Width;
  int Height;
};

SDL_WindowDimension SDLGetWindowDimension(SDL_Window *Window) {
  SDL_WindowDimension Result;
  SDL_GetWindowSize(Window, &Result.Width, &Result.Height);
  return Result;
}

internal void RenderWeirdGradient(SDLOffscreenBuffer *Buffer, int BlueOffset,
                                  int GreenOffset) {
  uint8_t *Row = (uint8_t *)Buffer->Pixels;
  for (int y = 0; y < Buffer->TextureHeight; ++y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int x = 0; x < Buffer->TextureWidth; ++x) {
      uint8_t Blue = x + BlueOffset;
      uint8_t Green = y + GreenOffset;
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
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
      SDL_WindowDimension WindowDimension = {
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
      SDL_WindowDimension WindowDimension = SDLGetWindowDimension(Window);
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
        YOffset -= Speed;
      } else if (KeyCode == SDLK_DOWN || KeyCode == SDLK_s) {
        YOffset += Speed;
      } else if (KeyCode == SDLK_LEFT || KeyCode == SDLK_a) {
        XOffset -= Speed;
      } else if (KeyCode == SDLK_RIGHT || KeyCode == SDLK_d) {
        XOffset += Speed;
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
          ToneHz *= 2;
        }
        if (WasDown) {
          printf("WasDown");
          ToneHz /= 2;
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

internal void SDLFillAudioBuffer(SDLSoundOutput *SoundOutput, int ToneHz) {
  int WavePeriod = SoundOutput->SamplesPerSecond / ToneHz;
  int BytesToWrite = SoundOutput->TargetQueueBytes - SDL_GetQueuedAudioSize(1);
  int SampleCount = BytesToWrite / SoundOutput->BytesPerSample;

  SoundOutput->AudioBuffer = malloc(BytesToWrite);
  int16_t *SampleOut = (int16_t *)SoundOutput->AudioBuffer;

  for (int SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
    float SineValue = sinf(SoundOutput->t);
    int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    SoundOutput->t += TAU * 1.0f / (float)WavePeriod;
    if (SoundOutput->t > TAU) {
      SoundOutput->t -= TAU;
    }
  }

  if (SDL_QueueAudio(1, SoundOutput->AudioBuffer, BytesToWrite) != 0) {
    printf("FAILED: SDL_QUEUEAUDIO");
  }

  free(SoundOutput->AudioBuffer);
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
  SoundOutput.t = 0;
  SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
  SoundOutput.LatencySampleCount =
      SoundOutput.SamplesPerSecond / 15; // 4 frames
  SoundOutput.TargetQueueBytes =
      SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample;
  SoundOutput.ToneVolume = 3000;
  SDLInitAudio(SoundOutput.SamplesPerSecond, SoundOutput.LatencySampleCount *
                                                 SoundOutput.BytesPerSample /
                                                 FPS);
  SDL_PauseAudio(0);

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
    // NOTE: Some input are SDL Events
    // TODO: poll more often?
    for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS;
         ++controllerIndex) {
      SDL_GameController *handle = ControllerHandles[controllerIndex];
      bool up =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_UP);
      bool down =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      bool left =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      bool right =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
      bool start =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_START);
      bool back =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_BACK);
      bool leftShoulder = SDL_GameControllerGetButton(
          handle, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
      bool rightShoulder = SDL_GameControllerGetButton(
          handle, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      bool aButton =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_A);
      bool bButton =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B);
      bool xButton =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_X);
      bool yButton =
          SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y);
      int16_t leftStickX =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTX);
      int16_t leftStickY =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTY);
      int16_t rightStick =
          SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_RIGHTX);

      if (leftStickX != 0) {
        XOffset += leftStickX / 8000;
      }

      if (leftStickY != 0) {
        YOffset += leftStickY / 8000;
      }

      if (bButton) {
        if (RumbleHandles[controllerIndex]) {
          SDL_HapticRumblePlay(RumbleHandles[controllerIndex], 0.5f, 500);
        }
      }
    }

    RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
    SDLFillAudioBuffer(&SoundOutput, ToneHz);
    SDLUpdateWindow(&GlobalBackBuffer, Window, Renderer);

    // Performance
    uint64_t EndCycleCount = __rdtsc();
    uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
    double MCPF = ((double)CyclesElapsed / (1000.0f * 1000.0f));
    LastCycleCount = EndCycleCount;

    uint64_t EndCounter = SDL_GetPerformanceCounter();
    uint64_t CounterElapsed = EndCounter - LastCounter;
    double MSPerFrame = 1000.0f * CounterElapsed / PerfCountFrequency;
    double MeasuredFPS = (double)PerfCountFrequency / CounterElapsed;
    printf("Elapsed:%0.2fms FPS:%0.2f MCPF:%0.2f\n", MSPerFrame, MeasuredFPS,
           MCPF);
    LastCounter = EndCounter;
  }

  SDLCloseGameControllers();
  SDL_CloseAudio();
  SDL_Quit();
  return 0;
}
