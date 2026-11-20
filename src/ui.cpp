#include "ui.hpp"
#include "pianoLayout.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <map>
#include <utility>

namespace CT = ChordTheory;

// On a Mac, taking over the whole monitor leaves a stretched, blurry picture
// when the player leaves fullscreen. So there the "Fullscreen" button just
// maximizes the window and restores it, which is reliable.
#if defined(__APPLE__) || defined(CHORD_USE_MAXIMIZE)
#define CHORD_MAXIMIZE_INSTEAD 1
#endif

// ------------------------------------------------------------------ palette
// One small set of colors used everywhere, so the whole window looks alike.

static const ImU32 COLOR_BG = IM_COL32(15, 17, 24, 255);
static const ImU32 COLOR_CARD = IM_COL32(24, 28, 38, 255);
static const ImU32 COLOR_CARD_BORDER = IM_COL32(46, 52, 70, 255);
static const ImU32 COLOR_TEXT = IM_COL32(228, 232, 242, 255);
static const ImU32 COLOR_MUTED = IM_COL32(132, 141, 164, 255);
static const ImU32 COLOR_ACCENT = IM_COL32(96, 165, 250, 255);
static const ImU32 COLOR_GOLD = IM_COL32(250, 204, 21, 255);
static const ImU32 COLOR_GREEN = IM_COL32(74, 222, 128, 255);
static const ImU32 COLOR_AMBER = IM_COL32(251, 191, 36, 255);
static const ImU32 COLOR_DARK_TEXT = IM_COL32(15, 18, 28, 255);

// Piano keys.
static const ImU32 KEY_WHITE = IM_COL32(248, 249, 252, 255);
static const ImU32 KEY_WHITE_SHADE = IM_COL32(220, 225, 234, 255);
static const ImU32 KEY_WHITE_PRESSED = IM_COL32(128, 188, 252, 255);
static const ImU32 KEY_WHITE_PRESSED_SHADE = IM_COL32(86, 150, 236, 255);
static const ImU32 KEY_WHITE_GLOW = IM_COL32(253, 238, 160, 255);
static const ImU32 KEY_WHITE_GLOW_SHADE = IM_COL32(246, 214, 96, 255);
static const ImU32 KEY_BLACK = IM_COL32(30, 32, 42, 255);
static const ImU32 KEY_BLACK_SHINE = IM_COL32(58, 62, 78, 255);
static const ImU32 KEY_BLACK_PRESSED = IM_COL32(59, 130, 246, 255);
static const ImU32 KEY_BLACK_GLOW = IM_COL32(222, 172, 22, 255);
static const ImU32 KEY_EDGE = IM_COL32(150, 157, 175, 255);
static const ImU32 KEY_LABEL = IM_COL32(112, 119, 138, 255);
static const ImU32 PIANO_CASE = IM_COL32(11, 13, 19, 255);

// One color per chord type, used for the dots, the chips and the legend.
static ImU32 typeColor(int typeIndex) {
    switch (typeIndex) {
        case CT::MAJOR: return IM_COL32(96, 165, 250, 255);       // blue
        case CT::MINOR: return IM_COL32(251, 146, 60, 255);       // orange
        case CT::DIMINISHED: return IM_COL32(192, 132, 252, 255); // purple
        case CT::AUGMENTED: return IM_COL32(74, 222, 128, 255);   // green
        default: return IM_COL32(200, 200, 200, 255);
    }
}

static ImVec4 vec4(ImU32 color) {
    return ImGui::ColorConvertU32ToFloat4(color);
}

static ImVec2 add(ImVec2 a, float x, float y) {
    return ImVec2(a.x + x, a.y + y);
}

// Text drawn at a given size (used for the big chord name and the title).
static float textWidth(float size, const char* text) {
    return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
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

// ------------------------------------------------------------ look and feel

// Use a clean system font if one is found. If not, ImGui's built-in font is
// used, so the program still works everywhere.
static void loadFont() {
    const char* candidates[] = {
        "/System/Library/Fonts/SFNS.ttf",                      // macOS
        "/System/Library/Fonts/Helvetica.ttc",                 // macOS
        "/Library/Fonts/Arial.ttf",                            // macOS
        "C:\\Windows\\Fonts\\segoeui.ttf",                     // Windows
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",     // Linux
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    };
    ImGuiIO& io = ImGui::GetIO();
    for (const char* path : candidates) {
        if (FILE* file = std::fopen(path, "rb")) {
            std::fclose(file);
            if (io.Fonts->AddFontFromFileTTF(path, 16.0f) != nullptr) {
                return;
            }
        }
    }
}

static void applyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(16.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 0.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 12.0f;
    style.GrabRounding = 8.0f;
    style.ChildBorderSize = 1.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = vec4(COLOR_BG);
    c[ImGuiCol_ChildBg] = vec4(COLOR_CARD);
    c[ImGuiCol_PopupBg] = vec4(IM_COL32(22, 25, 34, 255));
    c[ImGuiCol_Border] = vec4(COLOR_CARD_BORDER);
    c[ImGuiCol_Separator] = vec4(COLOR_CARD_BORDER);
    c[ImGuiCol_Text] = vec4(COLOR_TEXT);
    c[ImGuiCol_TextDisabled] = vec4(COLOR_MUTED);
    c[ImGuiCol_FrameBg] = vec4(IM_COL32(33, 38, 52, 255));
    c[ImGuiCol_FrameBgHovered] = vec4(IM_COL32(44, 51, 70, 255));
    c[ImGuiCol_FrameBgActive] = vec4(IM_COL32(54, 62, 86, 255));
    c[ImGuiCol_Button] = vec4(IM_COL32(40, 47, 64, 255));
    c[ImGuiCol_ButtonHovered] = vec4(IM_COL32(56, 65, 88, 255));
    c[ImGuiCol_ButtonActive] = vec4(IM_COL32(74, 86, 116, 255));
    c[ImGuiCol_CheckMark] = vec4(COLOR_ACCENT);
    c[ImGuiCol_Header] = ImVec4(0.38f, 0.65f, 0.98f, 0.20f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.65f, 0.98f, 0.32f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.65f, 0.98f, 0.45f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.62f);
}

// A card is a rounded box with a small grey title and a thin line under it.
// Every box in the window is a card, so they all look the same.
static void beginCard(const char* id, ImVec2 size, const char* title) {
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (title != nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
    }
}

// A smaller grey heading inside a card.
static void subTitle(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

// Same, but the text wraps to the next line if the card is narrow.
static void subTitleWrapped(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
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

    // Start at 1280 x 880, or a bit smaller if the screen is small.
    int startWidth = 1280;
    int startHeight = 920;
    int areaX = 0;
    int areaY = 0;
    int areaWidth = 0;
    int areaHeight = 0;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &areaX, &areaY, &areaWidth, &areaHeight);
    if (areaWidth > 0 && areaHeight > 0) {
        startWidth = std::min(startWidth, static_cast<int>(areaWidth * 0.92));
        startHeight = std::min(startHeight, static_cast<int>(areaHeight * 0.92));
    }

    window = glfwCreateWindow(startWidth, startHeight, "ChordCompanion", nullptr, nullptr);

    if (window == nullptr) {
        glfwTerminate();
        return false;
    }

    // The window can be resized, down to a narrow, phone-like shape.
    glfwSetWindowSizeLimits(window, 380, 520, GLFW_DONT_CARE, GLFW_DONT_CARE);

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
    io.IniFilename = nullptr;  // do not create an imgui.ini file

    loadFont();
    applyTheme();

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
    noticeVisible = false;
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

void UI::toggleFullscreen() {
    if (window == nullptr) {
        return;
    }
#ifdef CHORD_MAXIMIZE_INSTEAD
    if (!fullscreen) {
        glfwMaximizeWindow(window);
    } else {
        glfwRestoreWindow(window);
    }
    return;
#endif
    if (!fullscreen) {
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedW, &windowedH);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        if (monitor != nullptr && videoMode != nullptr) {
            glfwSetWindowMonitor(window, monitor, 0, 0, videoMode->width, videoMode->height,
                                 videoMode->refreshRate);
            fullscreen = true;
        }
    } else {
        glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedW, windowedH, 0);
        fullscreen = false;
    }
}

