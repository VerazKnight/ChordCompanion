#pragma once

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "chordTheory.hpp"
#include "learningSession.hpp"
#include "pianoInput.hpp"
#include <string>
#include <unordered_set>
#include <vector>

class UI {
public:
    static void GlfwErrorCallback(int error, const char* description);
    void setup();

    // Draw combined notes, but return only notes owned by mouse/computer input.
    std::unordered_set<int> update(std::unordered_set<int> pressedKeys = {});
    void setChordLabel(std::string newLabel);

    // ---- Added for the UI work (small additions to the original header) ----
    // Text for the "MIDI INPUT" box. connected = true shows it in green.
    void setConnectionStatus(std::string newStatus, bool connected);
    // True once the window has been closed (the main loop should stop).
    bool shouldClose() const;
    // Closes the window and cleans up ImGui and GLFW.
    void shutdown();
    // Keys held with the mouse: pressed while the button is down, plus keys
    // latched with Shift + click (Shift + click again to release).
    std::unordered_set<int> getMouseKeys() const;
    // How many seconds without a new chord before ideas appear (default 5).
    void setIdleSeconds(double seconds);
    // Skip the "Learner or Player?" question (true = Learner, false = Player).
    void setLearnerMode(bool learner);

private:
    // Learner: colors, ideas and teaching boxes.  Player: just the piano.
    // Unset: the user has not chosen yet, the question is shown.
    enum class Mode { Unset, Learner, Player };

    bool setup_glfw(const char* &glslVersion, GLFWwindow* &window);

    // Work out chords, key and ideas from the pressed keys.
    void updateComputerKeys();
    void updateAnalysis(const std::unordered_set<int>& pressedKeys);
    void computeGlow(const std::unordered_set<int>& pressedKeys);
    void changeMode(Mode newMode);

    void drawHeader();
    void drawNotice();
    void drawChordStrip(const std::unordered_set<int>& pressedKeys, ImVec2 size);
    void toggleFullscreen();
    std::string displayLabel() const;
    void drawModeChooser();
    void drawModeBar();
    void drawStatusBox(ImVec2 size);
    void drawChordBox(const std::unordered_set<int>& pressedKeys, ImVec2 size);
    void drawHistoryRow();
    void drawPianoBox(const std::unordered_set<int>& pressedKeys, ImVec2 size);
    void drawChordHelperBox(const std::unordered_set<int>& pressedKeys, ImVec2 size);
    void drawSuggestionBox(ImVec2 size);
    void drawInversionBox(ImVec2 size);

    GLFWwindow* window = nullptr;
    std::string chordLabel = "--";
    std::string connectionStatus = "No MIDI device connected";
    bool connected = false;

    Mode mode = Mode::Unset;

    // ---- window size and layout ----
    bool narrow = false;              // narrow or tall window: one column
    bool bigKeyboard = false;         // the piano fills the window (B key)
    bool fullscreen = false;          // F11
    bool wantFullscreenToggle = false;
    int windowedX = 100, windowedY = 100, windowedW = 1280, windowedH = 880;

    // Mouse and computer keyboard keys (see pianoInput.hpp).
    PianoInput::State input;

    // The teaching logic: chords played, key, ideas, notice, practice
    // (see learningSession.hpp).
    Learning::Session session;

    // ---- learning features ----
    double idleSeconds = 5.0;
    bool autoSuggest = true;          // show ideas after a pause
    bool showNoteNames = true;        // note names on every key
    bool showDots = true;             // show the colored dots
    // The inversion box example: -1 = follow what the player plays.
    int exampleRootPc = -1;
    int exampleType = 0;

    std::vector<int> glowNotes;       // piano keys drawn with a yellow glow
    std::string glowCaption;
};
