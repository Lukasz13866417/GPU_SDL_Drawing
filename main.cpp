#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cassert>
#include "../include/rendering.hpp"
#include "../include/util.hpp"

int main(){
// Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Set screen dimensions
    int screenWidth = 1920;
    int screenHeight = 1080;

    SDL_Window* window = SDL_CreateWindow("3D Cube with SDL and OpenCL",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          screenWidth, screenHeight, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             screenWidth, screenHeight);

    initGPU();
    DepthBuffer depthBuffer(screenWidth, screenHeight, 1000); // z-distance to screen

    // Main loop flag
    bool quit = false;
    SDL_Event e;

    float y=-50;

    vec O = {0,0,1000}, O2 = {500,0,1000}, O3 = {0,0,1000};
    std::vector<vec> origins({O/*,O2,O3*/});
    vec V1 = vec{-100, -100, -100};
    vec V2 = vec{100, -100, -100};
    vec V3 = vec{100, 100, -100};
    vec V4 = vec{-100, 100, -100};
    vec V5 = vec{-100, -100, 100};
    vec V6 = vec{100, -100, 100};
    vec V7 = vec{100, 100, 100};
    vec V8 = vec{-100, 100, 100};


    std::vector<vec*> verts;
    verts.push_back(&V1);
    verts.push_back(&V2);
    verts.push_back(&V3);
    verts.push_back(&V4);
    verts.push_back(&V5);
    verts.push_back(&V6);
    verts.push_back(&V7);
    verts.push_back(&V8);

    std::vector<vec> verts2;
    verts2.push_back(V1);
    verts2.push_back(V2);
    verts2.push_back(V3);
    verts2.push_back(V4);
    verts2.push_back(V5);
    verts2.push_back(V6);
    verts2.push_back(V7);
    verts2.push_back(V8);

    int frontColor = 0xFF0000; // Red
    int backColor = 0x00FF00; // Green
    int topColor = 0x0000FF; // Blue
    int bottomColor = 0xFFFF00; // Yellow
    int rightColor = 0xFF00FF; // Magenta
    int leftColor = 0x00FFFF; // Cyan
   

    
    int countedFrames = 0;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS; // Target frame time in milliseconds
    Uint32 frameStart;
    int frameTime;
    float rotSpeed=0.0175f;
    while (!quit) {
        frameStart = SDL_GetTicks();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } 
        }
        for(vec* v : verts){
            *v = rotY(*v, {0,0,0}, rotSpeed);
            *v = rotZ(*v, {0,0,0}, rotSpeed);
        }
        SDL_RenderClear(renderer);

        //O=rotX(O,3.141f/250);
        //O -= {0,0,2};

        // Draw triangles for each face

        for(int i=-5;i<6;++i){

            // Front face
                depthBuffer.drawTriangle(V3+O + vec{150,0,0}*i, V2+O + vec{150,0,0}*i, V1+O + vec{150,0,0}*i, frontColor);
                depthBuffer.drawTriangle(V4+O + vec{150,0,0}*i, V3+O + vec{150,0,0}*i, V1+O + vec{150,0,0}*i, frontColor);

                // Back face
                depthBuffer.drawTriangle(V5+O + vec{150,0,0}*i, V6+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, backColor);
                depthBuffer.drawTriangle(V5+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, V8+O + vec{150,0,0}*i, backColor);

                // Top face
                depthBuffer.drawTriangle(V4+O + vec{150,0,0}*i, V3+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, topColor);
                depthBuffer.drawTriangle(V4+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, V8+O + vec{150,0,0}*i, topColor);

                // Bottom face
                depthBuffer.drawTriangle(V1+O + vec{150,0,0}*i, V2+O + vec{150,0,0}*i, V6+O + vec{150,0,0}*i, bottomColor);
                depthBuffer.drawTriangle(V1+O + vec{150,0,0}*i, V6+O + vec{150,0,0}*i, V5+O + vec{150,0,0}*i, bottomColor);

                // Right face
                depthBuffer.drawTriangle(V2+O + vec{150,0,0}*i, V3+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, rightColor);
                depthBuffer.drawTriangle(V2+O + vec{150,0,0}*i, V7+O + vec{150,0,0}*i, V6+O + vec{150,0,0}*i, rightColor);

                // Left face
                depthBuffer.drawTriangle(V8+O + vec{150,0,0}*i, V4+O + vec{150,0,0}*i, V1+O + vec{150,0,0}*i, leftColor);
                depthBuffer.drawTriangle(V5+O + vec{150,0,0}*i, V8+O + vec{150,0,0}*i, V1+O + vec{150,0,0}*i, leftColor);
        }

        

        // Update texture with the rendered color buffer
        Uint32* colorBuffer = depthBuffer.finishFrame(); // Get the rendered frame
        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32)); // Assuming colorBuffer is std::vector<int> and matches the texture format
        ++countedFrames;
        // Copy texture to renderer and present
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }else{
            std::cerr<<"OUCH"<<std::endl;
        }
    }
    //std::cout<<countedFrames;

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    deleteGPU();

    return 0;
}
