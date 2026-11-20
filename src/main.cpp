#include "mainApp.hpp"

#include <cstdio>
#include <exception>

int main() {
    try {
        MainApp app;
        app.run();
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