// The text shown in the big chord name: the app's label, or the UI's own.
std::string UI::displayLabel() const {
    if (chordLabel == "--" && currentChord.found) {
        return CT::chordName(currentChord.rootPc, currentChord.typeIndex);
    }
    return chordLabel;
}

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

#ifdef CHORD_MAXIMIZE_INSTEAD
    // The green window button can also maximize, so follow the real state.
    fullscreen = (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE);
#endif

    // Changing fullscreen is done here, between frames.
    if (wantFullscreenToggle) {
        toggleFullscreen();
        wantFullscreenToggle = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Shortcuts: B = big keyboard, F11 = fullscreen, Esc = go back.
    if (mode != Mode::Unset) {
        if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
            wantFullscreenToggle = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
            bigKeyboard = !bigKeyboard;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (bigKeyboard) {
                bigKeyboard = false;
            } else if (fullscreen) {
                wantFullscreenToggle = true;
            }
        }
    }

    // One window fills the whole app. Everything else is a card inside it.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    // A narrow window (like a phone held upright) gets one column of cards.
    narrow = viewport->WorkSize.x < 980.0f;

    // The window can scroll up and down if the cards do not all fit.
    ImGui::Begin("ChordCompanion", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    drawHeader();

    // The keys to draw as pressed: the MIDI keyboard's keys, the computer
    // keyboard's keys and the keys held with the mouse.
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

    if (bigKeyboard) {
        // Big keyboard: the chord name on a slim strip, the piano fills the rest.
        drawChordStrip(pressedKeys, ImVec2(0, 64));
        drawPianoBox(pressedKeys, ImVec2(0, 0));
    } else if (narrow) {
        // One column, from top to bottom. The window scrolls.
        drawStatusBox(ImVec2(0, 142));
        drawChordBox(pressedKeys, ImVec2(0, 142));
        drawHistoryRow();

        float pianoScale = std::max(0.4f, (fullWidth - 56.0f) / (PianoLayout::WHITE_KEY_COUNT * PianoLayout::WHITE_KEY_WIDTH));
        drawPianoBox(pressedKeys, ImVec2(0, PianoLayout::WHITE_KEY_HEIGHT * pianoScale + (learning ? 160.0f : 110.0f)));
        if (learning) {
            drawChordHelperBox(pressedKeys, ImVec2(0, 270));
            drawSuggestionBox(ImVec2(0, 260));
            drawInversionBox(ImVec2(0, 240));
        }
    } else {
        // Row 1: connection status and the detected chord.
        float statusWidth = 310.0f;
        drawStatusBox(ImVec2(statusWidth, 142));
        ImGui::SameLine();
        drawChordBox(pressedKeys, ImVec2(fullWidth - statusWidth - spacing, 142));

        // Row 2: the chords played so far.
        drawHistoryRow();

        if (learning) {
            // The piano gets the full width and most of the height. Three
            // cards sit side by side below it.
            float remaining = ImGui::GetContentRegionAvail().y;
            float gap = ImGui::GetStyle().ItemSpacing.y;
            float lastRow = 270.0f;
            float pianoRow = std::max(290.0f, std::min(remaining - lastRow - gap, 600.0f));
            drawPianoBox(pressedKeys, ImVec2(0, pianoRow));

            float rowHeight = std::max(lastRow, remaining - pianoRow - gap);
            float ideasWidth = fullWidth * 0.36f;
            float helperWidth = fullWidth * 0.33f;
            float inversionWidth = fullWidth - ideasWidth - helperWidth - spacing * 2.0f;
            drawSuggestionBox(ImVec2(ideasWidth, rowHeight));
            ImGui::SameLine();
            drawChordHelperBox(pressedKeys, ImVec2(helperWidth, rowHeight));
            ImGui::SameLine();
            drawInversionBox(ImVec2(inversionWidth, rowHeight));
        } else {
            // Player mode: a big piano and nothing else, no colors, no ideas.
            drawPianoBox(pressedKeys, ImVec2(0, 0));
        }
    }

    // The first time, ask what the player wants to do.
    drawModeChooser();

    ImGui::End();

    drawNotice();

    ImGui::Render();

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    ImVec4 background = vec4(COLOR_BG);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(background.x, background.y, background.z, 1.0f);
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

        // Teach from where the player is. If the last chord is in the key,
        // the ideas are in that key. If it is not, the ideas start fresh from
        // that chord: a major chord is treated as "home", a minor chord as
        // the sixth chord of its relative major.
        ideasKeyPc = keyPc;
        if (!CT::isDiatonic(keyPc, last.rootPc, last.typeIndex)) {
            if (last.typeIndex == CT::MINOR) {
                ideasKeyPc = CT::pitchClass(last.rootPc + 3);
            } else if (last.typeIndex == CT::DIMINISHED) {
                ideasKeyPc = CT::pitchClass(last.rootPc + 1);
            } else {
                ideasKeyPc = last.rootPc;
            }
            bridgeText = CT::bridgeMessage(keyPc, last.rootPc, last.typeIndex);
        }
        suggestions = CT::suggestNext(ideasKeyPc, last.rootPc, last.typeIndex);
    } else {
        ideasKeyPc = 0;
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
            return "Nice! " + name + " was idea " + std::to_string(i + 1) + ". You are getting it!";
        }
    }
    if (oldKeyPc >= 0 && CT::isDiatonic(oldKeyPc, played.rootPc, played.typeIndex)) {
        return "Oh, " + name + "! Not on the list, but it fits the key. Love the detour.";
    }
    if (oldKeyPc >= 0) {
        return "Ooh, " + name + " steps outside the key. Bold! That adds color, keep going.";
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

            // Was this chord outside the key the player has been playing in?
            // (Only after a few chords, so the key is not just a guess.)
            bool outside = (mode == Mode::Learner) && oldKeyPc >= 0 && history.size() >= 3 &&
                           !CT::isDiatonic(oldKeyPc, identified.rootPc, identified.typeIndex);

            history.push_back(CT::PlayedChord{ identified.rootPc, identified.typeIndex });
            if (history.size() > 8) {
                history.erase(history.begin());
            }

            // The old ideas and highlights are done as soon as the player
            // plays something. New ones can appear after the next pause.
            suggestionsVisible = false;
            suggestRequested = false;
            selectedSuggestion = -1;

            if (outside) {
                noticeVisible = true;
                noticeChord = CT::PlayedChord{ identified.rootPc, identified.typeIndex };
                noticeKeyPc = oldKeyPc;
                noticeTime = now;
            } else if (noticeVisible && oldKeyPc >= 0 &&
                       CT::isDiatonic(oldKeyPc, identified.rootPc, identified.typeIndex)) {
                noticeVisible = false;  // back in the key, no need for the note
            }
        }
        if (identified.found && mode == Mode::Learner) {
            checkPractice(identified);
        }
        currentChord = identified;
        keyPc = CT::detectKey(history);
    }

    if (!practiceMsg.empty() && now - practiceTime > 10.0) {
        practiceMsg.clear();
    }

    pausedFor = now - lastChangeTime;

    // The note about a chord outside the key fades by itself.
    if (noticeVisible && now - noticeTime > 14.0) {
        noticeVisible = false;
    }

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

