#pragma once

#include "imgui/imgui.h"

#define WINDOW_INITIAL_WIDTH 1280
#define WINDOW_INITIAL_HEIGHT 800

namespace UI {
    static ImGuiContext* context;

    bool InitializeBackend();
    void TeardownBackend();

    void Setup();
    void Loop();
    void Render();

};