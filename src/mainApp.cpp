#include "mainApp.hpp"

#include <stdexcept>

MainApp::~MainApp() {
    ui.shutdown();
}

void MainApp::setup() {
    ui.setup();
    if (ui.shouldClose()) {
        throw std::runtime_error("Failed to initialize UI");
    }
}

void MainApp::update() {
    auto uiKeys = ui.update(keyboard.getPressedKeys());
    keyboard.setUiPressedKeys(uiKeys);
}

void MainApp::run() {
    setup();
    while (!ui.shouldClose()) {
        update();
    }
}
