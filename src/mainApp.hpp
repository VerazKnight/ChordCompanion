#pragma once

#include "pianoKeyboard.hpp"
#include "ui.hpp"

class MainApp {
public:
    ~MainApp();
    void setup();
    void update();
    void run();

private:
    PianoKeyboard keyboard;
    UI ui;
};
