#pragma once

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "pianoKeyboard.hpp"
#include <string>

class UI {
public:
    static void GlfwErrorCallback(int error, const char* description);
    void setup();

    void update(PianoKeyboard keyboard);
    void setChordLabel(std::string newLabel);

private:
    bool setup_glfw(const char* &glslVersion, GLFWwindow* &window);
    GLFWwindow* window;
};