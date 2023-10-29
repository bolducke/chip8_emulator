// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

#include "app.h"


int main(int argc, char** argv) {

    chip8::App app;

    if (!chip8::create_app(app)) {
        return 1;
    }

    if (argc > 1) {
        chip8::load_rom(app, argv[1]);
    }

    chip8::run(app);

    chip8::destroy_app(app);

    return 0;
}