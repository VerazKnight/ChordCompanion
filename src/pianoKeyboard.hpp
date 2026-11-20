#pragma once

#include <vector>
#include <span>
#include <unordered_set>
#include <RtMidi.h>
#include "pianoSound.hpp"

class PianoKeyboard {
public:
    PianoKeyboard();
    ~PianoKeyboard();

    static void handleMIDI(double deltatime, std::vector<unsigned char> *message, void *userData);
    void setPressedKeys(std::span<const int> keys);
    void releasePressedKeys(std::span<const int> keys);
    std::unordered_set<int> getPressedKeys();

private: 
    void setup();

private:
    std::unordered_set<int> pressedKeys;
    RtMidiIn* midiDevice = nullptr;
    PianoSound sound;
};