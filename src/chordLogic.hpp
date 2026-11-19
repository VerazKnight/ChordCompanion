#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>

class ChordLogic {
public:
    std::string identifyChord(std::unordered_set<int> pressedKeys);
private:
    void setup();
    std::unordered_map<std::unordered_set<int>, std::string> chords;
};


