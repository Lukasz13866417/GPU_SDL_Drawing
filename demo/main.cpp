#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cstdlib>   
#include <ctime>    
#include "shapes/tower.hpp"      
#include "../include/rendering.hpp" 
#include "../include/util.hpp"      


void drawText(const std::string &what, int x, int y, SDL_Renderer* renderer){
    static TTF_Font* font = nullptr;
    if (!font) {
        font = TTF_OpenFont("../demo/comic_sans.ttf", 40);  // adjust font path & size
        if (!font) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            return;
        }
    }

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, what.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text to surface: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }
    SDL_FreeSurface(surface);

    SDL_Rect dstRect;
    dstRect.x = x;  // x position on screen
    dstRect.y = y;  // y position on screen
    SDL_QueryTexture(texture, NULL, NULL, &dstRect.w, &dstRect.h);

    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

int main(){
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
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

    const int nTowers = 5;

    std::vector<Tower> towers;
    towers.reserve(nTowers);

    for (int i = 0; i < nTowers; ++i) {
        float x = float((rand() % 16001) - 8000);
        float z = float((rand() % 16001) + 8000);
        vec towerPos = {x, -100.0f, z};

        Tower t(225.0f, 550.0f + float(rand()%551), 300.0f, 300.0f, 0.01f, towerPos);
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

        depthBuffer.clear();     

        for (auto &tower : towers) {
            tower.update();
            tower.draw(depthBuffer, cameraPos, cameraYaw, cameraPitch);
        }

        Uint32* colorBuffer = depthBuffer.finishFrame();

        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        drawText("Move with WASD", 100,100,renderer);
        drawText("Move up/down with SPACE / C", 100,150,renderer);
        drawText("Look around with mouse", 100,200,renderer);

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
