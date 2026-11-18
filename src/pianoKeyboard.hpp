#include <vector>
#include <span>
#include <unordered_set>
#include "RtMidi.h"

class PianoKeyboard {
public:
    void setup();
    static void handleMIDI(double, std::vector<unsigned char>, void*);
    void setPressedKeys(std::span<const int>);
    void releasedPressedKeys(std::span<const int>);
    std::unordered_set<int> getPressedKeys();
private:
    std::unordered_set<int> pressedKeys;
    RtMidiIn midiDevice;
    // PianoSound sound;
};