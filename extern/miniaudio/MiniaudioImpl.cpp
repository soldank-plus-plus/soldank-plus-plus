module;

#include <miniaudio.h>

module Extern.Miniaudio;

namespace Miniaudio
{
bool InitializeEngine(Engine& engine)
{
    return ma_engine_init(nullptr, &engine) == MA_SUCCESS;
}

void UninitializeEngine(Engine& engine)
{
    ma_engine_uninit(&engine);
}

bool InitializeSound(Engine& engine, const char* path, Sound& sound)
{
    return ma_sound_init_from_file(&engine,
                                   path,
                                   MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                   nullptr,
                                   nullptr,
                                   &sound) == MA_SUCCESS;
}

void UninitializeSound(Sound& sound)
{
    ma_sound_uninit(&sound);
}

void StartSound(Sound& sound)
{
    ma_sound_start(&sound);
}

void SetLooping(Sound& sound, bool looping)
{
    ma_sound_set_looping(&sound, looping ? MA_TRUE : MA_FALSE);
}

bool IsAtEnd(const Sound& sound)
{
    return ma_sound_at_end(&sound) == MA_TRUE;
}
} // namespace Miniaudio
