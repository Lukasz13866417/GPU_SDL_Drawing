#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cstdlib>   
#include <ctime>    
#include "shapes/tower.hpp"      
#include "../include/rendering.hpp" 
#include "../include/util.hpp"      

int main(){
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    int screenWidth = 1280;
    int screenHeight = 720;
    SDL_Window* window = SDL_CreateWindow("Demo Towers",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screenWidth, screenHeight,
        SDL_WINDOW_SHOWN
    );

    // Fullscreen, hide cursor, capture mouse
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    if (!window) {
        std::cerr << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        screenWidth, screenHeight
    );

    // Initialize GPU and DepthBuffer
    initGPU();
    DepthBuffer depthBuffer(screenWidth, screenHeight, 1000);

    // Create multiple random towers
    srand((unsigned)time(nullptr));

    std::vector<Tower> towers;
    towers.reserve(5);

    for (int i = 0; i < 5; ++i) {
        // random x in [-3000..+3000]
        float x = float((rand() % 6001) - 3000);
        // random z in [3000..9000], for instance
        float z = float((rand() % 6001) + 3000);
        vec towerPos = {x, -100.0f, z};

        Tower t(200.0f, 400.0f, 250.0f, 300.0f, 0.01f, towerPos);
        towers.push_back(t);
    }

    // Camera
    vec  cameraPos   = {0, 400, -900};
    float cameraYaw   = 0.0f;
    float cameraPitch = 0.0f;

    float moveSpeed = 20.0f;
    float mouseSens = 0.001f;

    bool quit = false;
    SDL_Event e;

    // Frame timing
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    while (!quit) {
        frameStart = SDL_GetTicks();

        // Poll events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEMOTION) {
                cameraYaw   += e.motion.xrel * mouseSens;
                cameraPitch += e.motion.yrel * mouseSens;

                // clamp pitch
                const float maxPitch = 1.57f;
                if (cameraPitch >  maxPitch) cameraPitch =  maxPitch;
                if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;
            }
        }

        // Keyboard
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_ESCAPE]) {
            quit = true;
        }

        float cy = cos(cameraYaw);
        float sy = sin(cameraYaw);
        vec forward = { sy, 0.0f, cy };
        vec right   = { cy, 0.0f, -sy };

        if (keys[SDL_SCANCODE_W]) cameraPos = cameraPos + forward * moveSpeed;
        if (keys[SDL_SCANCODE_S]) cameraPos = cameraPos - forward * moveSpeed;
        if (keys[SDL_SCANCODE_D]) cameraPos = cameraPos + right   * moveSpeed;
        if (keys[SDL_SCANCODE_A]) cameraPos = cameraPos - right   * moveSpeed;
        if (keys[SDL_SCANCODE_SPACE]) cameraPos.y += moveSpeed;
        if (keys[SDL_SCANCODE_C])     cameraPos.y -= moveSpeed;

        SDL_RenderClear(renderer);

        // Update & draw all towers
        for (auto &tower : towers) {
            tower.update();
            tower.draw(depthBuffer, cameraPos, cameraYaw, cameraPitch);
        }

        // Present
        Uint32* colorBuffer = depthBuffer.finishFrame();
        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Cap framerate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    deleteGPU();

    return 0;
}
