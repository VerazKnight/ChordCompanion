#include "pianoKeyboard.hpp"
#include "chordLogic.hpp"
#include "pianoKeyboard.hpp"

struct GLFWwindow;

class MainApp {
public:
    MainApp() = default;
    ~MainApp();
    MainApp(const MainApp&) = delete;
    MainApp& operator=(const MainApp&) = delete;

    bool setup(); // window, ImGui, MIDI keyboard. false if the window can't be created
    void update(); // one frame: reads keys -> identify chord -> draw
    bool shouldClose() const; // true once the window has closed

private:
    bool setupWindow();
    void setupImGui();
    void shutdown();
    void drawChordPanel(const ChordAnalysis& analysis, const std::unordered_set<int>& pressedKeys);
    void drawPianoKeyboard(const std::unordered_set<int>& pressedKeys);

    GLFWwindow* window = nullptr;
    const char* glslVersion = nullptr;
    bool initilized = false;

    ChordLogic chordLogic;
    PianoKeyboard keyboard;
};