#include <thread>

#include "hid.hxx"
#include "ui/ui.hxx"

int main(const int argc, const char **argv) {
    if (UI::InitializeBackend()) {
        UI::Setup();
        UI::Loop();
    }

    //std::terminate();

    return 0;
}