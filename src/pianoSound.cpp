#include "pianoSound.hpp"

#include <cstdio>
#include <cstring>
#include <new>
#include <stdexcept>

#include "embedded_sf2.h"

namespace {
struct SoundFontFile {
    fluid_long_long_t position = 0;
};

void* sfOpen(const char* filename) {
    if (std::strcmp(filename, "embedded.sf2") != 0) {
        return nullptr;
    }

    return new (std::nothrow) SoundFontFile;
}

int sfRead(void* buffer, fluid_long_long_t count, void* handle) {
    auto* file = static_cast<SoundFontFile*>(handle);
    const auto size = static_cast<fluid_long_long_t>(embedded_sf2_len);

    // FluidSynth requires exact reads; partial reads count as failure.
    if (count < 0 || count > size - file->position) {
        return FLUID_FAILED;
    }

    std::memcpy(
        buffer,
        embedded_sf2 + file->position,
        static_cast<std::size_t>(count)
    );

    file->position += count;
    return FLUID_OK;
}

int sfSeek(void* handle, fluid_long_long_t offset, int origin) {
    auto* file = static_cast<SoundFontFile*>(handle);
    const auto size = static_cast<fluid_long_long_t>(embedded_sf2_len);

    fluid_long_long_t base;
    switch (origin) {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = file->position;
            break;
        case SEEK_END:
            base = size;
            break;
        default:
            return FLUID_FAILED;
    }

    // Check bounds before addition to avoid integer overflow.
    if (offset < -base || offset > size - base) {
        return FLUID_FAILED;
    }

    file->position = base + offset;
    return FLUID_OK;
}

fluid_long_long_t sfTell(void* handle) {
    return static_cast<SoundFontFile*>(handle)->position;
}

int sfClose(void* handle) {
    delete static_cast<SoundFontFile*>(handle);
    return FLUID_OK;
}
}  // namespace


PianoSound::PianoSound() {
    if (!setup()) {
        throw std::runtime_error("Failed to init PianoSound");
    }
}

PianoSound::~PianoSound() {
    if (adriver) {
        delete_fluid_audio_driver(adriver);
    }

    if (synth) {
        delete_fluid_synth(synth);
    }

    if (settings) {
        delete_fluid_settings(settings);
    }
}

bool PianoSound::setup() {
    fluid_set_log_function(FLUID_DBG, nullptr, nullptr);
    
    settings = new_fluid_settings();
    if (!settings) {
        return false;
    }

    synth = new_fluid_synth(settings);
    if (!synth) {
        return false;
    }

    auto* loader = new_fluid_defsfloader(settings);
    if (!loader) {
        return false;
    }

    fluid_sfloader_set_callbacks(
        loader,
        sfOpen,
        sfRead,
        sfSeek,
        sfTell,
        sfClose
    );

    fluid_synth_add_sfloader(synth, loader);

    // Virtual filename handled by sfOpen(), not a disk path.
    int sfont_id = fluid_synth_sfload(synth, "embedded.sf2", 1);
    if (sfont_id == FLUID_FAILED) {
        return false;
    }

    if (fluid_synth_program_select(synth, 0, sfont_id, 0, 0) != FLUID_OK) {
        throw std::runtime_error("Soundfont loaded, but bank 0 / preset 0 could not be selected");
    }

    adriver = new_fluid_audio_driver(settings, synth);
    if (!adriver) {
        throw std::runtime_error("Failed to create FluidSynth audio driver");
    }

    return true;
}

void PianoSound::noteOn(int channel, int note, int velocity) {
    if (fluid_synth_noteon(synth, channel, note, velocity) != FLUID_OK) {
        throw std::runtime_error("FluidSynth note-on failed");
    }
}

void PianoSound::noteOff(int channel, int note) {
    if (fluid_synth_noteoff(synth, channel, note) != FLUID_OK) {
        throw std::runtime_error("FluidSynth note-off failed");
    }
}