#pragma once

#include "imgui/imgui.h"

#define WINDOW_INITIAL_WIDTH 1600
#define WINDOW_INITIAL_HEIGHT 900

namespace UI {
    static ImGuiContext* context;

    /**
     * These methods are platform-specific and are implemented
     * in the file `ui_$PLATFORM.cxx`
     */
    bool InitializeBackend();
    void TeardownBackend();

    void Setup();
    void Loop();
    void Render();

};