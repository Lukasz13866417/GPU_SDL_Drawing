#include "../include/rendering.hpp"
#include "../include/log.hpp"
#include "../include/util.hpp"
#include "../include/buffer.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

// Simple function to draw text on screen
void drawText(const std::string& text, int x, int y, SDL_Renderer* renderer) {
    // Simple colored rectangle as placeholder for text
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = {x, y, (int)text.length() * 8, 16};
    SDL_RenderFillRect(renderer, &rect);
}

int main() {
    // Initialize logging system
    LOG_INIT();
    LOG_INFO("Starting Vertex Buffer Demo");

    // SDL initialization
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_FATAL("SDL could not initialize! SDL_Error: " + std::string(SDL_GetError()));
        return -1;
    }

    const int screenWidth = 800;
    const int screenHeight = 600;

    SDL_Window* window = SDL_CreateWindow("Vertex Buffer Demo", 
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                        screenWidth, screenHeight, SDL_WINDOW_SHOWN);
    if (!window) {
        LOG_FATAL("Window could not be created! SDL_Error: " + std::string(SDL_GetError()));
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdlRenderer) {
        LOG_FATAL("Renderer could not be created! SDL_Error: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTexture(sdlRenderer,
                                           SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           screenWidth, screenHeight);

    // Initialize GPU and Renderer
    LOG_DEBUG("Initializing GPU and Renderer");
    initGPU();
    Renderer renderer(screenWidth, screenHeight, 1000);
    LOG_SUCCESS("GPU and Renderer initialized successfully");

    // Create a vertex buffer with a simple triangle
    // Original world-space vertices
    std::vector<vec> originalVertices = {
        vec(-200.0f, -100.0f, 500.0f),  // v0: bottom left
        vec( 200.0f, -100.0f, 500.0f),  // v1: bottom right  
        vec(   0.0f,  150.0f, 500.0f),  // v2: top center
        vec(-100.0f,  100.0f, 400.0f)   // v3: another vertex for demonstration
    };
    
    LOG_DEBUG("Original vertices defined in world space");

    // Define triangle colors
    int redColor = fromRgb(255, 0, 0);
    int greenColor = fromRgb(0, 255, 0);
    int blueColor = fromRgb(0, 0, 255);

    bool running = true;
    SDL_Event e;
    int frameCount = 0;

    LOG_INFO("Starting render loop");

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        SDL_RenderClear(sdlRenderer);
        
        // Start new frame for binning system
        renderer.startNewFrame();
        renderer.clear();

        // Apply camera transformations to vertices
        std::vector<vec> transformedVertices;
        transformedVertices.reserve(originalVertices.size());
        
        const Camera& camera = renderer.getCamera();
        for (const vec& vertex : originalVertices) {
            transformedVertices.push_back(camera.transformVertex(vertex));
        }
        
        // Create vertex buffer with camera-transformed vertices
        lr::AllPurposeBuffer<vec> vertexBuffer(transformedVertices.size(), transformedVertices);

        // Submit triangles for binning using vertex buffer
        renderer.submitTriangleForBinning(vertexBuffer, 0, 1, 2, greenColor);
        renderer.submitTriangleForBinning(vertexBuffer, 1, 2, 3, blueColor);

        // Execute binning pass and tile-based rendering
        renderer.executeBinningPass();
        renderer.executeFinishFrameTileBased();
        Uint32* colorBuffer = renderer.finishFrame();
        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(sdlRenderer, texture, NULL, NULL);

        // Add frame counter
        drawText("Frame: " + std::to_string(frameCount), 10, screenHeight - 30, sdlRenderer);
        drawText("Press ESC to exit", 10, screenHeight - 50, sdlRenderer);

        SDL_RenderPresent(sdlRenderer);
        frameCount++;

        // Limit to ~60 FPS
        SDL_Delay(16);
    }

    // Cleanup
    LOG_DEBUG("Starting cleanup process");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    deleteGPU();
    LOG_INFO("Vertex Buffer Demo exited successfully");

    return 0;
} 