#include "mainApp.hpp"
#include "pianoKeyboard.hpp"

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <cstdio>
#include <vector>


namespace {
void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool isBlackKey(int midiKey) {
    switch (((midiKey %12) + 12) % 12) {
        case 1:
        case 3:
        case 6:
        case 8:
        case 10:
            return true;
        default:
            return false;
    }
}
} // namespace

MainApp::~MainApp() {
    shutdown();
}


bool MainApp::setupWindow() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        return false;
    }

#if defined(__APPLE__)
    gkfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glslVersion = "#version 150";
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glslVersion = "#version 330";
#endif

    window = glfwCreateWindow(1280, 800, "Chord Companion", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    return true;
}


void MainApp::setupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
}


bool MainApp::setup() {
    if (!setupWindow()) return false;
    setupImGui();
    initialized = true;

    keyboard.setup(); // lists MIDI devices and starts listening
    return true;
}


bool MainApp::shouldClose() const {
    return !initialized || glfwWindowShouldClose(window);
}


void MainApp::update() {
    if (!initialized) return;

    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
        glfwWaitEventsTimeout(0.1);
        return;
    }

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // One snapshot of held keys per frame
    const std::unordered_set<int> pressedKeys = keyboard.getPressedKeys();
    const ChordAnalysis analysis = chordLogic.analyze(pressedKeys);

    drawChordPanel(analysis, pressedKeys);
    drawPianoKeyboard(pressedKeys);

    ImGui::Render();

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}


void MainApp::shutdown() {
    if (!initialized) return;
    initialized = false;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    window = nullptr;
}


void MainApp::drawChordPanel(const ChordAnalysis& analysis, const std::unordered_set<int>& pressedKeys) {
    using Status = ChordAnalysis::Status;

    ImGui::Begin("Chord");

    if (analysis.status == Status::Empty) {
        ImGui::TextDisabled("Play some keys on the MIDI keyboard...");
    } else if (analysis.status == Status::TooManyNotes) {
        ImGui::TextWrapped("%d different notes held. Chord detection covers up to %d.",
            analysis.distinctNotes, ChordLogic::MAX_CHORD_NOTES);
    } else {
        // Big chord name
        ImGui::SetWindowFontScale(2.5f);
        ImGui::TextUnformatted(analysis.label.c_str());
        ImGui::SetWindowFontScale(1.0f);

        if (analysis.status == Status::Recognized) {
            ImGui::Text("%s", analysis.matches.front().typeName.c_str());

            if (analysis.matches.size() > 1) {
                ImGui::Separator();
                ImGui::TextDisabled("Also reads as:");

                for (std::size_t i = 1; i < analysis.matches.size(); ++i) {
                    ImGui::BulletText("%s  (%s)", analysis.matches[i].name.c_str(), analysis.matches[i].typeName.c_str());
                }
            }
        } else if (analysis.status == Status::Unnamed) {
            ImGui::TextDisabled("No name for this combination of notes.");
        }
    }

    ImGui::Separator();
    ImGui::Text("Held notes:");
    std::vector<int> sortedKeys(pressedKeys.begin(), pressedKeys.end());
    std::sort(sortedKeys.begin(), sortedKeys.end());
    for (int midiKey : sortedKeys) {
        ImGui::SameLine();
        ImGui::Text("%s", ChordLogic::midiNoteName(midiKey).c_str());
    }

    ImGui::End();
}


void MainApp::drawPianoKeyboard(const std::unordered_set<int>& pressedKeys) {
    constexpr int firstKey = 36; // C2
    constexpr int lastKey = 96;  // C7

    ImGui::Begin("Keyboard");

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float availableWidth = ImGui::GetContentRegionAvail().x;

    int whiteKeyCount = 0;
    for (int key = firstKey; key <= lastKey; ++key) {
        if (!isBlackKey(key)) ++whiteKeyCount;
    }

    const float whiteWidth = availableWidth / whiteKeyCount;
    const float whiteHeight = 160.0f;
    const float blackWidth = whiteWidth * 0.6f;
    const float blackHeight = whiteHeight * 0.6f;

    const ImU32 pressedColor = IM_COL32(90, 170, 255, 255);
    const ImU32 outline = IM_COL32(40, 40, 40, 255);

    // White keys first, then black keys on top.
    int whiteIndex = 0;
    for (int key = firstKey; key <= lastKey; ++key) {
        if (isBlackKey(key)) continue;

        const ImVec2 p0(origin.x + whiteIndex * whiteWidth, origin.y);
        const ImVec2 p1(p0.x + whiteWidth, origin.y + whiteHeight);

        drawList->AddRectFilled(p0, p1, pressedKeys.count(key) ? pressedColor : IM_COL32(240, 240, 240, 255));
        drawList->AddRect(p0, p1, outline);

        ++whiteIndex;
    }

    whiteIndex = 0;
    for (int key = firstKey; key <= lastKey; ++key) {
        if (!isBlackKey(key)) {
            ++whiteIndex;
            continue;
        }
        // A black key straddles the boundary after the last white key drawn.
        const float centerX = origin.x + whiteIndex * whiteWidth;
        const ImVec2 p0(centerX - blackWidth / 2, origin.y);
        const ImVec2 p1(centerX + blackWidth / 2, origin.y + blackHeight);

        drawList->AddRectFilled(p0, p1, pressedKeys.count(key) ? pressedColor : IM_COL32(25, 25, 25, 255));
        drawList->AddRect(p0, p1, outline);
    }

    ImGui::Dummy(ImVec2(availableWidth, whiteHeight));
    ImGui::End();
}


int main() {
    MainApp app;
    if (!app.setup()) {
        std::fprintf(stderr, "Failed to initialize Chord Companion .\n");
        return 1;
    }

    while (!app.shouldClose()) {
        app.update();
    }

    return 0;
}