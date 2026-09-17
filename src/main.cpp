#include <cstdio>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "RtMidi.h"

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

    window =
        glfwCreateWindow(1280, 800, "My App", nullptr, nullptr);

    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return true;
}

void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    // Some messages contain less than 3 parts, and if they do, we can ignore them because they are not a keypress event
    if (!message || message->size() < 3) return;
    unsigned int nBytes = message->size();
    // The message has 3 parts:
    // The first byte appears to be either 144, signaling key pressed, or 128, signaling key release
    // The second byte is the midi key number
    // The third byte is the "velocity" v with witch the key was pressed, with v > 0 signaling a key press, and v = 0 signaling key release
    // We will be using the second and third bytes and ignoring the first
    std::string press_state = "";
    if ((int)message->at(2) != 0) {
        press_state = " Pressed";
    } else {
        press_state = " Released";
    }
    std::cout << "Midi Key: " << (int)message->at(1) << press_state << std::endl;
}

int main() {
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

    // rtmidi
    RtMidiIn *midiin = nullptr;
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
        error.printMessage();
        exit( EXIT_FAILURE );
    }
    
    // Check available ports.
    unsigned int nPorts = midiin->getPortCount();
    std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
    if (nPorts == 0) {
        std::cerr << "No MIDI ports available" << std::endl;
    }
    
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            std::cout << "  Input Port #" << i + 1 << ": " << midiin->getPortName(i) << '\n';;
        }
        catch ( RtMidiError &error ) {
            error.printMessage();
            return 1;
        }
    }

    unsigned int port = 0;

    if (nPorts > 1) {
        while (true) {
            std::cout << "Select MIDI Device [1-" << (nPorts) << "]: ";
            std::cin >> port;

            if (port >= 1 && port <= nPorts) {
                port -= 1;
                break;
            }
            
            std::cout << "Please select a valid device" << std::endl;
            std::cin.clear();
            std::cin.ignore(100000, '\n');
        }
    }

    midiin->openPort(port);
        
    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback(&midi_callback);
    
    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( false, false, false );

    while (running && !glfwWindowShouldClose(window)) {
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    delete midiin;

    return 0;
}