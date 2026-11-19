#include "ui.hpp"
#include "pianoLayout.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <utility>

namespace CT = ChordTheory;

// ------------------------------------------------------------------ colors

static const ImU32 COLOR_WHITE_KEY = IM_COL32(255, 255, 255, 255);
static const ImU32 COLOR_WHITE_KEY_PRESSED = IM_COL32(120, 190, 255, 255);
static const ImU32 COLOR_WHITE_KEY_GLOW = IM_COL32(255, 232, 130, 255);
static const ImU32 COLOR_BLACK_KEY = IM_COL32(20, 20, 20, 255);
static const ImU32 COLOR_BLACK_KEY_PRESSED = IM_COL32(80, 140, 220, 255);
static const ImU32 COLOR_BLACK_KEY_GLOW = IM_COL32(215, 170, 30, 255);
static const ImU32 COLOR_GLOW_OUTLINE = IM_COL32(255, 200, 40, 255);
static const ImU32 COLOR_OUTLINE = IM_COL32(0, 0, 0, 255);
static const ImU32 COLOR_KEY_LABEL = IM_COL32(90, 90, 90, 255);
static const ImU32 COLOR_CHORD_TEXT = IM_COL32(120, 190, 255, 255);
static const ImU32 COLOR_TEXT = IM_COL32(230, 230, 235, 255);
static const ImU32 COLOR_DARK_TEXT = IM_COL32(20, 20, 30, 255);
static const ImU32 COLOR_PAUSE_CHIP = IM_COL32(255, 200, 60, 255);
static const ImU32 COLOR_KEY_CHIP = IM_COL32(120, 200, 150, 255);

// One color per chord type, used for the dots and the chips.
static ImU32 typeColor(int typeIndex) {
    switch (typeIndex) {
        case CT::MAJOR: return IM_COL32(80, 150, 255, 255);       // blue
        case CT::MINOR: return IM_COL32(255, 160, 60, 255);       // orange
        case CT::DIMINISHED: return IM_COL32(180, 110, 235, 255); // purple
        case CT::AUGMENTED: return IM_COL32(80, 200, 120, 255);   // green
        default: return IM_COL32(200, 200, 200, 255);
    }
}

static ImVec2 add(ImVec2 a, float x, float y) {
    return ImVec2(a.x + x, a.y + y);
}

// ------------------------------------------------ computer keyboard as piano

struct ComputerKey {
    ImGuiKey key;
    int note;
    const char* letter;
};

// Like a piano: A S D F ... are the white keys, W E T Y ... the black keys.
static const ComputerKey COMPUTER_KEYS[] = {
    { ImGuiKey_A, 60, "A" }, { ImGuiKey_W, 61, "W" }, { ImGuiKey_S, 62, "S" },
    { ImGuiKey_E, 63, "E" }, { ImGuiKey_D, 64, "D" }, { ImGuiKey_F, 65, "F" },
    { ImGuiKey_T, 66, "T" }, { ImGuiKey_G, 67, "G" }, { ImGuiKey_Y, 68, "Y" },
    { ImGuiKey_H, 69, "H" }, { ImGuiKey_U, 70, "U" }, { ImGuiKey_J, 71, "J" },
    { ImGuiKey_K, 72, "K" }, { ImGuiKey_O, 73, "O" }, { ImGuiKey_L, 74, "L" },
    { ImGuiKey_P, 75, "P" }, { ImGuiKey_Semicolon, 76, ";" },
};

static const char* computerKeyLetter(int note) {
    for (const ComputerKey& k : COMPUTER_KEYS) {
        if (k.note == note) {
            return k.letter;
        }
    }
    return "";
}

// "C4 E4 G4" for a list of notes.
static std::string noteNames(const std::vector<int>& notes) {
    std::string text;
    for (int note : notes) {
        if (!text.empty()) {
            text += " ";
        }
        text += PianoLayout::noteName(note);
    }
    return text;
}

// ------------------------------------------------------------------- setup

