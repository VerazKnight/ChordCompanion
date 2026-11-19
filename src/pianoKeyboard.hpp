#include <vector>
#include <span>
#include <unordered_set>
#include "RtMidi.h"

class PianoKeyboard {
public:
    void setup();
    static void handleMIDI(double deltatime, std::vector<unsigned char> *message, void *userData);
    void setPressedKeys(std::span<const int> keys);
    void releasedPressedKeys(std::span<const int> keys);
    std::unordered_set<int> getPressedKeys();
private:
    std::unordered_set<int> pressedKeys;
    RtMidiIn* midiDevice = nullptr;
    // PianoSound sound;
};