// The player chose a chord to practice (Show button). Answer like a friend:
// cheer when it is right, give a small tip when it is close, and never stop
// them from playing anything else.
void UI::checkPractice(const CT::IdentifiedChord& played) {
    if (highlightType < 0 || highlightRootPc < 0) {
        return;
    }
    std::string target = CT::chordName(highlightRootPc, highlightType);
    std::string playedName = CT::chordName(played.rootPc, played.typeIndex);
    practiceTime = ImGui::GetTime();

    if (played.rootPc == highlightRootPc && played.typeIndex == highlightType) {
        if (played.inversion == highlightInversion) {
            practiceMsg = "Yes! " + target + ", " + CT::inversionName(highlightInversion) +
                          ". Nailed it! Pick another one to keep going.";
            practiceMood = 1;
            highlightType = -1;
            highlightRootPc = -1;
        } else {
            std::vector<int> want = CT::chordNotes(highlightRootPc, highlightType, highlightInversion);
            practiceMsg = "So close! That is " + target + ", but in " +
                          CT::inversionName(played.inversion) + ". For " +
                          CT::inversionName(highlightInversion) + " put " +
                          CT::pitchClassName(CT::pitchClass(want.front())) + " at the bottom.";
            practiceMood = 2;
        }
    } else {
        // They moved on to something else. That is fine: let the glow go and
        // let the box follow what they play again.
        practiceMsg = "Oh, you went with " + playedName + " instead of " + target +
                      ". No problem! Press Show again whenever you want to try it.";
        practiceMood = 2;
        highlightType = -1;
        highlightRootPc = -1;
        exampleRootPc = -1;
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

    if (highlightType < 0 && suggestionsVisible && shown >= 0 && shown < static_cast<int>(suggestions.size())) {
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
                      CT::inversionName(highlightInversion) + " (your pick)";
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
    if (ImGui::BeginPopupModal("Choose your mode", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("How do you want to use ChordCompanion?");
        ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
        ImGui::TextUnformatted("You can switch any time from the bar at the top.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::Button("Learner\n\nColors, ideas and\ntips while I play", ImVec2(250, 110))) {
            changeMode(Mode::Learner);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Player\n\nJust the piano.\nNo colors, no ideas", ImVec2(250, 110))) {
            changeMode(Mode::Player);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void UI::drawHeader() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 position = ImGui::GetCursorScreenPos();
    float titleSize = 28.0f;

    float chordWidth = textWidth(titleSize, "Chord");
    drawList->AddText(ImGui::GetFont(), titleSize, position, COLOR_TEXT, "Chord");
    drawList->AddText(ImGui::GetFont(), titleSize, add(position, chordWidth, 0.0f), COLOR_ACCENT,
                      "Companion");

    // Short line on the right so the title bar does not look empty.
    if (!narrow) {
        const char* tagline = "Play, learn and explore chords";
        float taglineWidth = textWidth(ImGui::GetFontSize(), tagline);
        float right = position.x + ImGui::GetContentRegionAvail().x;
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                          ImVec2(right - taglineWidth, position.y + 10.0f), COLOR_MUTED, tagline);
    }

    ImGui::Dummy(ImVec2(0.0f, titleSize + 2.0f));
}

void UI::drawModeBar() {
    // A narrow window gets three short lines instead of one long one.
    beginCard("ModeBar", ImVec2(0, narrow ? 138.0f : 48.0f), nullptr);

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
    ImGui::TextUnformatted("MODE");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // A button that is drawn in blue while it is "on".
    auto toggleButton = [&](const char* label, bool on) {
        if (on) {
            ImGui::PushStyleColor(ImGuiCol_Button, vec4(COLOR_ACCENT));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, vec4(COLOR_ACCENT));
            ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_DARK_TEXT));
        }
        bool clicked = ImGui::Button(label);
        if (on) {
            ImGui::PopStyleColor(3);
        }
        return clicked;
    };

    if (toggleButton("Learner", mode == Mode::Learner) && mode != Mode::Learner) {
        changeMode(Mode::Learner);
    }
    ImGui::SameLine();
    if (toggleButton("Player", mode == Mode::Player) && mode != Mode::Player) {
        changeMode(Mode::Player);
    }
    if (!narrow) {
        ImGui::SameLine();
    }

    // Note names on the keys: useful in both modes.
    ImGui::Checkbox("Note names", &showNoteNames);
    ImGui::SameLine();

    if (mode == Mode::Learner) {
        ImGui::Checkbox("Ideas when I pause", &autoSuggest);
        ImGui::SameLine();
        ImGui::Checkbox("Color dots", &showDots);
    } else if (mode == Mode::Player) {
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
        ImGui::TextUnformatted("Just play. No colors or ideas.");
        ImGui::PopStyleColor();
    }

    // Buttons for the size of the keyboard. On a wide window they sit at the
    // right edge, on a narrow one they get their own line.
    const char* bigLabel = bigKeyboard ? "Exit big keyboard" : "Big keyboard";
#ifdef CHORD_MAXIMIZE_INSTEAD
    const char* fullLabel = fullscreen ? "Restore window" : "Maximize window";
#else
    const char* fullLabel = fullscreen ? "Exit fullscreen" : "Fullscreen";
#endif
    const char* releaseLabel = "Release all keys";
    if (!narrow) {
        float padding = ImGui::GetStyle().FramePadding.x * 2.0f;
        float gap = ImGui::GetStyle().ItemSpacing.x;
        float total = ImGui::CalcTextSize(bigLabel).x + ImGui::CalcTextSize(fullLabel).x +
                      ImGui::CalcTextSize(releaseLabel).x + padding * 3.0f + gap * 2.0f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - total);
    }

    if (toggleButton(bigLabel, bigKeyboard)) {
        bigKeyboard = !bigKeyboard;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Make the piano fill the window (shortcut: B, Esc to go back)");
    }
    ImGui::SameLine();
    if (toggleButton(fullLabel, fullscreen)) {
        wantFullscreenToggle = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Make the window fill the screen (shortcut: F11)");
    }
    ImGui::SameLine();
    if (ImGui::Button(releaseLabel)) {
        mouseLatched.clear();
        mouseKeys.clear();
    }

    ImGui::EndChild();
}