void UI::GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool UI::setup_glfw(const char* &glslVersion, GLFWwindow* &window) {
    glfwSetErrorCallback(UI::GlfwErrorCallback);

    if (!glfwInit()) {
        return false;
    }

#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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

    window = glfwCreateWindow(1280, 800, "ChordCompanion", nullptr, nullptr);

    if (window == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return true;
}

void UI::setup() {
    const char* glslVersion = nullptr;
    if (!setup_glfw(glslVersion, window)) {
        std::fprintf(stderr, "UI: GLFW failed to set up\n");
        window = nullptr;
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void UI::setChordLabel(std::string newLabel) {
    // An empty label would leave a blank space, so show "--" instead.
    chordLabel = newLabel.empty() ? "--" : std::move(newLabel);
}

void UI::setConnectionStatus(std::string newStatus, bool isConnected) {
    connectionStatus = std::move(newStatus);
    connected = isConnected;
}

void UI::setIdleSeconds(double seconds) {
    idleSeconds = seconds;
}

void UI::setLearnerMode(bool learner) {
    changeMode(learner ? Mode::Learner : Mode::Player);
}

// Switching mode clears everything the old mode was showing.
void UI::changeMode(Mode newMode) {
    mode = newMode;
    suggestionsVisible = false;
    suggestRequested = false;
    selectedSuggestion = -1;
    feedback.clear();
    highlightType = -1;
    highlightRootPc = -1;
    lastChangeTime = ImGui::GetCurrentContext() ? ImGui::GetTime() : 0.0;
}

bool UI::shouldClose() const {
    return window == nullptr || glfwWindowShouldClose(window);
}

std::unordered_set<int> UI::getMouseKeys() const {
    return mouseKeys;
}

// -------------------------------------------------------------- main frame

void UI::update(PianoKeyboard keyboard) {
    if (window == nullptr) {
        return;
    }

    // Do not draw while the window is minimized.
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
        glfwWaitEventsTimeout(0.1);
        return;
    }

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // One window fills the whole app. Everything else is a bordered box
    // inside it.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("ChordCompanion", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SeparatorText("CHORD COMPANION");

    // The keys to draw as pressed: the MIDI keyboard's keys, the computer
    // keyboard's keys and the key held with the mouse.
    updateComputerKeys();
    std::unordered_set<int> pressedKeys = keyboard.getPressedKeys();
    pressedKeys.insert(mouseKeys.begin(), mouseKeys.end());
    pressedKeys.insert(computerKeys.begin(), computerKeys.end());

    drawModeBar();

    updateAnalysis(pressedKeys);
    computeGlow(pressedKeys);

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float fullWidth = ImGui::GetContentRegionAvail().x;
    bool learning = (mode == Mode::Learner);

    // Row 1: connection status and the detected chord.
    float statusWidth = 300.0f;
    drawStatusBox(ImVec2(statusWidth, 126));
    ImGui::SameLine();
    drawChordBox(pressedKeys, ImVec2(fullWidth - statusWidth - spacing, 126));

    // Row 2: the chords played so far.
    drawHistoryRow();

    if (learning) {
        // Row 3: the piano and the chord helper.
        float pianoWidth = fullWidth * 0.60f;
        drawPianoBox(pressedKeys, ImVec2(pianoWidth, 296));
        ImGui::SameLine();
        drawChordHelperBox(pressedKeys, ImVec2(fullWidth - pianoWidth - spacing, 296));

        // Row 4: ideas for what comes next, and what an inversion is.
        float suggestionWidth = fullWidth * 0.55f;
        float rowHeight = ImGui::GetContentRegionAvail().y;
        drawSuggestionBox(ImVec2(suggestionWidth, rowHeight));
        ImGui::SameLine();
        drawInversionBox(ImVec2(fullWidth - suggestionWidth - spacing, rowHeight));
    } else {
        // Player mode: a big piano and nothing else, no colors, no ideas.
        drawPianoBox(pressedKeys, ImVec2(0, 0));
    }

    // The first time, ask what the player wants to do.
    drawModeChooser();

    ImGui::End();

    ImGui::Render();

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.11f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void UI::shutdown() {
    if (window == nullptr) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
}

// ---------------------------------------------------------------- analysis

void UI::updateComputerKeys() {
    // Z and X switch between two octaves, so every piano key can be reached.
    if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        octaveShift = 0;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        octaveShift = 12;
    }

    computerKeys.clear();
    for (const ComputerKey& k : COMPUTER_KEYS) {
        int note = k.note + octaveShift;
        if (ImGui::IsKeyDown(k.key) && PianoLayout::isInRange(note)) {
            computerKeys.insert(note);
        }
    }
}

// Four friendly chords to start with when nothing has been played yet.
static std::vector<CT::Suggestion> starterChords() {
    return {
        CT::suggestionFor(0, 0, "A friendly place to start"),
        CT::suggestionFor(0, 4, "Bright and open"),
        CT::suggestionFor(0, 5, "Soft and a little sad"),
        CT::suggestionFor(0, 3, "Warm, used in many songs"),
    };
}

void UI::buildSuggestions() {
    suggestions.clear();
    bridgeText.clear();
    if (!history.empty() && keyPc >= 0) {
        const CT::PlayedChord& last = history.back();
        suggestions = CT::suggestNext(keyPc, last.rootPc, last.typeIndex);
        bridgeText = CT::bridgeMessage(keyPc, last.rootPc, last.typeIndex);
    } else {
        suggestions = starterChords();
    }
    suggestionsVisible = true;
    selectedSuggestion = -1;
    feedback.clear();
}

// A friendly reply when the player plays a chord while ideas were showing.
// Playing something that is not on the list is always fine.
std::string UI::makeFeedback(int oldKeyPc, const CT::IdentifiedChord& played) const {
    std::string name = CT::chordName(played.rootPc, played.typeIndex);
    for (size_t i = 0; i < suggestions.size(); i++) {
        if (suggestions[i].rootPc == played.rootPc && suggestions[i].typeIndex == played.typeIndex) {
            return "Nice! " + name + " was idea " + std::to_string(i + 1) + ".";
        }
    }
    if (oldKeyPc >= 0 && CT::isDiatonic(oldKeyPc, played.rootPc, played.typeIndex)) {
        return name + " was not on the list, but it fits the key. Going your own way is great.";
    }
    if (oldKeyPc >= 0) {
        return name + " is outside the key. That adds color, and it is totally fine.";
    }
    return name + " is a fine choice. Play on!";
}

void UI::updateAnalysis(const std::unordered_set<int>& pressedKeys) {
    double now = ImGui::GetTime();

    // Something changed: a key went down or up.
    if (pressedKeys != lastPressed) {
        lastPressed = pressedKeys;
        lastChangeTime = now;
        if (!pressedKeys.empty()) {
            hasPlayed = true;
        }

        // A new full chord goes into the "You played" row. Single notes or
        // unfinished chords are fine too, they just do not count as a chord.
        CT::IdentifiedChord identified = CT::identifyChord(pressedKeys);
        bool isNewChord = identified.found &&
            (!currentChord.found ||
             identified.rootPc != currentChord.rootPc ||
             identified.typeIndex != currentChord.typeIndex);
        if (isNewChord) {
            int oldKeyPc = keyPc;
            if (suggestionsVisible) {
                feedback = makeFeedback(oldKeyPc, identified);
            } else {
                feedback.clear();
            }
            history.push_back(CT::PlayedChord{ identified.rootPc, identified.typeIndex });
            if (history.size() > 8) {
                history.erase(history.begin());
            }
            // The old ideas are done. New ones can appear after the next pause.
            suggestionsVisible = false;
            suggestRequested = false;
            selectedSuggestion = -1;
        }
        currentChord = identified;
        keyPc = CT::detectKey(history);
    }

    pausedFor = now - lastChangeTime;

    // Learner mode only. Ideas appear after a pause (if turned on) or when
    // the player asks. They stay until a new chord is played.
    if (mode == Mode::Learner && !suggestionsVisible) {
        bool byPause = autoSuggest && hasPlayed && pausedFor >= idleSeconds;
        if (byPause || suggestRequested) {
            buildSuggestions();
            suggestedByPause = byPause && !suggestRequested;
        }
    }
}

void UI::computeGlow(const std::unordered_set<int>& pressedKeys) {
    glowNotes.clear();
    glowCaption.clear();

    // Player mode has no colors at all.
    if (mode != Mode::Learner) {
        return;
    }

    int shown = selectedSuggestion;
    if (shown < 0 && suggestionsVisible && !suggestions.empty()) {
        shown = 0;  // by default show the first idea
    }

    if (suggestionsVisible && shown >= 0 && shown < static_cast<int>(suggestions.size())) {
        const CT::Suggestion& s = suggestions[shown];
        glowNotes = CT::chordNotes(s.rootPc, s.typeIndex, 0);
        glowCaption = "Glowing = " + CT::chordName(s.rootPc, s.typeIndex) + " (an idea, not a rule)";
    } else if (highlightType >= 0) {
        int root = 0;
        if (highlightRootPc >= 0) {
            root = highlightRootPc;
        } else if (!pressedKeys.empty()) {
            root = CT::pitchClass(CT::lowestNote(pressedKeys));
        } else if (!history.empty()) {
            root = history.back().rootPc;
        }
        glowNotes = CT::chordNotes(root, highlightType, highlightInversion);
        glowCaption = "Glowing = " + CT::chordName(root, highlightType) + ", " +
                      CT::inversionName(highlightInversion);
    }
}

// ------------------------------------------------------------------- mode

void UI::drawModeChooser() {
    if (mode != Mode::Unset) {
        return;
    }
    if (!ImGui::IsPopupOpen("Choose your mode")) {
        ImGui::OpenPopup("Choose your mode");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Choose your mode", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("How do you want to use ChordCompanion?");
        ImGui::Spacing();

        if (ImGui::Button("Learner\n\nColors, ideas and\ntips while I play", ImVec2(260, 110))) {
            changeMode(Mode::Learner);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Player\n\nJust the piano.\nNo colors, no ideas", ImVec2(260, 110))) {
            changeMode(Mode::Player);
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("You can switch any time from the bar at the top.");
        ImGui::EndPopup();
    }
}

void UI::drawModeBar() {
    ImGui::BeginChild("ModeBar", ImVec2(0, 44), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Mode:");
    ImGui::SameLine();

    // The chosen mode is drawn in blue.
    ImVec4 chosen(0.20f, 0.45f, 0.85f, 1.0f);
    ImVec4 normal = ImGui::GetStyleColorVec4(ImGuiCol_Button);

    ImGui::PushStyleColor(ImGuiCol_Button, mode == Mode::Learner ? chosen : normal);
    if (ImGui::Button("Learner") && mode != Mode::Learner) {
        changeMode(Mode::Learner);
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, mode == Mode::Player ? chosen : normal);
    if (ImGui::Button("Player") && mode != Mode::Player) {
        changeMode(Mode::Player);
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    if (mode == Mode::Learner) {
        ImGui::Checkbox("Ideas when I pause", &autoSuggest);
        ImGui::SameLine();
        ImGui::Checkbox("Color dots", &showDots);
        ImGui::SameLine();
    } else if (mode == Mode::Player) {
        ImGui::TextDisabled("Just play. No colors or ideas.");
        ImGui::SameLine();
    }

    if (ImGui::Button("Release all keys")) {
        mouseLatched.clear();
        mouseKeys.clear();
    }

    ImGui::EndChild();
}

// ------------------------------------------------------------------- boxes

void UI::drawStatusBox(ImVec2 size) {
    ImGui::BeginChild("MidiInputBox", size, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("MIDI INPUT");

    if (connected) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Connected");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "Not connected");
    }
    ImGui::TextWrapped("%s", connectionStatus.c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("No keyboard? Click the piano, or use A W S E D F ... Z = low octave, X = high octave (now: %s).",
                       octaveShift == 0 ? "low" : "high");
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

void UI::drawChordBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    ImGui::BeginChild("ChordBox", size, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("DETECTED CHORD");

    // The label comes from the app (ChordLogic). If it has none yet, the UI
    // names the chord itself so this box still teaches something.
    std::string label = chordLabel;
    if (label == "--" && currentChord.found) {
        label = CT::chordName(currentChord.rootPc, currentChord.typeIndex);
    }

    // The chord name is drawn bigger than normal text.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 position = ImGui::GetCursorScreenPos();
    float textSize = ImGui::GetFontSize() * 2.6f;
    drawList->AddText(ImGui::GetFont(), textSize, position, COLOR_CHORD_TEXT, label.c_str());
    ImGui::Dummy(ImVec2(0.0f, textSize + 4.0f));

    std::vector<int> lit = PianoLayout::keysToHighlight(pressedKeys);
    std::string line = "Pressed: " + PianoLayout::describeKeys(lit);
    if (currentChord.found) {
        line += "   (" + CT::inversionName(currentChord.inversion) + ")";
    }
    ImGui::TextUnformatted(line.c_str());

    ImGui::EndChild();
}

// A rounded label that is drawn with the draw list. style: 0 = plain text,
// 1 = outline only, 2 = filled.
static void drawChip(const std::string& text, ImU32 color, int style) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 position = ImGui::GetCursorScreenPos();
    float padding = (style == 0) ? 2.0f : 11.0f;
    ImVec2 size(textSize.x + padding * 2.0f, 26.0f);
    ImVec2 end = add(position, size.x, size.y);

    if (style == 2) {
        drawList->AddRectFilled(position, end, color, 13.0f);
    } else if (style == 1) {
        drawList->AddRect(position, end, color, 13.0f, 0, 1.6f);
    }
    drawList->AddText(add(position, padding, (size.y - textSize.y) * 0.5f),
                      style == 2 ? COLOR_DARK_TEXT : (style == 1 ? color : COLOR_TEXT),
                      text.c_str());
    ImGui::Dummy(size);
    ImGui::SameLine();
}

void UI::drawHistoryRow() {
    ImGui::BeginChild("HistoryRow", ImVec2(0, 46), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    drawChip("You played", COLOR_TEXT, 0);

    if (history.empty()) {
        drawChip("Play a chord to start", IM_COL32(150, 150, 160, 255), 0);
    }
    for (size_t i = 0; i < history.size(); i++) {
        const CT::PlayedChord& chord = history[i];
        bool isLast = (i + 1 == history.size());
        // Player mode keeps the chips plain: no colors.
        ImU32 chipColor = (mode == Mode::Learner) ? typeColor(chord.typeIndex)
                                                  : IM_COL32(150, 175, 210, 255);
        drawChip(CT::chordShortName(chord.rootPc, chord.typeIndex), chipColor, isLast ? 2 : 1);
    }

    if (mode == Mode::Learner && suggestionsVisible && suggestedByPause) {
        char text[96];
        std::snprintf(text, sizeof(text), "? paused %d seconds, ideas are below",
                      static_cast<int>(idleSeconds));
        drawChip(text, COLOR_PAUSE_CHIP, 1);
    }

    if (keyPc >= 0) {
        std::string text = (history.size() >= 3 ? "Key detected: " : "Key guess: ") +
                           CT::pitchClassName(keyPc) + " major";
        drawChip(text, COLOR_KEY_CHIP, 1);
    }

    ImGui::EndChild();
}

void UI::drawPianoBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    using namespace PianoLayout;

    ImGui::BeginChild("PianoBox", size, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SeparatorText("KEYBOARD (C4 to B5)");

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Make the piano as big as the box allows, without changing its shape.
    // The sizes in pianoLayout.hpp are for scale 1, so everything drawn is
    // multiplied by "scale" and mouse positions are divided by it.
    // 56 pixels are kept free under the piano for the legend.
    ImVec2 available = ImGui::GetContentRegionAvail();
    float pianoWidth = WHITE_KEY_COUNT * WHITE_KEY_WIDTH;
    float scale = std::min(available.x / pianoWidth, (available.y - 56.0f) / WHITE_KEY_HEIGHT);
    scale = std::max(scale, 0.5f);

    // Center the piano inside the box.
    float offsetX = std::max(0.0f, (available.x - pianoWidth * scale) * 0.5f);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 origin(cursor.x + offsetX, cursor.y + 4.0f);

    // Mouse: a normal click holds the key while the button is down. Shift +
    // click latches a key (and Shift + click again lets go), so a whole chord
    // can be played with the mouse. The key under the mouse is found with
    // noteAtPoint() from pianoLayout.hpp.
    mouseKeys = mouseLatched;
    if (ImGui::IsWindowHovered()) {
        ImVec2 mouse = ImGui::GetMousePos();
        int note = noteAtPoint((mouse.x - origin.x) / scale, (mouse.y - origin.y) / scale);
        if (note != -1) {
            if (ImGui::GetIO().KeyShift) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (mouseLatched.erase(note) == 0) {
                        mouseLatched.insert(note);
                    }
                    mouseKeys = mouseLatched;
                }
            } else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                mouseKeys.insert(note);
            }
        }
    }

    // A key is lit if the MIDI keyboard, the mouse or the computer keyboard
    // is holding it.
    std::vector<int> litNotes = keysToHighlight(pressedKeys);
    auto isLit = [&](int note) {
        return std::find(litNotes.begin(), litNotes.end(), note) != litNotes.end();
    };
    auto isGlowing = [&](int note) {
        return std::find(glowNotes.begin(), glowNotes.end(), note) != glowNotes.end();
    };

    // Colored dots: which keys could finish a chord. They are shown while a
    // chord is still being built (not once it is complete).
    std::map<int, std::vector<ImU32>> dots;
    if (mode == Mode::Learner && showDots && !pressedKeys.empty() && !currentChord.found) {
        int lowest = CT::lowestNote(pressedKeys);
        int lowestPc = CT::pitchClass(lowest);
        for (const CT::Candidate& c : CT::possibleChords(pressedKeys)) {
            if (!c.possible) {
                continue;
            }
            for (int pc : c.missingPcs) {
                int note = lowest + ((pc - lowestPc) % 12 + 12) % 12;
                if (isInRange(note)) {
                    dots[note].push_back(typeColor(c.typeIndex));
                }
            }
        }
    }

    float whiteWidth = (WHITE_KEY_WIDTH - 2.0f) * scale;
    float whiteHeight = WHITE_KEY_HEIGHT * scale;

    // White keys first, then black keys on top of them.
    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + whiteWidth, origin.y + whiteHeight);

        ImU32 fill = COLOR_WHITE_KEY;
        if (isLit(note)) {
            fill = COLOR_WHITE_KEY_PRESSED;
        } else if (isGlowing(note)) {
            fill = COLOR_WHITE_KEY_GLOW;
        }
        drawList->AddRectFilled(topLeft, bottomRight, fill, 3.0f);
        if (isGlowing(note) && !isLit(note)) {
            drawList->AddRect(topLeft, bottomRight, COLOR_GLOW_OUTLINE, 3.0f, 0, 3.0f);
        } else {
            drawList->AddRect(topLeft, bottomRight, COLOR_OUTLINE, 3.0f);
        }

        // Labels: the note name on the C keys (where each octave starts) and
        // the computer key that plays this note.
        if (note % 12 == 0) {
            std::string label = noteName(note);
            ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddText(ImVec2(x + (whiteWidth - labelSize.x) * 0.5f,
                                     origin.y + whiteHeight - 40.0f),
                              COLOR_KEY_LABEL, label.c_str());
        }
        const char* letter = computerKeyLetter(note - octaveShift);
        if (letter[0] != '\0') {
            ImVec2 letterSize = ImGui::CalcTextSize(letter);
            drawList->AddText(ImVec2(x + (whiteWidth - letterSize.x) * 0.5f,
                                     origin.y + whiteHeight - 22.0f),
                              COLOR_KEY_LABEL, letter);
        }
    }

    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (!isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + BLACK_KEY_WIDTH * scale, origin.y + BLACK_KEY_HEIGHT * scale);

        ImU32 fill = COLOR_BLACK_KEY;
        if (isLit(note)) {
            fill = COLOR_BLACK_KEY_PRESSED;
        } else if (isGlowing(note)) {
            fill = COLOR_BLACK_KEY_GLOW;
        }
        drawList->AddRectFilled(topLeft, bottomRight, fill, 2.0f);
        if (isGlowing(note) && !isLit(note)) {
            drawList->AddRect(topLeft, bottomRight, COLOR_GLOW_OUTLINE, 2.0f, 0, 3.0f);
        }

        const char* letter = computerKeyLetter(note - octaveShift);
        if (letter[0] != '\0') {
            ImVec2 letterSize = ImGui::CalcTextSize(letter);
            drawList->AddText(ImVec2(x + (BLACK_KEY_WIDTH * scale - letterSize.x) * 0.5f,
                                     origin.y + BLACK_KEY_HEIGHT * scale - 20.0f),
                              IM_COL32(200, 200, 200, 255), letter);
        }
    }

    // Draw the dots last so they sit on top of the keys.
    for (const auto& entry : dots) {
        int note = entry.first;
        const std::vector<ImU32>& colors = entry.second;
        float centerX = 0.0f;
        float centerY = 0.0f;
        if (isBlackKey(note)) {
            centerX = origin.x + keyX(note) * scale + BLACK_KEY_WIDTH * scale * 0.5f;
            centerY = origin.y + BLACK_KEY_HEIGHT * scale * 0.68f;
        } else {
            centerX = origin.x + keyX(note) * scale + whiteWidth * 0.5f;
            centerY = origin.y + whiteHeight * 0.62f;
        }
        float step = 12.0f;
        float first = -step * (static_cast<float>(colors.size()) - 1.0f) * 0.5f;
        for (size_t i = 0; i < colors.size(); i++) {
            ImVec2 center(centerX + first + step * static_cast<float>(i), centerY);
            drawList->AddCircleFilled(center, 5.5f, colors[i]);
            drawList->AddCircle(center, 5.5f, IM_COL32(0, 0, 0, 160), 0, 1.0f);
        }
    }

    float legendY = origin.y + whiteHeight + 10.0f;
    ImU32 grey = IM_COL32(150, 150, 160, 255);
    if (mode == Mode::Learner) {
        // Legend under the piano: what each dot color means.
        float legendX = origin.x;
        for (int type = 0; type < static_cast<int>(CT::chordTypes().size()); type++) {
            const std::string& name = CT::chordTypes()[type].name;
            drawList->AddCircleFilled(ImVec2(legendX + 6.0f, legendY + 8.0f), 5.5f, typeColor(type));
            drawList->AddText(ImVec2(legendX + 16.0f, legendY), COLOR_TEXT, name.c_str());
            legendX += 16.0f + ImGui::CalcTextSize(name.c_str()).x + 16.0f;
        }
        drawList->AddRectFilled(ImVec2(legendX, legendY + 3.0f), ImVec2(legendX + 12.0f, legendY + 14.0f),
                                COLOR_WHITE_KEY_GLOW, 2.0f);
        drawList->AddText(ImVec2(legendX + 18.0f, legendY), COLOR_TEXT, "Highlighted chord");

        if (!glowCaption.empty()) {
            drawList->AddText(ImVec2(origin.x, legendY + 22.0f), COLOR_GLOW_OUTLINE, glowCaption.c_str());
        } else {
            drawList->AddText(ImVec2(origin.x, legendY + 22.0f), grey,
                              "Dots show which keys could finish a chord.");
        }
    } else {
        drawList->AddText(ImVec2(origin.x, legendY), grey,
                          "Click to play a key. Shift + click holds it (for chords), Shift + click again lets go. Z / X change the octave.");
    }

    ImGui::EndChild();
}

void UI::drawChordHelperBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    ImGui::BeginChild("HelperBox", size, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("CHORD HELPER");

    // Dropdowns: choose a chord type and an inversion to see it on the piano.
    const char* typeItems[] = { "None", "Major", "Minor", "Diminished", "Augmented" };
    int typeChoice = highlightType + 1;
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::Combo("Highlight chord", &typeChoice, typeItems, 5)) {
        highlightType = typeChoice - 1;
        highlightRootPc = -1;
    }

    const char* inversionItems[] = { "Root position", "1st inversion", "2nd inversion" };
    ImGui::SetNextItemWidth(150.0f);
    ImGui::Combo("Inversion", &highlightInversion, inversionItems, 3);

    ImGui::Spacing();
    ImGui::SeparatorText("POSSIBLE CHORDS FROM THE PRESSED NOTE");

    if (pressedKeys.empty()) {
        ImGui::TextDisabled("Press a key to see what it could become.");
    } else if (currentChord.found) {
        ImGui::TextWrapped("Chord complete: %s",
                           CT::chordName(currentChord.rootPc, currentChord.typeIndex).c_str());
    } else {
        int rootPc = CT::pitchClass(CT::lowestNote(pressedKeys));
        std::set<int> pressedPcs;
        for (int note : pressedKeys) {
            pressedPcs.insert(CT::pitchClass(note));
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const CT::Candidate& c : CT::possibleChords(pressedKeys)) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            drawList->AddCircleFilled(ImVec2(p.x + 6.0f, p.y + ImGui::GetTextLineHeight() * 0.5f),
                                      5.5f, c.possible ? typeColor(c.typeIndex)
                                                       : IM_COL32(90, 90, 95, 255));
            ImGui::Dummy(ImVec2(16.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine();

            std::string name = CT::chordName(rootPc, c.typeIndex);
            if (c.possible) {
                std::string need;
                for (int pc : c.missingPcs) {
                    need += (need.empty() ? "" : " + ") + CT::pitchClassName(pc);
                }
                ImGui::Text("%s: add %s", name.c_str(), need.c_str());
            } else {
                // Which pressed note does not belong to this chord?
                std::vector<int> chordPcs = CT::chordPitchClasses(rootPc, c.typeIndex);
                std::string wrong;
                for (int pc : pressedPcs) {
                    if (std::find(chordPcs.begin(), chordPcs.end(), pc) == chordPcs.end()) {
                        wrong += (wrong.empty() ? "" : ", ") + CT::pitchClassName(pc);
                    }
                }
                ImGui::TextDisabled("%s: ruled out (no %s)", name.c_str(), wrong.c_str());
            }
        }
    }

    ImGui::EndChild();
}

void UI::drawSuggestionBox(ImVec2 size) {
    ImGui::BeginChild("SuggestionBox", size, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("IDEAS FOR WHAT COMES NEXT");

    ImVec4 amber(1.0f, 0.78f, 0.24f, 1.0f);

    // A friendly reply to the chord that was just played.
    if (!feedback.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.9f, 0.55f, 1.0f));
        ImGui::TextWrapped("%s", feedback.c_str());
        ImGui::PopStyleColor();
    }

    if (!suggestionsVisible) {
        ImGui::TextWrapped("Play whatever you like, nothing here is a rule. %s",
                           autoSuggest ? "Ideas appear if you pause for a few seconds, or press the button."
                                       : "Press the button when you want ideas.");
        if (ImGui::Button("Show ideas now")) {
            suggestRequested = true;
        }
    } else {
        if (history.empty()) {
            ImGui::TextColored(amber, hasPlayed ? "Those notes are not a full chord yet. Any three different notes work, or try:"
                                                : "Not sure how to start? Any chord is fine. Some friendly ones:");
        } else {
            ImGui::TextColored(amber, "Not sure what comes next? Some ideas (you can play anything):");
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < suggestions.size(); i++) {
            const CT::Suggestion& s = suggestions[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            drawList->AddCircleFilled(ImVec2(p.x + 7.0f, p.y + ImGui::GetTextLineHeight() * 0.5f + 2.0f),
                                      5.5f, typeColor(s.typeIndex));
            ImGui::Dummy(ImVec2(18.0f, ImGui::GetTextLineHeight() + 4.0f));
            ImGui::SameLine();

            std::string label = std::to_string(i + 1) + ".  " + CT::chordName(s.rootPc, s.typeIndex) +
                                ": " + s.reason;
            bool selected = (static_cast<int>(i) == selectedSuggestion) ||
                            (selectedSuggestion < 0 && i == 0);
            if (ImGui::Selectable(label.c_str(), selected)) {
                selectedSuggestion = static_cast<int>(i);
            }
        }

        if (!bridgeText.empty()) {
            ImGui::TextColored(amber, "Bridge:");
            ImGui::SameLine();
            ImGui::TextWrapped("%s", bridgeText.c_str());
        }
        ImGui::TextDisabled("Click an idea to see it on the piano. Playing something else is fine.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Hide ideas")) {
            suggestionsVisible = false;
            suggestRequested = false;
            selectedSuggestion = -1;
            lastChangeTime = ImGui::GetTime();  // wait a full pause before showing ideas again
        }
    }

    ImGui::EndChild();
}

