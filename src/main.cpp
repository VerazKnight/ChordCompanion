#include <cstdio>
#include <atomic>
#include <string>
#include <unordered_set>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "RtMidi.h"

// Tracks keys pressed by mouse click. Only touched on the main thread,
// so a plain bool array is safe.
bool mouseKeyPressed[24] = { false };

// Tracks keys pressed by a real MIDI keyboard. Written to from
// midi_callback() on rtmidi's own background thread, and read from
// draw_piano() on the main thread. std::atomic<bool> makes that safe.
std::atomic<bool> midiKeyPressed[24];

// Options for the chord-type dropdown.
const char* chordTypes[] = { "None", "Major", "Minor", "Diminished", "Augmented" };
static int selectedChordIndex = 0;

// Info about the connected MIDI device, shown in the "MIDI INPUT" box.
std::string midiPortName = "No device connected";
unsigned int midiPortCount = 0;

static void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool setup_glfw(const char* &glslVersion, GLFWwindow* &window) {
    glfwSetErrorCallback(GlfwErrorCallback);

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
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return true;
}

void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    if (!message || message->size() < 3) return;

    int note = (int)message->at(1);
    int velocity = (int)message->at(2);

    if (note >= 60 && note <= 83) {
        midiKeyPressed[note - 60] = (velocity != 0);
    }
}

void draw_piano(std::unordered_set<int> keys) {
    const float whiteKeyWidth = 40.0f;
    const float whiteKeyHeight = 160.0f;
    const float blackKeyWidth = 26.0f;
    const float blackKeyHeight = 100.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    bool hasBlackKeyAfter[7] = { true, true, false, true, true, true, false };
    int whiteKeyNotes[14] = { 60, 62, 64, 65, 67, 69, 71,
                               72, 74, 76, 77, 79, 81, 83 };
    int blackKeyNotes[10] = { 61, 63, 66, 68, 70,
                               73, 75, 78, 80, 82 };

    for (int i = 0; i < 14; i++) {
        float x = origin.x + i * whiteKeyWidth;
        float y = origin.y;
        ImVec2 topLeft(x, y);
        ImVec2 bottomRight(x + whiteKeyWidth - 2, y + whiteKeyHeight);

        int note = whiteKeyNotes[i];
        int idx = note - 60;

        bool isPressed = mouseKeyPressed[idx] || midiKeyPressed[idx].load() || (keys.find(note) != keys.end());

        ImU32 fillColor = isPressed
            ? IM_COL32(120, 190, 255, 255)
            : IM_COL32(255, 255, 255, 255);

        drawList->AddRectFilled(topLeft, bottomRight, fillColor, 3.0f);
        drawList->AddRect(topLeft, bottomRight, IM_COL32(0, 0, 0, 255), 3.0f);

        ImGui::SetCursorScreenPos(topLeft);
        ImGui::PushID(note);
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("whiteKey", ImVec2(whiteKeyWidth - 2, whiteKeyHeight));
        mouseKeyPressed[idx] = ImGui::IsItemActive();
        ImGui::PopID();
    }

    int blackIndex = 0;
    for (int i = 0; i < 14; i++) {
        int withinOctave = i % 7;
        if (hasBlackKeyAfter[withinOctave] && blackIndex < 10) {
            float whiteX = origin.x + i * whiteKeyWidth;
            float x = whiteX + whiteKeyWidth - (blackKeyWidth / 2.0f);
            float y = origin.y;
            ImVec2 topLeft(x, y);
            ImVec2 bottomRight(x + blackKeyWidth, y + blackKeyHeight);

            int note = blackKeyNotes[blackIndex];
            int idx = note - 60;

            bool isPressed = mouseKeyPressed[idx] || midiKeyPressed[idx].load();

            ImU32 fillColor = isPressed
                ? IM_COL32(80, 140, 220, 255)
                : IM_COL32(20, 20, 20, 255);

            drawList->AddRectFilled(topLeft, bottomRight, fillColor, 2.0f);

            ImGui::SetCursorScreenPos(topLeft);
            ImGui::PushID(note);
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("blackKey", ImVec2(blackKeyWidth, blackKeyHeight));
            mouseKeyPressed[idx] = ImGui::IsItemActive();
            ImGui::PopID();

            blackIndex++;
        }
    }

    ImGui::SetCursorScreenPos(origin);
ImGui::Dummy(ImVec2(whiteKeyWidth * 14, whiteKeyHeight + 10));
}

int main() {
    std::unordered_set<int> keys{};

    keys.insert(60);

    const char* glslVersion = nullptr;
    GLFWwindow* window = nullptr;
    if (!setup_glfw(glslVersion, window)) {
        std::fprintf(stderr, "GLFW failed to setup");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    bool running = true;

    RtMidiIn *midiin = nullptr;
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
        error.printMessage();
        exit( EXIT_FAILURE );
    }

    midiPortCount = midiin->getPortCount();

    unsigned int port = 0;
    if (midiPortCount > 0) {
        if (midiPortCount > 1) {
            while (true) {
                std::cout << "Select MIDI Device [1-" << (midiPortCount) << "]: ";
                std::cin >> port;
                if (port >= 1 && port <= midiPortCount) {
                    port -= 1;
                    break;
                }
                std::cout << "Please select a valid device" << std::endl;
                std::cin.clear();
                std::cin.ignore(100000, '\n');
            }
        }
        midiin->openPort(port);
        midiPortName = midiin->getPortName(port);
        midiin->setCallback(&midi_callback);
        midiin->ignoreTypes( false, false, false );
    }

    while (running && !glfwWindowShouldClose(window)) {
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---- ONE window fills the whole app; everything else lives
        // inside it as separate bordered "module boxes" (BeginChild),
        // the same way a synth panel groups its controls into labeled
        // sections instead of scattering separate free-floating windows.

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::Begin("ChordCompanion", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImGui::SeparatorText("CHORD COMPANION");

        // --- Top row: two module boxes side by side ---

        ImGui::BeginChild("MidiInputBox", ImVec2(320, 130), ImGuiChildFlags_Borders);
        ImGui::SeparatorText("MIDI INPUT");
        if (midiPortCount > 0) {
            ImGui::TextWrapped("Connected: %s", midiPortName.c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "No MIDI device found");
        }
        ImGui::Text("Ports available: %u", midiPortCount);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ChordBox", ImVec2(320, 130), ImGuiChildFlags_Borders);
        ImGui::SeparatorText("CHORD SELECT");
        ImGui::Text("Highlight chord:");
        ImGui::Combo("##chordType", &selectedChordIndex, chordTypes, IM_ARRAYSIZE(chordTypes));
        ImGui::Spacing();
        ImGui::Text("Detected chord: --");
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Spacing();

        // --- Bottom: one wide box holding the piano ---

        ImGui::BeginChild("PianoBox", ImVec2(0, 250), ImGuiChildFlags_Borders);
        ImGui::SeparatorText("KEYBOARD (C4 - B5)");
        draw_piano(keys);
        ImGui::EndChild();

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    delete midiin;
    

    return 0;
}