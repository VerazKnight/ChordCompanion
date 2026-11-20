#pragma once

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "chordTheory.hpp"
#include "pianoKeyboard.hpp"
#include <string>
#include <unordered_set>
#include <vector>

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
    void buildSuggestions();
    std::string makeFeedback(int oldKeyPc, const ChordTheory::IdentifiedChord& played) const;
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

    // Keys held with the mouse (held + latched) and with the computer keyboard.
    std::unordered_set<int> mouseKeys;
    std::unordered_set<int> mouseLatched;   // Shift + click keys
    std::unordered_set<int> computerKeys;
    int octaveShift = 0;              // 0 = A is C4, 12 = A is C5 (Z / X keys)

    // ---- learning features ----
    double idleSeconds = 5.0;
    double lastChangeTime = 0.0;      // when the pressed keys last changed
    double pausedFor = 0.0;           // seconds since then
    bool hasPlayed = false;           // has anything been pressed yet?
    bool autoSuggest = true;          // show ideas after a pause
    bool showNoteNames = true;        // note names on every key
    bool showDots = true;             // show the colored dots
    std::unordered_set<int> lastPressed;

    ChordTheory::IdentifiedChord currentChord;        // chord held right now
    std::vector<ChordTheory::PlayedChord> history;    // last chords played
    int keyPc = -1;                                   // detected key, -1 = none

    // Ideas stay on screen until the player plays a new chord. Playing
    // something else is always fine.
    bool suggestionsVisible = false;
    bool suggestRequested = false;    // the player pressed "Show ideas now"
    bool suggestedByPause = false;
    std::vector<ChordTheory::Suggestion> suggestions;
    std::string bridgeText;           // note about an outside chord
    std::string feedback;             // friendly reply to the chord just played
    int selectedSuggestion = -1;

    // The "Highlight chord" dropdowns. highlightType -1 means None.
    int highlightType = -1;
    int highlightInversion = 0;
    int highlightRootPc = -1;         // -1 = use the pressed note's name

    // The inversion box example: -1 = follow what the player plays.
    int exampleRootPc = -1;
    int exampleType = 0;

    // A small, non-blocking note shown when a chord is outside the key.
    // The player can ignore it; it fades by itself.
    bool noticeVisible = false;
    ChordTheory::PlayedChord noticeChord;
    int noticeKeyPc = -1;             // the key the chord does not belong to
    double noticeTime = 0.0;
    int ideasKeyPc = 0;               // the key the ideas are given in

    // Practice: the player picked a chord to try (Show button). It stays until
    // they get it or press Clear. Playing other chords in between is fine.
    std::string practiceMsg;
    int practiceMood = 0;             // 0 hint, 1 happy, 2 oops (friendly)
    double practiceTime = 0.0;
    void checkPractice(const ChordTheory::IdentifiedChord& played);

    std::vector<int> glowNotes;       // piano keys drawn with a yellow glow
    std::string glowCaption;
};