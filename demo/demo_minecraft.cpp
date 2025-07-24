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
#include "../include/log.hpp"

void drawText(const std::string &what, int x, int y, SDL_Renderer* renderer){
    static TTF_Font* font = nullptr;
    if (!font) {
        font = TTF_OpenFont("../demo/comic_sans.ttf", 40);  // adjust font path & size
        if (!font) {
            LOG_ERR("Failed to load font: " + std::string(TTF_GetError()));
            return;
        }
    }

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, what.c_str(), color);
    if (!surface) {
        LOG_ERR("Failed to render text to surface: " + std::string(TTF_GetError()));
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        LOG_ERR("Failed to create texture from surface: " + std::string(SDL_GetError()));
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
        LOG_ERR("Failed to create surface for screenshot: " + std::string(SDL_GetError()));
        return;
    }

    // Read pixels from the renderer
    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, 
                            surface->pixels, surface->pitch) != 0) {
        LOG_ERR("Failed to read pixels: " + std::string(SDL_GetError()));
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
        LOG_ERR("Failed to save screenshot: " + std::string(SDL_GetError()));
    } else {
        LOG_SUCCESS("Screenshot saved: " + filename);
    }

    SDL_FreeSurface(surface);
}

int main(){
    // Initialize logging system
    LOG_INIT();
    LOG_INFO("Starting GPU SDL Drawing Minecraft Demo");
    
    // Initialize SDL
    LOG_DEBUG("Initializing SDL video subsystem");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERR("SDL could not initialize! Error: " + std::string(SDL_GetError()));
        return -1;
    }
    LOG_SUCCESS("SDL video subsystem initialized successfully");

    LOG_DEBUG("Initializing SDL TTF");
    if (TTF_Init() == -1) {
        LOG_ERR("TTF_Init failed: " + std::string(TTF_GetError()));
        return -1;
    }
    LOG_SUCCESS("SDL TTF initialized successfully");

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
        LOG_ERR("Window could not be created! Error: " + std::string(SDL_GetError()));
        SDL_Quit();
        return -1;
    }
    LOG_SUCCESS("SDL window created successfully");

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(sdlRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        screenWidth, screenHeight
    );

    // Initialize GPU and Renderer
    LOG_DEBUG("Initializing GPU and Renderer");
    initGPU();
    Renderer renderer(screenWidth, screenHeight, 1000);
    LOG_SUCCESS("GPU and Renderer initialized successfully");
    
    // Load texture from PNG file
    LOG_DEBUG("Loading dirt texture from file");
    std::optional<Texture> dirtTextureOpt = Texture::loadFromFile("../demo/textures/minecraft_dirt.png");
    if (!dirtTextureOpt) {
        LOG_ERR("Failed to load texture, exiting");
        return -1;
    }
    Texture dirtTexture = std::move(*dirtTextureOpt);
    LOG_SUCCESS("Dirt texture loaded successfully");

    // Create terrain map - 2D array where each element represents height (number of stacked blocks)
    const int terrainWidth = 5;
    const int terrainDepth = 5;
    const float blockSize = 300.0f;
    
    // Define terrain height map - you can modify these values to create different landscapes
    int terrainMap[terrainDepth][terrainWidth] = {
        {1,1,1,1,1},
        {1,2,2,2,1},
        {1,2,3,2,1},
        {1,2,2,2,1},
        {1,1,1,1,1},
    };

    // Create all dirt blocks based on terrain map
    std::vector<Shape3D> dirtBlocks;
    
    for (int z = 0; z < terrainDepth; z++) {
        for (int x = 0; x < terrainWidth; x++) {
            int height = terrainMap[z][x];
            
            // Create stacked blocks for this position
            for (int y = 0; y < height; y++) {
                Shape3D dirtBlock = createMinecraftDirtBlock(blockSize);
                dirtBlock.texture = dirtTexture;
                
                // Position the block in world space
                vec blockPos = {
                    (x - terrainWidth/2.0f) * blockSize,   // Center the terrain on X axis
                    y * blockSize,                          // Stack blocks vertically
                    (z - terrainDepth/2.0f) * blockSize    // Center the terrain on Z axis
                };
                
                // Apply position to all vertices
                for (auto &v : dirtBlock.vertices) {
                    v = v + blockPos;
                }
                
                dirtBlocks.push_back(dirtBlock);
            }
        }
    }

    LOG_SUCCESS("Created terrain with " + std::to_string(dirtBlocks.size()) + " dirt blocks");

    // Set up camera to view the terrain
    Camera& camera = renderer.getCamera();
    camera.setPosition(0, 4000, -5000);  // Higher and further back to see the terrain
    camera.setYawDegrees(0);              // Looking straight ahead
    camera.setPitchDegrees(-17.2f);       // Look down slightly (-0.3 radians = -17.2 degrees)
    camera.setRollDegrees(0);             // No roll
    LOG_SUCCESS("Camera configured to view terrain");

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
                    LOG_DEBUG("Screenshot requested by user");
                    saveScreenshot(sdlRenderer, screenWidth, screenHeight);
                }
            }
            else if (e.type == SDL_MOUSEMOTION) {
                camera.rotate(e.motion.xrel * mouseSens, e.motion.yrel * mouseSens, 0);
            }
        }

        // Keyboard
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_ESCAPE]) {
            quit = true;
        }

        if (keys[SDL_SCANCODE_W]) camera.moveForward(moveSpeed);
        if (keys[SDL_SCANCODE_S]) camera.moveForward(-moveSpeed);
        if (keys[SDL_SCANCODE_D]) camera.moveRight(moveSpeed);
        if (keys[SDL_SCANCODE_A]) camera.moveRight(-moveSpeed);
        if (keys[SDL_SCANCODE_SPACE]) camera.moveUp(moveSpeed);
        if (keys[SDL_SCANCODE_C]) camera.moveUp(-moveSpeed);

        SDL_RenderClear(sdlRenderer);

        // Start new frame for binning system
        renderer.startNewFrame();
        renderer.clear();     

        // Draw all the dirt blocks in the terrain (submits to binning)
        for (const auto& dirtBlock : dirtBlocks) {
            drawTexturedShape(renderer, dirtBlock);
        }

        // Execute binning pass and tile-based rendering
        renderer.executeBinningPass();
        renderer.executeFinishFrameTileBased();
        Uint32* colorBuffer = renderer.finishFrame();

        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(sdlRenderer, texture, NULL, NULL);

        drawText("Move with WASD", 100,100,sdlRenderer);
        drawText("Move up/down with SPACE / C", 100,150,sdlRenderer);
        drawText("Look around with mouse", 100,200,sdlRenderer);
        drawText("Press P to take a screenshot", 100,250,sdlRenderer);
        drawText("Terrain blocks: " + std::to_string(dirtBlocks.size()), 100,300,sdlRenderer);

        SDL_RenderPresent(sdlRenderer);

        // Cap framerate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Cleanup
    LOG_DEBUG("Starting cleanup process");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    deleteGPU(); // This will now also delete all textures
    LOG_INFO("GPU SDL Drawing Minecraft Demo exited successfully");

    return 0;
}