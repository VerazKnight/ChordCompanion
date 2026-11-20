#pragma once

#include <vector>
#include <array>
#include <mutex>
#include <unordered_set>
#include <RtMidi.h>
#include "pianoSound.hpp"

// Input ownership is tracked per MIDI channel/note. UI notes use channel 0.
// The caller synchronizes access and sends the returned transitions to audio.
class NoteState {
public:
    struct Event { int channel; int note; int velocity; }; // zero = note off

    std::vector<Event> setMidi(int channel, int note, int velocity) {
        if (channel < 0 || channel >= 16 || note < 0 || note >= 128 ||
            velocity < 0 || velocity > 127) return {};
        const bool before = held(channel, note);
        midi[channel][note] = velocity != 0;
        const bool after = held(channel, note);
        if (before == after) return {};
        return {{channel, note, after ? velocity : 0}};
    }

    std::vector<Event> setUi(const std::unordered_set<int>& keys) {
        std::vector<Event> events;
        for (int note = 0; note < 128; ++note) {
            const bool before = held(0, note);
            ui[note] = keys.contains(note);
            const bool after = held(0, note);
            if (before != after) events.push_back({0, note, after ? 100 : 0});
        }
        return events;
    }

    std::unordered_set<int> pressedKeys() const {
        std::unordered_set<int> keys;
        for (int channel = 0; channel < 16; ++channel)
            for (int note = 0; note < 128; ++note)
                if (held(channel, note)) keys.insert(note);
        return keys;
    }

private:
    bool held(int channel, int note) const {
        return midi[channel][note] || (channel == 0 && ui[note]);
    }
    std::array<std::array<bool, 128>, 16> midi{};
    std::array<bool, 128> ui{};
};


class PianoKeyboard {
public:
    PianoKeyboard();
    ~PianoKeyboard();

    static void handleMIDI(double deltatime, std::vector<unsigned char> *message, void *userData);
    void setUiPressedKeys(const std::unordered_set<int>& keys);
    std::unordered_set<int> getPressedKeys();

private: 
    void setup();
    void playEvents(const std::vector<NoteState::Event>& events);

private:
    std::mutex stateMutex;
    NoteState notes;
    RtMidiIn* midiDevice = nullptr;
    PianoSound sound;
};