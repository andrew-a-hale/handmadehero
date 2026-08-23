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
