#include "app.h"

namespace chip8
{

    static_assert(std::is_same_v<std::uint8_t, char> ||
                  std::is_same_v<std::uint8_t, unsigned char>,
                  "This app requires std::uint8_t to be implemented as char or unsigned char.");


    void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        fmt::println("Error: {}", message);
        //flush;
    }

    bool create_app(App& app)
    {
        static_assert(std::is_same_v<std::uint8_t, char> ||
                      std::is_same_v<std::uint8_t, unsigned char>,
                    "This app requires std::uint8_t to be implemented as char or unsigned char.");

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            fmt::print("Error: %s\n", SDL_GetError());
            return false;
        }
        
        const char* glsl_version = "#version 330";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        app.window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        app.gl_context = SDL_GL_CreateContext(app.window);
        SDL_GL_MakeCurrent(app.window, app.gl_context);
        SDL_GL_SetSwapInterval(0); // Enable vsync

        gladLoadGLLoader(SDL_GL_GetProcAddress);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(app.window, app.gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);

        app.emulator = create_chip8emulator();
        return true;
    }

    void destroy_app(App& app) 
    {
        destroy_chip8emulator(app.emulator);

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(app.gl_context);
        SDL_DestroyWindow(app.window);
        SDL_Quit();
    }

    bool load_rom(App& app, const char* filename)
    {
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);

        bool ok;
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            char buffer[size];
            if (file.read(buffer, size))
            {
                //TODO: How should I cast array? 
                //      Is using memcpy the intended way? 
                //      Should I reinterpret the pointer?

                if (size < C8_MEMORY_SIZE) {
                    uint8_t* rom = reinterpret_cast<uint8_t*>(buffer);
                    load_rom_from_buffer(app.emulator, rom, size);
                    ok = true;

                } else {
                    fmt::println("ROM size is bigger than memory");
                    ok = false;
                }
            }
        } else {
            ok = false;
        }

        return ok;
    } 

    void run(App& app) {
        
        bool done = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        bool running = false;
        bool step = false;

        const ImGuiTableFlags tables_flags = ImGuiTableFlags_BordersOuterH | 
                                             ImGuiTableFlags_BordersOuterV | 
                                             ImGuiTableFlags_BordersInnerV | 
                                             ImGuiTableFlags_BordersOuter | 
                                             ImGuiTableFlags_RowBg | 
                                             ImGuiTableRowFlags_Headers;

        MemoryEditor im_mem_edit; // Hex Editor
        MemoryEditor im_display_edit; // Hex Editor

        imgui_addons::ImGuiFileBrowser file_dialog; // File Dialog

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);

        // Create the texture for the display
        unsigned int tex_display;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex_display);
        glTextureParameteri(tex_display, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTextureParameteri(tex_display, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(tex_display, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(tex_display, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureStorage2D(tex_display, 1, GL_R8, C8_DISPLAY_WIDTH, C8_DISPLAY_HEIGHT);

        auto cycle_delay = 1.f/300.f;
        auto time_last_cycle = std::chrono::high_resolution_clock::now();

        while (!done)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    done = true;
                if (event.type == SDL_WINDOWEVENT 
                    && event.window.event == SDL_WINDOWEVENT_CLOSE 
                    && event.window.windowID == SDL_GetWindowID(app.window))
                    done = true;
            }

            // Inputs
            {
                app.emulator.keypad[0x0] = keystate[SDL_GetScancodeFromKey(SDLK_x)];
                app.emulator.keypad[0x1] = keystate[SDL_GetScancodeFromKey(SDLK_1)];
                app.emulator.keypad[0x2] = keystate[SDL_GetScancodeFromKey(SDLK_2)];
                app.emulator.keypad[0x3] = keystate[SDL_GetScancodeFromKey(SDLK_3)];
                app.emulator.keypad[0x4] = keystate[SDL_GetScancodeFromKey(SDLK_q)];
                app.emulator.keypad[0x5] = keystate[SDL_GetScancodeFromKey(SDLK_w)];
                app.emulator.keypad[0x6] = keystate[SDL_GetScancodeFromKey(SDLK_e)];
                app.emulator.keypad[0x7] = keystate[SDL_GetScancodeFromKey(SDLK_a)];
                app.emulator.keypad[0x8] = keystate[SDL_GetScancodeFromKey(SDLK_s)];
                app.emulator.keypad[0x9] = keystate[SDL_GetScancodeFromKey(SDLK_d)];
                app.emulator.keypad[0xA] = keystate[SDL_GetScancodeFromKey(SDLK_z)];
                app.emulator.keypad[0xB] = keystate[SDL_GetScancodeFromKey(SDLK_c)];
                app.emulator.keypad[0xC] = keystate[SDL_GetScancodeFromKey(SDLK_4)];
                app.emulator.keypad[0xD] = keystate[SDL_GetScancodeFromKey(SDLK_r)];
                app.emulator.keypad[0xE] = keystate[SDL_GetScancodeFromKey(SDLK_f)];
                app.emulator.keypad[0xF] = keystate[SDL_GetScancodeFromKey(SDLK_v)];
            }

            if (running) {
                auto time_curr = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float, std::chrono::seconds::period>(time_curr - time_last_cycle).count();

                if (dt > cycle_delay)
                {
                    time_last_cycle = time_curr;
                    emulate_cycle(app.emulator);
                }
            } else if (step) {
                emulate_cycle(app.emulator);
                step = false;
            }

            // Update view only when it is necessary
            uint8_t image[C8_DISPLAY_WIDTH*C8_DISPLAY_HEIGHT];
            for(int i =0; i <= C8_DISPLAY_WIDTH * C8_DISPLAY_HEIGHT; i++) {
                image[i] = app.emulator.display[i] * 0xFF;
            }
            glTextureSubImage2D(tex_display, 0, 0, 0, C8_DISPLAY_WIDTH, C8_DISPLAY_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, &image);

            // GUI
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            ImGui::DockSpaceOverViewport();

            // Emulator
            {
                ImGui::Begin("Emulator");

                if (ImGui::Button("Load")) {
                    ImGui::OpenPopup("Open File");
                } ImGui::SameLine();

                if(file_dialog.showFileDialog("Open File", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".ch8")) {
                    load_rom(app, file_dialog.selected_path.c_str());
                }

                if (!running) {
                    if (ImGui::Button("Run")) {
                        running = true;
                    }; ImGui::SameLine();

                    if (ImGui::Button("Step")) {
                        step = true;
                    }
                } else {
                    if(ImGui::Button("Stop")) {
                        running = false;
                    }
                }

                ImGui::Separator();

                ImGui::Image((void*)(intptr_t)tex_display, ImVec2(C8_DISPLAY_WIDTH*10, C8_DISPLAY_HEIGHT*10));
                ImGui::End();
            }

            // Disassembler
            {
                im_mem_edit.DrawWindow("Memory", &app.emulator.memory, C8_MEMORY_SIZE, 0);

                im_display_edit.DrawWindow("Display", &app.emulator.display, C8_DISPLAY_WIDTH * C8_DISPLAY_HEIGHT, 0);
            }

            // Meta
            {
                ImGui::Begin("Meta");
                
                // cycle_delay
                {
                    static float nb_cycles = 1. / cycle_delay;
                    if (ImGui::InputFloat("Cycles/Secs", &nb_cycles)) {
                        cycle_delay = 1. / nb_cycles;
                    }
                }
            }

            // Dynamic State
            {
                ImGui::Begin("Dynamic State");

                ImGui::Text("Instruction"); ImGui::Separator(); ImGui::Indent();
                {
                    ImGui::Text("OpCode: 0x%04x", app.emulator.opcode);
                }
                ImGui::Unindent();


                ImGui::Text("Memory"); ImGui::Separator(); ImGui::Indent();
                {
                    ImGui::Text("Pointer Counter: 0x%03x", app.emulator.pc);
                    if (ImGui::TreeNodeEx("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Text("I: 0x%03x", app.emulator.I);

                        ImGui::BeginTable("Registers", 2, tables_flags);
                        ImGui::TableSetupColumn("Vx");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();
                        for (unsigned int i = 0; i < C8_REGISTER_SIZE; i++) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("V[%2d]", i);
                            ImGui::TableNextColumn();
                            ImGui::Text("0x%03x", app.emulator.V[i]);
                        }   
                        ImGui::EndTable();
                        ImGui::TreePop();
                    }
                    
                    if (ImGui::TreeNodeEx("Stack", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Text("sp: %d", app.emulator.sp);

                        ImGui::BeginTable("Stack", 2, tables_flags);
                        ImGui::TableSetupColumn("Stack[x]");
                        ImGui::TableSetupColumn("Address");
                        ImGui::TableHeadersRow();
                        for (unsigned int i = 0; i < app.emulator.sp; i++) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%2d", i);
                            ImGui::TableNextColumn();
                            ImGui::Text("0x%03x", app.emulator.stack[i]);
                        }   
                        ImGui::EndTable();
                        ImGui::TreePop();
                    }
                    ImGui::Unindent();
                }
                
                ImGui::Text("Inputs"); ImGui::Separator(); ImGui::Indent();
                {
                    if (ImGui::TreeNodeEx("Keypads", ImGuiTreeNodeFlags_DefaultOpen)) {

                        ImGui::BeginTable("Keypads", 4, tables_flags);
                        const char* keypads_letters[16] = {
                            "X", "1", "2", "3", 
                            "Q", "W", "E", "A", 
                            "S", "D", "Z", "C",
                            "4", "R", "F", "V",
                        };

                        for (unsigned int i = 0; i < C8_KEYPAD_SIZE; i++) {
                            ImGui::TableNextColumn();
                            ImGui::Selectable(keypads_letters[i], static_cast<bool>(app.emulator.keypad[i]));
                        }
                        ImGui::EndTable();
                        ImGui::TreePop();
                    }
                }
                ImGui::Unindent();

                ImGui::Text("Extra"); ImGui::Separator(); ImGui::Indent();
                {
                    if (ImGui::TreeNodeEx("Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Text("Delay Timer: %d", app.emulator.delay_timer);
                        ImGui::Text("Sound Timer: %d", app.emulator.sound_timer);
                        ImGui::TreePop();
                    }
                }
                ImGui::Unindent();
                ImGui::End();
            }

            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(app.window);
        }
        glDeleteTextures(1, &tex_display);
    }
}