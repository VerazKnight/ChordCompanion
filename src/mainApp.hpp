#pragma once

#include "pianoKeyboard.hpp"

class MainApp {
public:
    void setup();
    void update(); 

private:
    PianoKeyboard keyboard;
};