// A slim strip used in big keyboard mode: the chord name and the pressed notes.
void UI::drawChordStrip(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    beginCard("ChordStrip", size, nullptr);

    std::string label = displayLabel();
    bool hasChord = (label != "--");
    ImU32 labelColor = COLOR_MUTED;
    if (hasChord) {
        labelColor = (mode == Mode::Learner && currentChord.found) ? typeColor(currentChord.typeIndex)
                                                                    : COLOR_ACCENT;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 position = ImGui::GetCursorScreenPos();
    float bigSize = 30.0f;
    drawList->AddText(ImGui::GetFont(), bigSize, position, labelColor, label.c_str());
    ImGui::Dummy(ImVec2(textWidth(bigSize, label.c_str()) + 18.0f, bigSize));
    ImGui::SameLine();

    std::vector<int> lit = PianoLayout::keysToHighlight(pressedKeys);
    std::string notes = lit.empty() ? "Press keys to start" : PianoLayout::describeKeys(lit);
    if (currentChord.found) {
        notes += "   (" + CT::inversionName(currentChord.inversion) + ")";
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (bigSize - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
    ImGui::TextUnformatted(notes.c_str());
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

// A small box at the side. It never blocks the playing: the player can ignore
// it, and it fades away by itself.
void UI::drawNotice() {
    if (!noticeVisible || mode != Mode::Learner) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float margin = 20.0f;
    if (narrow) {
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - margin,
                                       viewport->WorkPos.y + viewport->WorkSize.y - margin),
                                ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    } else {
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - margin,
                                       viewport->WorkPos.y + 116.0f),
                                ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    }

    float wrapWidth = std::min(380.0f, viewport->WorkSize.x - 80.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, vec4(IM_COL32(32, 28, 20, 245)));
    ImGui::PushStyleColor(ImGuiCol_Border, vec4(COLOR_AMBER));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("OutsideKeyNotice", nullptr, flags)) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);

        ImGui::TextColored(vec4(COLOR_AMBER), "Outside the key");
        ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
        ImGui::TextWrapped("%s is not in %s major. That is fine, keep playing.",
                           CT::chordName(noticeChord.rootPc, noticeChord.typeIndex).c_str(),
                           CT::pitchClassName(noticeKeyPc).c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::Button("Keep playing")) {
            noticeVisible = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Start fresh here")) {
            // Forget the old chords so the key is worked out from this chord on.
            history.clear();
            history.push_back(noticeChord);
            keyPc = CT::detectKey(history);
            noticeVisible = false;
        }

        ImGui::PopTextWrapPos();
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

// ------------------------------------------------------------------- cards

void UI::drawStatusBox(ImVec2 size) {
    beginCard("MidiInputBox", size, "MIDI INPUT");

    // A colored dot and a word: green = connected, amber = not connected.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float line = ImGui::GetTextLineHeight();
    drawList->AddCircleFilled(ImVec2(p.x + 6.0f, p.y + line * 0.5f), 5.0f,
                              connected ? COLOR_GREEN : COLOR_AMBER);
    ImGui::Dummy(ImVec2(18.0f, line));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, vec4(connected ? COLOR_GREEN : COLOR_AMBER));
    ImGui::TextUnformatted(connected ? "Connected" : "Not connected");
    ImGui::PopStyleColor();

    ImGui::TextWrapped("%s", connectionStatus.c_str());

    ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
    ImGui::TextWrapped("No MIDI? Click the piano or press A W S E D F. Z / X = octave (%s).",
                       octaveShift == 0 ? "low" : "high");
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

void UI::drawChordBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    beginCard("ChordBox", size, "DETECTED CHORD");

    // The label comes from the app (ChordLogic). If it has none yet, the UI
    // names the chord itself so this box still teaches something.
    std::string label = chordLabel;
    if (label == "--" && currentChord.found) {
        label = CT::chordName(currentChord.rootPc, currentChord.typeIndex);
    }
    bool hasChord = (label != "--");

    // The chord name is drawn much bigger than normal text. In Learner mode
    // it takes the color of the chord type (blue major, orange minor, ...).
    ImU32 labelColor = COLOR_MUTED;
    if (hasChord) {
        labelColor = (mode == Mode::Learner && currentChord.found) ? typeColor(currentChord.typeIndex)
                                                                    : COLOR_ACCENT;
    }
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 position = ImGui::GetCursorScreenPos();
    float bigSize = 44.0f;
    drawList->AddText(ImGui::GetFont(), bigSize, position, labelColor, label.c_str());
    ImGui::Dummy(ImVec2(0.0f, bigSize + 2.0f));

    // Under the name: the notes that are pressed, and the inversion.
    std::vector<int> lit = PianoLayout::keysToHighlight(pressedKeys);
    if (lit.empty()) {
        subTitle("Press keys to start");
    } else {
        ImGui::AlignTextToFramePadding();
        subTitle(PianoLayout::describeKeys(lit).c_str());
        if (currentChord.found) {
            ImGui::SameLine();
            ImGui::TextUnformatted(" ");
            ImGui::SameLine();
            ImDrawList* list = ImGui::GetWindowDrawList();
            std::string text = CT::inversionName(currentChord.inversion);
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            ImVec2 chipPosition = ImGui::GetCursorScreenPos();
            ImVec2 chipSize(textSize.x + 20.0f, 24.0f);
            list->AddRect(chipPosition, add(chipPosition, chipSize.x, chipSize.y), COLOR_MUTED, 12.0f, 0, 1.2f);
            list->AddText(add(chipPosition, 10.0f, (chipSize.y - textSize.y) * 0.5f), COLOR_MUTED, text.c_str());
            ImGui::Dummy(chipSize);
        }
    }

    ImGui::EndChild();
}

