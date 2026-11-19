#include "ui.hpp"
#include "pianoLayout.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

// Colors used by the interface.
static const ImU32 COLOR_WHITE_KEY = IM_COL32(255, 255, 255, 255);
static const ImU32 COLOR_WHITE_KEY_PRESSED = IM_COL32(120, 190, 255, 255);
static const ImU32 COLOR_BLACK_KEY = IM_COL32(20, 20, 20, 255);
static const ImU32 COLOR_BLACK_KEY_PRESSED = IM_COL32(80, 140, 220, 255);
static const ImU32 COLOR_OUTLINE = IM_COL32(0, 0, 0, 255);
static const ImU32 COLOR_KEY_LABEL = IM_COL32(90, 90, 90, 255);
static const ImU32 COLOR_CHORD_TEXT = IM_COL32(120, 190, 255, 255);

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

bool UI::shouldClose() const {
    return window == nullptr || glfwWindowShouldClose(window);
}

std::unordered_set<int> UI::getMouseKeys() const {
    return mouseKeys;
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
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::SeparatorText("CHORD COMPANION");

    // The keys to draw as pressed: the MIDI keyboard's keys plus the key
    // held with the mouse.
    std::unordered_set<int> pressedKeys = keyboard.getPressedKeys();
    pressedKeys.insert(mouseKeys.begin(), mouseKeys.end());

    // Top row: connection status and the detected chord.
    drawStatusBox();
    ImGui::SameLine();
    drawChordBox(pressedKeys);

    ImGui::Spacing();
    ImGui::Spacing();

    // Bottom: the piano.
    drawPianoBox(pressedKeys);

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

void UI::drawStatusBox() {
    ImGui::BeginChild("MidiInputBox", ImVec2(360, 150), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("MIDI INPUT");

    if (connected) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Connected");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "Not connected");
    }
    ImGui::TextWrapped("%s", connectionStatus.c_str());

    ImGui::EndChild();
}

void UI::drawChordBox(const std::unordered_set<int>& pressedKeys) {
    // Fill the rest of the row next to the status box.
    float chordBoxWidth = ImGui::GetContentRegionAvail().x;
    ImGui::BeginChild("ChordBox", ImVec2(chordBoxWidth, 150), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("DETECTED CHORD");

    // The chord name is drawn bigger than normal text.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 position = ImGui::GetCursorScreenPos();
    float size = ImGui::GetFontSize() * 3.0f;
    drawList->AddText(ImGui::GetFont(), size, position, COLOR_CHORD_TEXT,
                      chordLabel.c_str());
    ImGui::Dummy(ImVec2(0.0f, size + 6.0f));

    std::vector<int> lit = PianoLayout::keysToHighlight(pressedKeys);
    ImGui::Text("Pressed: %s", PianoLayout::describeKeys(lit).c_str());

    ImGui::EndChild();
}

void UI::drawPianoBox(const std::unordered_set<int>& pressedKeys) {
    using namespace PianoLayout;

    // Size 0, 0 means: use all the space that is left in the window.
    ImGui::BeginChild("PianoBox", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("KEYBOARD (C4 to B5)");

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Make the piano as big as the box allows, without changing its shape.
    // The sizes in pianoLayout.hpp are for scale 1, so everything drawn is
    // multiplied by "scale" and mouse positions are divided by it.
    ImVec2 available = ImGui::GetContentRegionAvail();
    float pianoWidth = WHITE_KEY_COUNT * WHITE_KEY_WIDTH;
    float scale = std::min(available.x / pianoWidth, (available.y - 12.0f) / WHITE_KEY_HEIGHT);
    scale = std::max(scale, 0.5f);

    // Center the piano inside the box.
    float offsetX = std::max(0.0f, (available.x - pianoWidth * scale) * 0.5f);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 origin(cursor.x + offsetX, cursor.y + 6.0f);

    // Mouse: one key can be held at a time. The key under the mouse is
    // found with noteAtPoint() from pianoLayout.hpp.
    mouseKeys.clear();
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 mouse = ImGui::GetMousePos();
        int note = noteAtPoint((mouse.x - origin.x) / scale, (mouse.y - origin.y) / scale);
        if (note != -1) {
            mouseKeys.insert(note);
        }
    }

    // A key is lit if the MIDI keyboard or the mouse is holding it.
    std::vector<int> litNotes = keysToHighlight(pressedKeys);
    auto isLit = [&](int note) {
        return std::find(litNotes.begin(), litNotes.end(), note) != litNotes.end();
    };

    // White keys first, then black keys on top of them.
    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + (WHITE_KEY_WIDTH - 2.0f) * scale, origin.y + WHITE_KEY_HEIGHT * scale);

        drawList->AddRectFilled(topLeft, bottomRight,
            isLit(note) ? COLOR_WHITE_KEY_PRESSED : COLOR_WHITE_KEY, 3.0f);
        drawList->AddRect(topLeft, bottomRight, COLOR_OUTLINE, 3.0f);

        // Label the C keys so it is easy to see where each octave starts.
        if (note % 12 == 0) {
            std::string label = noteName(note);
            drawList->AddText(ImVec2(x + 8.0f, origin.y + WHITE_KEY_HEIGHT * scale - 24.0f),
                              COLOR_KEY_LABEL, label.c_str());
        }
    }

    for (int note = FIRST_NOTE; note <= LAST_NOTE; note++) {
        if (!isBlackKey(note)) {
            continue;
        }
        float x = origin.x + keyX(note) * scale;
        ImVec2 topLeft(x, origin.y);
        ImVec2 bottomRight(x + BLACK_KEY_WIDTH * scale, origin.y + BLACK_KEY_HEIGHT * scale);

        drawList->AddRectFilled(topLeft, bottomRight,
            isLit(note) ? COLOR_BLACK_KEY_PRESSED : COLOR_BLACK_KEY, 2.0f);
    }

    ImGui::EndChild();
}