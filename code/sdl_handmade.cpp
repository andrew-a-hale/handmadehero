#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_haptic.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <sys/mman.h>

#define internal static
#define local_persist static
#define global_variable static
#define MAX_CONTROLLERS 4

struct SDLOffscreenBuffer {
  SDL_Texture *Texture;
  void *Pixels;
  int TextureWidth;
  int TextureHeight;
  int Pitch;
  int BytesPerPixel;
};

global_variable bool GlobalRunning;
global_variable SDLOffscreenBuffer GlobalBackBuffer;
global_variable SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
global_variable SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];

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
  SDL_RenderClear(Renderer);
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
    bool WasDown = false;
    if (Event->key.state == SDL_RELEASED) {
      WasDown = true;
    } else if (Event->key.repeat != 0) {
      WasDown = true;
    }
    switch (KeyCode) {
    case SDLK_w: {
      printf("w\n");
    } break;
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

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) !=
      0) {
    printf("FAILED: SDL_INIT: %s", SDL_GetError());
    return 1;
  };
  SDLOpenGameControllers();

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

  GlobalRunning = true;

  int XOffset = 0;
  int YOffset = 0;
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

    // Render
    RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
    SDLUpdateWindow(&GlobalBackBuffer, Window, Renderer);
  }

  SDLCloseGameControllers();
  SDL_Quit();
  return 0;
}
