#pragma once

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "pianoKeyboard.hpp"
#include <string>
#include <unordered_set>

class UI {
public:
    static void GlfwErrorCallback(int error, const char* description);
    void setup();

    void update(PianoKeyboard keyboard);
    void setChordLabel(std::string newLabel);

    // ---- Added for the UI work (small additions to the original header) ----
    // Text for the "MIDI INPUT" box. connected = true shows it in green.
    void setConnectionStatus(std::string newStatus, bool connected);
    // True once the window has been closed (the main loop should stop).
    bool shouldClose() const;
    // Closes the window and cleans up ImGui and GLFW.
    void shutdown();
    // Keys currently held down with the mouse on the on-screen piano.
    std::unordered_set<int> getMouseKeys() const;

private:
    bool setup_glfw(const char* &glslVersion, GLFWwindow* &window);
    void drawStatusBox();
    void drawChordBox(const std::unordered_set<int>& pressedKeys);
    void drawPianoBox(const std::unordered_set<int>& pressedKeys);

    GLFWwindow* window = nullptr;
    std::string chordLabel = "--";
    std::string connectionStatus = "No MIDI device connected";
    bool connected = false;
    std::unordered_set<int> mouseKeys;
};