#include <cstdint>
#include <fstream>
#include <iostream>
#include <chrono>

#include "fmt/core.h"
//#include "stb_image.h"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"
#include "ImGuiFileBrowser.h"

#include "emulator.h"


namespace chip8
{
    const int SCALE = 10;

    struct App {
        CHIP8EmulatorState emulator;

        // Backend
        SDL_Window* window;
        SDL_GLContext gl_context;
    };

    bool create_app(App& app);

    bool load_rom(App& app, const char* filename);

    void run(App& app);

    void destroy_app(App& app);
} // namespace chip8_emulator