void UI::drawInversionBox(ImVec2 size) {
    ImGui::BeginChild("InversionBox", size, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("WHAT IS AN INVERSION?");

    ImGui::TextWrapped("The same chord can have a different note at the bottom. "
                       "The notes stay the same, only the order changes.");
    ImGui::Spacing();

    // Teach with the chord the player just used, or C Major at the start.
    CT::PlayedChord base{ 0, CT::MAJOR };
    if (currentChord.found) {
        base = CT::PlayedChord{ currentChord.rootPc, currentChord.typeIndex };
    } else if (!history.empty()) {
        base = history.back();
    }
    ImGui::Text("Example: %s", CT::chordName(base.rootPc, base.typeIndex).c_str());

    for (int inversion = 0; inversion < 3; inversion++) {
        ImGui::PushID(inversion);

        std::vector<int> notes = CT::chordNotes(base.rootPc, base.typeIndex, inversion);
        bool isPlaying = currentChord.found && currentChord.inversion == inversion &&
                         currentChord.rootPc == base.rootPc &&
                         currentChord.typeIndex == base.typeIndex;

        std::string line = CT::inversionName(inversion) + ":  " + noteNames(notes);
        if (isPlaying) {
            ImGui::TextColored(ImVec4(0.47f, 0.75f, 1.0f, 1.0f), "%s   <- you are playing this",
                               line.c_str());
        } else {
            ImGui::TextUnformatted(line.c_str());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Show")) {
            highlightType = base.typeIndex;
            highlightRootPc = base.rootPc;
            highlightInversion = inversion;
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Tip: play the same notes in a different order and watch this box.");

    ImGui::EndChild();
}
