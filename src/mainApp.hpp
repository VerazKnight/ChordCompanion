#pragma once

#include "pianoKeyboard.hpp"
#include "ui.hpp"

class MainApp {
public:
    MainApp() = default;
    ~MainApp();
    MainApp(const MainApp&) = delete;
    MainApp& operator=(const MainApp&) = delete;

    void setup(); // initialize the UI; throws if initialization fails
    void update(); // draw combined input and send UI-owned notes to the keyboard
    void run(); // setup, then update until the UI closes

private:
    PianoKeyboard keyboard;
    // UI owns the GLFW window and ImGui lifecycle.
    UI ui;
};