// A rounded label that is drawn with the draw list. style: 0 = plain text,
// 1 = outline only, 2 = filled.
static void drawChip(const std::string& text, ImU32 color, int style) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 position = ImGui::GetCursorScreenPos();
    float padding = (style == 0) ? 2.0f : 12.0f;
    ImVec2 size(textSize.x + padding * 2.0f, 28.0f);
    ImVec2 end = add(position, size.x, size.y);

    if (style == 2) {
        drawList->AddRectFilled(position, end, color, 14.0f);
    } else if (style == 1) {
        drawList->AddRect(position, end, color, 14.0f, 0, 1.5f);
    }
    drawList->AddText(add(position, padding, (size.y - textSize.y) * 0.5f),
                      style == 2 ? COLOR_DARK_TEXT : (style == 1 ? color : COLOR_MUTED),
                      text.c_str());
    ImGui::Dummy(size);
    ImGui::SameLine();
}

void UI::drawHistoryRow() {
    beginCard("HistoryRow", ImVec2(0, 52), nullptr);

    drawChip("YOU PLAYED", COLOR_MUTED, 0);

    if (history.empty()) {
        drawChip("Play a chord to start", COLOR_MUTED, 0);
    }
    // A narrow window only has room for the last few chords.
    size_t firstShown = (narrow && history.size() > 4) ? history.size() - 4 : 0;
    for (size_t i = firstShown; i < history.size(); i++) {
        const CT::PlayedChord& chord = history[i];
        bool isLast = (i + 1 == history.size());
        // Player mode keeps the chips plain: no colors.
        ImU32 chipColor = (mode == Mode::Learner) ? typeColor(chord.typeIndex)
                                                  : IM_COL32(150, 175, 210, 255);
        drawChip(CT::chordShortName(chord.rootPc, chord.typeIndex), chipColor, isLast ? 2 : 1);
    }

    if (mode == Mode::Learner && suggestionsVisible && suggestedByPause) {
        char text[96];
        std::snprintf(text, sizeof(text), "paused %d seconds, ideas are below",
                      static_cast<int>(idleSeconds));
        drawChip(text, COLOR_AMBER, 1);
    }

    if (keyPc >= 0) {
        std::string text = (history.size() >= 3 ? "Key: " : "Key guess: ") +
                           CT::pitchClassName(keyPc) + " major";
        drawChip(text, COLOR_GREEN, 1);
    }

    ImGui::EndChild();
}

