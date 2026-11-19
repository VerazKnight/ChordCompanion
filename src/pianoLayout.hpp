#pragma once

// pianoLayout.hpp
//
// Plain helper functions that describe the on-screen piano: which MIDI
// notes are black keys, what a note is called, where a key sits, and which
// key is under the mouse.
//
// IMPORTANT: this file does not use ImGui, GLFW or OpenGL. That is on
// purpose. It means these functions can be unit tested without opening a
// window, and ui.cpp only has to draw what these functions decide.

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace PianoLayout {

// The piano on screen shows two octaves: C4 (MIDI 60) up to B5 (MIDI 83).
constexpr int FIRST_NOTE = 60;
constexpr int LAST_NOTE = 83;
constexpr int WHITE_KEY_COUNT = 14;

// Key sizes in pixels.
constexpr float WHITE_KEY_WIDTH = 40.0f;
constexpr float WHITE_KEY_HEIGHT = 160.0f;
constexpr float BLACK_KEY_WIDTH = 26.0f;
constexpr float BLACK_KEY_HEIGHT = 100.0f;

// True if the note is one of the notes drawn on the piano.
inline bool isInRange(int note) {
    return note >= FIRST_NOTE && note <= LAST_NOTE;
}

// True if the note is a black key (C#, D#, F#, G#, A#).
inline bool isBlackKey(int note) {
    int pitchClass = ((note % 12) + 12) % 12;
    return pitchClass == 1 || pitchClass == 3 || pitchClass == 6 ||
           pitchClass == 8 || pitchClass == 10;
}

// Name of a MIDI note, for example 60 -> "C4" and 61 -> "C#4".
// Returns an empty string if the number is not a valid MIDI note (0 to 127).
inline std::string noteName(int note) {
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B" };
    if (note < 0 || note > 127) {
        return "";
    }
    return std::string(names[note % 12]) + std::to_string(note / 12 - 1);
}

// Position of a white key counted from the left (C4 is 0, D4 is 1, ...).
// Returns -1 for black keys and for notes outside the piano.
inline int whiteKeyIndex(int note) {
    if (!isInRange(note) || isBlackKey(note)) {
        return -1;
    }
    int index = 0;
    for (int n = FIRST_NOTE; n < note; n++) {
        if (!isBlackKey(n)) {
            index++;
        }
    }
    return index;
}

// Left edge of a key in pixels, measured from the left edge of the piano.
// A black key sits on the line between the white key before it and the
// white key after it. Returns -1 for notes outside the piano.
inline float keyX(int note) {
    if (!isInRange(note)) {
        return -1.0f;
    }
    if (!isBlackKey(note)) {
        return whiteKeyIndex(note) * WHITE_KEY_WIDTH;
    }
    // The note just below a black key is always a white key.
    float previousWhiteX = whiteKeyIndex(note - 1) * WHITE_KEY_WIDTH;
    return previousWhiteX + WHITE_KEY_WIDTH - BLACK_KEY_WIDTH / 2.0f;
}

// Which key is at a point? x and y are measured from the top left corner of
// the piano. Returns the MIDI note, or -1 if the point is not on a key.
// Black keys are checked first because they sit on top of the white keys.
inline int noteAtPoint(float x, float y) {
    if (x < 0.0f || y < 0.0f || y >= WHITE_KEY_HEIGHT ||
        x >= WHITE_KEY_COUNT * WHITE_KEY_WIDTH) {
        return -1;
    }

    if (y < BLACK_KEY_HEIGHT) {
        for (int n = FIRST_NOTE; n <= LAST_NOTE; n++) {
            if (isBlackKey(n)) {
                float left = keyX(n);
                if (x >= left && x < left + BLACK_KEY_WIDTH) {
                    return n;
                }
            }
        }
    }

    int column = static_cast<int>(x / WHITE_KEY_WIDTH);
    for (int n = FIRST_NOTE; n <= LAST_NOTE; n++) {
        if (whiteKeyIndex(n) == column) {
            return n;
        }
    }
    return -1;
}

// From a set of pressed notes, the notes the piano should light up:
// only notes that are on the piano, in order from low to high.
inline std::vector<int> keysToHighlight(const std::unordered_set<int>& pressed) {
    std::vector<int> result;
    for (int note : pressed) {
        if (isInRange(note)) {
            result.push_back(note);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Text for the "Pressed:" line, for example "C4 E4 G4".
// Returns "None" when no keys are pressed.
inline std::string describeKeys(const std::vector<int>& notes) {
    if (notes.empty()) {
        return "None";
    }
    std::string text;
    for (size_t i = 0; i < notes.size(); i++) {
        if (i > 0) {
            text += " ";
        }
        text += noteName(notes[i]);
    }
    return text;
}

}  // namespace PianoLayout