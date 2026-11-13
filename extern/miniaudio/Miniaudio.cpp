module;

#include <miniaudio.h>

export module Extern.Miniaudio;

export namespace Miniaudio
{
using Engine = ma_engine;
using Sound = ma_sound;

bool InitializeEngine(Engine& engine);
void UninitializeEngine(Engine& engine);
bool InitializeSound(Engine& engine, const char* path, Sound& sound);
void UninitializeSound(Sound& sound);
void StartSound(Sound& sound);
void SetLooping(Sound& sound, bool looping);
bool IsAtEnd(const Sound& sound);
} // namespace Miniaudio