void UI::drawPianoBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    using namespace PianoLayout;

    beginCard("PianoBox", size, "KEYBOARD   C4 to B5");

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // The legend (what each dot color means) sits on the title line when
    // there is room, so the keys can be taller. If not, it goes under the piano.
    ImVec2 windowPos = ImGui::GetWindowPos();
    float windowWidth = ImGui::GetWindowWidth();
    float legendWidth = 0.0f;
    for (const CT::ChordType& type : CT::chordTypes()) {
        legendWidth += 18.0f + ImGui::CalcTextSize(type.name.c_str()).x + 22.0f;
    }
    legendWidth += 20.0f + ImGui::CalcTextSize("Highlighted chord").x;
    bool legendInHeader = (mode == Mode::Learner) && (windowWidth > legendWidth + 620.0f);

    // Make the piano as big as the box allows. The sizes in pianoLayout.hpp
    // are for scale 1. "scale" is used sideways and "scaleY" up and down, so
    // everything drawn is multiplied by them and mouse positions are divided.
    // Some room is kept under the piano for the legend and a caption.
    ImVec2 available = ImGui::GetContentRegionAvail();
    float pianoWidth = WHITE_KEY_COUNT * WHITE_KEY_WIDTH;
    float reserved = legendInHeader ? 16.0f : 64.0f;
    float scale = std::max(0.3f, (available.x - 24.0f) / pianoWidth);
    float scaleY = std::max(0.3f, (available.y - reserved) / WHITE_KEY_HEIGHT);
    // Keep the keys a pleasant shape: never more than 1.7 times wider, or
    // 1.8 times taller, than the normal proportions.
    scale = std::min(scale, scaleY * 1.7f);
    scaleY = std::min(scaleY, scale * 1.8f);

    // Legend on the title line, at the right edge of the card.
    if (legendInHeader) {
        float legendX = windowPos.x + windowWidth - legendWidth - ImGui::GetStyle().WindowPadding.x;
        float legendTop = windowPos.y + ImGui::GetStyle().WindowPadding.y;
        for (int type = 0; type < static_cast<int>(CT::chordTypes().size()); type++) {
            const std::string& name = CT::chordTypes()[type].name;
            drawList->AddCircleFilled(ImVec2(legendX + 6.0f, legendTop + 8.0f), 6.0f, typeColor(type));
            drawList->AddText(ImVec2(legendX + 18.0f, legendTop - 1.0f), COLOR_TEXT, name.c_str());
            legendX += 18.0f + ImGui::CalcTextSize(name.c_str()).x + 22.0f;
        }
        drawList->AddRectFilled(ImVec2(legendX, legendTop + 2.0f), ImVec2(legendX + 13.0f, legendTop + 15.0f),
                                KEY_WHITE_GLOW, 3.0f);
        drawList->AddRect(ImVec2(legendX, legendTop + 2.0f), ImVec2(legendX + 13.0f, legendTop + 15.0f),
                          COLOR_GOLD, 3.0f, 0, 1.5f);
        drawList->AddText(ImVec2(legendX + 20.0f, legendTop - 1.0f), COLOR_TEXT, "Highlighted chord");
    }

    // Center the piano inside the box.
    float offsetX = std::max(12.0f, (available.x - pianoWidth * scale) * 0.5f);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 origin(cursor.x + offsetX, cursor.y + 8.0f);

    float whiteWidth = (WHITE_KEY_WIDTH - 2.0f) * scale;
    float whiteHeight = WHITE_KEY_HEIGHT * scaleY;

    // Mouse: a normal click holds the key while the button is down. Shift +
    // click latches a key (and Shift + click again lets go), so a whole chord
    // can be played with the mouse. The key under the mouse is found with
    // noteAtPoint() from pianoLayout.hpp.
    mouseKeys = mouseLatched;
    if (ImGui::IsWindowHovered()) {
        ImVec2 mouse = ImGui::GetMousePos();
        int note = noteAtPoint((mouse.x - origin.x) / scale, (mouse.y - origin.y) / scaleY);
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

    // The dark case behind the keys.
    drawList->AddRectFilled(add(origin, -10.0f, -8.0f),
                            add(origin, pianoWidth * scale + 8.0f, whiteHeight + 8.0f),
                            PIANO_CASE, 10.0f);

    float labelSize = ImGui::GetFontSize() * 0.82f;
    ImFont* font = ImGui::GetFont();

    // White keys first, then black keys on top of them.
    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + whiteWidth, origin.y + whiteHeight);

        ImU32 fill = KEY_WHITE;
        ImU32 shade = KEY_WHITE_SHADE;
        if (isLit(note)) {
            fill = KEY_WHITE_PRESSED;
            shade = KEY_WHITE_PRESSED_SHADE;
        } else if (isGlowing(note)) {
            fill = KEY_WHITE_GLOW;
            shade = KEY_WHITE_GLOW_SHADE;
        }
        drawList->AddRectFilled(topLeft, bottomRight, fill, 5.0f, ImDrawFlags_RoundCornersBottom);
        // A darker band along the bottom gives the key some depth.
        drawList->AddRectFilled(ImVec2(x, bottomRight.y - 9.0f * scale), bottomRight, shade, 5.0f,
                                ImDrawFlags_RoundCornersBottom);
        if (isGlowing(note) && !isLit(note)) {
            drawList->AddRect(topLeft, bottomRight, COLOR_GOLD, 5.0f, ImDrawFlags_RoundCornersBottom, 2.5f);
        } else {
            drawList->AddRect(topLeft, bottomRight, KEY_EDGE, 5.0f, ImDrawFlags_RoundCornersBottom, 1.0f);
        }

        // Labels: the note name on the C keys (where each octave starts) and
        // the computer key that plays this note.
        float labelY = bottomRight.y - 12.0f * scale - labelSize;
        const char* letter = computerKeyLetter(note - octaveShift);
        if (letter[0] != '\0') {
            float w = textWidth(labelSize, letter);
            drawList->AddText(font, labelSize, ImVec2(x + (whiteWidth - w) * 0.5f, labelY),
                              KEY_LABEL, letter);
            labelY -= labelSize + 4.0f;
        }
        // Note name: every key when "Note names" is on, otherwise only the C keys.
        if (showNoteNames || note % 12 == 0) {
            std::string name = noteName(note);
            float w = textWidth(labelSize, name.c_str());
            drawList->AddText(font, labelSize, ImVec2(x + (whiteWidth - w) * 0.5f, labelY),
                              note % 12 == 0 ? COLOR_ACCENT : COLOR_MUTED, name.c_str());
        }
    }

    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (!isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        float width = BLACK_KEY_WIDTH * scale;
        float height = BLACK_KEY_HEIGHT * scaleY;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + width, origin.y + height);

        ImU32 fill = KEY_BLACK;
        if (isLit(note)) {
            fill = KEY_BLACK_PRESSED;
        } else if (isGlowing(note)) {
            fill = KEY_BLACK_GLOW;
        }
        drawList->AddRectFilled(topLeft, bottomRight, fill, 4.0f, ImDrawFlags_RoundCornersBottom);
        // A lighter strip at the bottom, like the shine on a real black key.
        if (!isLit(note) && !isGlowing(note)) {
            drawList->AddRectFilled(ImVec2(x + 3.0f, bottomRight.y - 10.0f * scale),
                                    ImVec2(bottomRight.x - 3.0f, bottomRight.y - 3.0f),
                                    KEY_BLACK_SHINE, 3.0f);
        }
        if (isGlowing(note) && !isLit(note)) {
            drawList->AddRect(topLeft, bottomRight, COLOR_GOLD, 4.0f, ImDrawFlags_RoundCornersBottom, 2.5f);
        }

        float blackLabelY = bottomRight.y - 10.0f * scale - labelSize * 0.5f - 2.0f;
        const char* letter = computerKeyLetter(note - octaveShift);
        if (letter[0] != '\0') {
            float w = textWidth(labelSize, letter);
            drawList->AddText(font, labelSize, ImVec2(x + (width - w) * 0.5f, blackLabelY),
                              IM_COL32(190, 196, 212, 255), letter);
        }
        blackLabelY -= labelSize + 3.0f;  // note name always sits on the same line
        if (showNoteNames) {
            std::string name = noteName(note);  // e.g. C#4, shown as C#
            while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back()))) {
                name.pop_back();
            }
            float w = textWidth(labelSize, name.c_str());
            drawList->AddText(font, labelSize, ImVec2(x + (width - w) * 0.5f, blackLabelY),
                              IM_COL32(150, 160, 185, 255), name.c_str());
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
            centerY = origin.y + BLACK_KEY_HEIGHT * scaleY * 0.40f;
        } else {
            centerX = origin.x + keyX(note) * scale + whiteWidth * 0.5f;
            centerY = origin.y + whiteHeight * 0.72f;  // below the black keys
        }
        // Two dots must fit side by side on one key, so the size follows the key width.
        float keyWidth = (isBlackKey(note) ? BLACK_KEY_WIDTH : WHITE_KEY_WIDTH - 2.0f) * scale;
        float step = std::min(17.0f, keyWidth * 0.44f);
        float radius = std::max(3.5f, std::min(7.5f, step * 0.46f));
        float first = -step * (static_cast<float>(colors.size()) - 1.0f) * 0.5f;
        for (size_t i = 0; i < colors.size(); i++) {
            ImVec2 center(centerX + first + step * static_cast<float>(i), centerY);
            drawList->AddCircleFilled(center, radius, colors[i]);
            drawList->AddCircle(center, radius, IM_COL32(255, 255, 255, 230), 0, 1.6f);
        }
    }

    // Legend under the piano: one clean row (it goes on to a second row if
    // the window is narrow).
    float legendY = origin.y + whiteHeight + 16.0f;
    float rightLimit = cursor.x + available.x;
    if (mode == Mode::Learner) {
        float captionY = legendY - 2.0f;
        if (legendInHeader) {
            // The caption goes on the title line, between the title and the legend.
            float captionTop = windowPos.y + ImGui::GetStyle().WindowPadding.y - 1.0f;
            float captionX = windowPos.x + 230.0f;
            if (!glowCaption.empty()) {
                drawList->AddText(ImVec2(captionX, captionTop), COLOR_GOLD, glowCaption.c_str());
            } else {
                drawList->AddText(ImVec2(captionX, captionTop), COLOR_MUTED,
                                  "Dots show which keys could finish a chord.");
            }
        } else {
            float legendX = origin.x;
            auto startItem = [&](float itemWidth) {
                if (legendX + itemWidth > rightLimit && legendX > origin.x) {
                    legendX = origin.x;
                    legendY += 22.0f;
                }
            };
            for (int type = 0; type < static_cast<int>(CT::chordTypes().size()); type++) {
                const std::string& name = CT::chordTypes()[type].name;
                float itemWidth = 18.0f + ImGui::CalcTextSize(name.c_str()).x;
                startItem(itemWidth);
                drawList->AddCircleFilled(ImVec2(legendX + 6.0f, legendY + 8.0f), 6.0f, typeColor(type));
                drawList->AddText(ImVec2(legendX + 18.0f, legendY - 1.0f), COLOR_TEXT, name.c_str());
                legendX += itemWidth + 22.0f;
            }
            float itemWidth = 20.0f + ImGui::CalcTextSize("Highlighted chord").x;
            startItem(itemWidth);
            drawList->AddRectFilled(ImVec2(legendX, legendY + 2.0f), ImVec2(legendX + 13.0f, legendY + 15.0f),
                                    KEY_WHITE_GLOW, 3.0f);
            drawList->AddRect(ImVec2(legendX, legendY + 2.0f), ImVec2(legendX + 13.0f, legendY + 15.0f),
                              COLOR_GOLD, 3.0f, 0, 1.5f);
            drawList->AddText(ImVec2(legendX + 20.0f, legendY - 1.0f), COLOR_TEXT, "Highlighted chord");
            captionY = legendY + 24.0f;
        }

        if (!legendInHeader) {
            if (!glowCaption.empty()) {
                drawList->AddText(ImVec2(origin.x, captionY), COLOR_GOLD, glowCaption.c_str());
            } else if (!narrow) {
                drawList->AddText(ImVec2(origin.x, captionY), COLOR_MUTED,
                                  "Dots show which keys could finish a chord.");
            }
        }
    } else {
        drawList->AddText(ImVec2(origin.x, legendY - 1.0f), COLOR_MUTED,
                          narrow ? "Click to play. Shift + click holds a key."
                                 : "Click to play a key. Shift + click holds it (for chords), Shift + click again lets go. Z / X change the octave.");
    }

    ImGui::EndChild();
}

