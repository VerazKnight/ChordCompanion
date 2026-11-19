#pragma once

#include <fluidsynth.h>

class PianoSound {
public:
    PianoSound();
    ~PianoSound();
    
    void noteOn(int channel, int note, int velocity);
    void noteOff(int channel, int note);
    
private:
    bool setup();

private:
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = nullptr;
    fluid_audio_driver_t* adriver = nullptr;
};