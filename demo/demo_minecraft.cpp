#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <ctime>    
#include <string>
#include <iomanip>
#include <sstream>
#include "shapes/shape3d.hpp"
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

void saveScreenshot(SDL_Renderer* renderer, int width, int height) {
    // Create a surface with the same dimensions as the renderer
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    if (!surface) {
        std::cerr << "Failed to create surface for screenshot: " << SDL_GetError() << std::endl;
        return;
    }

    // Read pixels from the renderer
    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, 
                            surface->pixels, surface->pitch) != 0) {
        std::cerr << "Failed to read pixels: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    // Generate a unique filename using a timestamp
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    std::stringstream ss;
    ss << "screenshot_" << (now->tm_year + 1900) << '-' 
       << std::setfill('0') << std::setw(2) << (now->tm_mon + 1) << '-'
       << std::setfill('0') << std::setw(2) << now->tm_mday << '_'
       << std::setfill('0') << std::setw(2) << now->tm_hour
       << std::setfill('0') << std::setw(2) << now->tm_min
       << std::setfill('0') << std::setw(2) << now->tm_sec << ".bmp";
    std::string filename = ss.str();

    // Save as BMP
    if (SDL_SaveBMP(surface, filename.c_str()) != 0) {
        std::cerr << "Failed to save screenshot: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "Screenshot saved: " << filename << std::endl;
    }

    SDL_FreeSurface(surface);
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

    int screenWidth = 2560;
    int screenHeight = 1440;
    SDL_Window* window = SDL_CreateWindow("Demo Minecraft",
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
    
    // Load texture from PNG file and get its ID
    std::optional<size_t> textureIDOpt = Texture::loadFromFile("../demo/textures/minecraft_dirt.png");
    if (!textureIDOpt) {
        std::cerr << "Failed to load texture, exiting." << std::endl;
        // In a real app, you might fall back to a default texture
        return -1;
    }
    size_t dirtTextureID = *textureIDOpt;

    // Create a Minecraft dirt block for testing
    Shape3D dirtBlock = createMinecraftDirtBlock(800.0f);
    dirtBlock.textureID = dirtTextureID;
    
    // Position the dirt block
    vec cubePos = {0, 400, 500};
    for (auto &v : dirtBlock.vertices) {
        v = v + cubePos;
    }

    // Camera
    vec  cameraPos   = {0, 400, -900};
    float cameraYaw   = 0.0f;
    float cameraPitch = 0.0f;

    float moveSpeed = 60.0f;
    float mouseSens = 0.001f;

    bool quit = false;
    SDL_Event e;

    // Frame timing
    const int FPS = 120;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    size_t lagCount = 0;

    uint64_t xd=0;

    while (!quit) {
        frameStart = SDL_GetTicks();

        // Poll events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_p) {
                    saveScreenshot(renderer, screenWidth, screenHeight);
                }
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

        for (auto &v : dirtBlock.vertices) {
            v = rotY(v, {0,400,500}, 0.025f);
            v = rotX(v, {0,400,500}, 0.025f);
        }

        SDL_RenderClear(renderer);

        depthBuffer.clear();     

        // Draw the textured cube
        drawTexturedShape(depthBuffer, dirtBlock, cameraPos, cameraYaw, cameraPitch);

        Uint32* colorBuffer = depthBuffer.finishFrame();

        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        drawText("Move with WASD", 100,100,renderer);
        drawText("Move up/down with SPACE / C", 100,150,renderer);
        drawText("Look around with mouse", 100,200,renderer);
        drawText("Press P to take a screenshot", 100,250,renderer);

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
    deleteGPU(); // This will now also delete all textures

    return 0;
}