void UI::drawChordHelperBox(const std::unordered_set<int>& pressedKeys, ImVec2 size) {
    beginCard("HelperBox", size, "CHORD HELPER");

    // Dropdowns: choose a chord type and an inversion to see it on the piano.
    const char* typeItems[] = { "None", "Major", "Minor", "Diminished", "Augmented" };
    int typeChoice = highlightType + 1;
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("Highlight chord", &typeChoice, typeItems, 5)) {
        highlightType = typeChoice - 1;
        highlightRootPc = -1;
        practiceMsg.clear();
    }

    const char* inversionItems[] = { "Root position", "1st inversion", "2nd inversion" };
    ImGui::SetNextItemWidth(160.0f);
    ImGui::Combo("Inversion", &highlightInversion, inversionItems, 3);

    ImGui::Spacing();
    subTitle("POSSIBLE CHORDS FROM THE PRESSED NOTE");
    ImGui::Separator();
    ImGui::Spacing();

    if (pressedKeys.empty()) {
        subTitle("Press a key to see what it could become.");
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
            drawList->AddCircleFilled(ImVec2(p.x + 7.0f, p.y + ImGui::GetTextLineHeight() * 0.5f),
                                      6.0f, c.possible ? typeColor(c.typeIndex)
                                                       : IM_COL32(70, 75, 90, 255));
            ImGui::Dummy(ImVec2(20.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine();

            std::string name = CT::chordName(rootPc, c.typeIndex);
            if (c.possible) {
                std::string need;
                for (int pc : c.missingPcs) {
                    need += (need.empty() ? "" : " + ") + CT::pitchClassName(pc);
                }
                ImGui::TextWrapped("%s: add %s", name.c_str(), need.c_str());
            } else {
                // Which pressed note does not belong to this chord?
                std::vector<int> chordPcs = CT::chordPitchClasses(rootPc, c.typeIndex);
                std::string wrong;
                for (int pc : pressedPcs) {
                    if (std::find(chordPcs.begin(), chordPcs.end(), pc) == chordPcs.end()) {
                        wrong += (wrong.empty() ? "" : ", ") + CT::pitchClassName(pc);
                    }
                }
                ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_MUTED));
                ImGui::TextWrapped("%s: ruled out (no %s)", name.c_str(), wrong.c_str());
                ImGui::PopStyleColor();
            }
        }
    }

    ImGui::EndChild();
}

