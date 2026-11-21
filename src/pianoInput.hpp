#pragma once

// pianoInput.hpp
//
// Which piano keys are being held by the mouse and by the computer keyboard.
// No ImGui or GLFW in here on purpose: ui.cpp reads the real mouse and the
// real keyboard and passes the facts in, so this logic can be tested with a
// fake input (no window needed).

#include "pianoLayout.hpp"

#include <unordered_set>
#include <vector>

namespace PianoInput {

// Like a piano: A S D F ... are the white keys, W E T Y ... the black keys.
// This is the low octave (C4 = 60 to E5 = 76). Z / X shift it up an octave.
struct ComputerKeyDef {
    const char* letter;
    int note;
};

inline const std::vector<ComputerKeyDef>& computerKeys() {
    static const std::vector<ComputerKeyDef> keys = {
        { "A", 60 }, { "W", 61 }, { "S", 62 }, { "E", 63 }, { "D", 64 },
        { "F", 65 }, { "T", 66 }, { "G", 67 }, { "Y", 68 }, { "H", 69 },
        { "U", 70 }, { "J", 71 }, { "K", 72 }, { "O", 73 }, { "L", 74 },
        { "P", 75 }, { ";", 76 },
    };
    return keys;
}

// The piano note played by a computer key letter, or -1 if the letter is not
// a piano key or the note would be off the piano. octaveShift is 0 or 12.
inline int computerKeyNote(char letter, int octaveShift) {
    for (const ComputerKeyDef& k : computerKeys()) {
        if (k.letter[0] == letter) {
            int note = k.note + octaveShift;
            return PianoLayout::isInRange(note) ? note : -1;
        }
    }
    return -1;
}

// The letter printed on a piano key ("" if no computer key plays it).
inline const char* computerKeyLetter(int note) {
    for (const ComputerKeyDef& k : computerKeys()) {
        if (k.note == note) {
            return k.letter;
        }
    }
    return "";
}

class State {
public:
    // Keys held by the mouse: held while the button is down, plus keys
    // latched with Shift + click (Shift + click again lets go).
    std::unordered_set<int> mouseKeys;
    std::unordered_set<int> mouseLatched;
    std::unordered_set<int> computerKeys;
    int octaveShift = 0;  // 0 = A is C4, 12 = A is C5

    // noteUnderMouse is -1 when the mouse is not over a key.
    void updateMouse(int noteUnderMouse, bool leftDown, bool leftClicked, bool shift) {
        mouseKeys = mouseLatched;
        if (noteUnderMouse == -1) {
            return;
        }
        if (shift) {
            if (leftClicked) {
                if (mouseLatched.erase(noteUnderMouse) == 0) {
                    mouseLatched.insert(noteUnderMouse);
                }
                mouseKeys = mouseLatched;
            }
        } else if (leftDown) {
            mouseKeys.insert(noteUnderMouse);
        }
    }

    // heldLetters: the computer keys that are down right now.
    void updateComputer(const std::vector<char>& heldLetters) {
        computerKeys.clear();
        for (char letter : heldLetters) {
            int note = computerKeyNote(letter, octaveShift);
            if (note != -1) {
                computerKeys.insert(note);
            }
        }
    }

    void setHighOctave(bool high) { octaveShift = high ? 12 : 0; }

    void releaseMouseKeys() {
        mouseLatched.clear();
        mouseKeys.clear();
    }

    // Everything that is sounding: MIDI keys + mouse + computer keyboard.
    std::unordered_set<int> allPressed(const std::unordered_set<int>& midiKeys) const {
        std::unordered_set<int> all = midiKeys;
        all.insert(mouseKeys.begin(), mouseKeys.end());
        all.insert(computerKeys.begin(), computerKeys.end());
        return all;
    }
};

}  // namespace PianoInput