void UI::drawSuggestionBox(ImVec2 size) {
    beginCard("SuggestionBox", size, "IDEAS FOR WHAT COMES NEXT");

    ImVec4 amber = vec4(COLOR_AMBER);

    // A friendly reply to the chord that was just played.
    if (!feedback.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, vec4(COLOR_GREEN));
        ImGui::TextWrapped("%s", feedback.c_str());
        ImGui::PopStyleColor();
    }

    if (!suggestionsVisible) {
        subTitleWrapped(autoSuggest ? "Play whatever you like, nothing here is a rule. Ideas appear if you pause."
                                    : "Play whatever you like, nothing here is a rule.");
        if (ImGui::Button("Show ideas now")) {
            suggestRequested = true;
        }
    } else {
        if (history.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, amber);
            ImGui::TextWrapped("%s", hasPlayed ? "Those notes are not a full chord yet. Any three different notes work, or try:"
                                               : "Not sure how to start? Any chord is fine. Some friendly ones:");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, amber);
            ImGui::TextWrapped("Some ideas in %s major (you can play anything):",
                               CT::pitchClassName(ideasKeyPc).c_str());
            ImGui::PopStyleColor();
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < suggestions.size(); i++) {
            const CT::Suggestion& s = suggestions[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            drawList->AddCircleFilled(ImVec2(p.x + 8.0f, p.y + ImGui::GetTextLineHeight() * 0.5f + 2.0f),
                                      6.0f, typeColor(s.typeIndex));
            ImGui::Dummy(ImVec2(22.0f, ImGui::GetTextLineHeight() + 4.0f));
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
        subTitle("Click an idea to see it on the piano.");
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
    beginCard("InversionBox", size, "WHAT IS AN INVERSION?");

    if (!practiceMsg.empty()) {
        // A friendly reply replaces the intro text while it is showing.
        ImVec4 col = practiceMood == 1 ? vec4(COLOR_GREEN)
                   : practiceMood == 2 ? vec4(COLOR_AMBER) : vec4(COLOR_ACCENT);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextWrapped("%s", practiceMsg.c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::TextWrapped("The same chord can have a different note at the bottom. "
                           "The notes stay the same, only the order changes.");
    }
    ImGui::Spacing();

    // Pick any chord to see its inversions. "Auto" follows what is being
    // played (or C Major at the start).
    const char* rootItems[] = { "Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const char* typeItems[] = { "Major", "Minor", "Diminished", "Augmented" };
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(vec4(COLOR_ACCENT), "Example chord:");
    ImGui::SameLine();
    int rootChoice = exampleRootPc + 1;
    ImGui::SetNextItemWidth(84.0f);
    if (ImGui::Combo("##exampleRoot", &rootChoice, rootItems, 13)) {
        exampleRootPc = rootChoice - 1;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(exampleRootPc < 0);
    ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("##exampleType", &exampleType, typeItems, 4);
    ImGui::EndDisabled();

    CT::PlayedChord base{ 0, CT::MAJOR };
    if (exampleRootPc >= 0) {
        base = CT::PlayedChord{ exampleRootPc, exampleType };
    } else if (currentChord.found) {
        base = CT::PlayedChord{ currentChord.rootPc, currentChord.typeIndex };
    } else if (!history.empty()) {
        base = history.back();
    }
    ImGui::TextColored(vec4(COLOR_ACCENT), "%s", CT::chordName(base.rootPc, base.typeIndex).c_str());
    if (highlightType >= 0) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear glow")) {
            highlightType = -1;
            highlightRootPc = -1;
            practiceMsg.clear();
        }
    }

    for (int inversion = 0; inversion < 3; inversion++) {
        ImGui::PushID(inversion);

        std::vector<int> notes = CT::chordNotes(base.rootPc, base.typeIndex, inversion);
        bool isPlaying = currentChord.found && currentChord.inversion == inversion &&
                         currentChord.rootPc == base.rootPc &&
                         currentChord.typeIndex == base.typeIndex;

        std::string name = CT::inversionName(inversion);
        name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        std::string line = name + ":  " + noteNames(notes);
        if (isPlaying) {
            ImGui::TextColored(vec4(COLOR_ACCENT), "%s  <- playing", line.c_str());
        } else {
            ImGui::TextUnformatted(line.c_str());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Show")) {
            highlightType = base.typeIndex;
            highlightRootPc = base.rootPc;
            highlightInversion = inversion;
            // Keep this chord in the box, even after other chords are played.
            exampleRootPc = base.rootPc;
            exampleType = base.typeIndex;
            practiceMsg = "Go ahead and play " + CT::chordName(base.rootPc, base.typeIndex) + ", " +
                          CT::inversionName(inversion) + ". Take your time!";
            practiceMood = 0;
            practiceTime = ImGui::GetTime();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}