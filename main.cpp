// Just a demo. 
// The important stuff is in include/ and src/ 
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cmath>      
#include "../include/rendering.hpp"  
#include "../include/util.hpp"       

int makeColorDarker(int baseColor) {
    int r = (baseColor >> 16) & 0xFF;
    int g = (baseColor >>  8) & 0xFF;
    int b = (baseColor >>  0) & 0xFF;

    r = static_cast<int>(r * 0.8f);
    g = static_cast<int>(g * 0.8f);
    b = static_cast<int>(b * 0.8f);

    return (r << 16) | (g << 8) | b;
}

struct Face {
    int v0, v1, v2;
};

struct Shape3D {
    std::vector<vec>  vertices;    // each vec is (x,y,z)
    std::vector<Face> faces;       // each face has 3 vertex indices
    std::vector<int>  faceColors;  // color for each face (same indexing as faces)
    int color; // base color (should have no value if makes no sense)
};

Shape3D createHexPyramid(float radius, float height, int color)
{
    Shape3D pyramid;
    pyramid.color = color;

    const int N = 6;
    float angleStep = 2.0f * 3.1415926535f / N;

    // The hex base
    for(int i = 0; i < N; ++i) {
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        pyramid.vertices.push_back({x, 0.0f, z});
    }
    // Apex
    pyramid.vertices.push_back({0.0f, height, 0.0f});
    int apexIndex = (int)pyramid.vertices.size() - 1;

    int darkColor = makeColorDarker(color);

    for(int i = 0; i < N; ++i) {
        int next = (i+1)%N;
        // create face
        pyramid.faces.push_back({i, next, apexIndex});

        // color it
        if (i % 2 == 0) {
            pyramid.faceColors.push_back(color);
        } else {
            pyramid.faceColors.push_back(darkColor);
        }
    }

    //  Base face: fan triangulation around vertex 0
    for(int i = 1; i < N - 1; ++i) {
        pyramid.faces.push_back({ 0, i, i+1 });
        pyramid.faceColors.push_back(color);
    }

    return pyramid;
}

Shape3D createHexPrism(float radius, float height, int color)
{
    Shape3D prism;
    prism.color = color;  // store base color
    prism.vertices.reserve(12); // 6 bottom + 6 top
    prism.faces.reserve( (6-2)*2 + (6-2)*2 + 6*2 ); // enough space
    prism.faceColors.reserve( (6-2)*2 + (6-2)*2 + 6*2 );

    const int N = 6;
    float angleStep = 2.0f * 3.1415926535f / N;

    // Bottom ring
    for(int i = 0; i < N; ++i){
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        prism.vertices.push_back({x, 0.0f, z});
    }
    // Top ring
    for(int i = 0; i < N; ++i){
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        prism.vertices.push_back({x, height, z});
    }

    // Some local lambdas for convenience:
    auto bottomIndex = [&](int i){ return i; };
    auto topIndex    = [&](int i){ return i + N; };

    // Top face 
    int topFaceColor = color;
    for(int i = 1; i < N-1; ++i){
        prism.faces.push_back({ topIndex(0), topIndex(i), topIndex(i+1) });
        prism.faceColors.push_back(topFaceColor);
    }

    // Bottom face 
    int bottomFaceColor = color;
    for(int i = 1; i < N-1; ++i){
        prism.faces.push_back({ bottomIndex(0), bottomIndex(i+1), bottomIndex(i) });
        prism.faceColors.push_back(bottomFaceColor);
    }

    // Side rectangles (2 triangles per side).
    int darkColor = makeColorDarker(color);

    for(int i = 0; i < N; ++i) {
        int next = (i+1)%N;

        // If i%2==0, use the original color; else use darker
        int sideColor = (i % 2 == 0) ? color : darkColor;

        prism.faces.push_back({ bottomIndex(i), bottomIndex(next), topIndex(i) });
        prism.faceColors.push_back(sideColor);

        prism.faces.push_back({ topIndex(i), bottomIndex(next), topIndex(next) });
        prism.faceColors.push_back(sideColor);
    }

    return prism;
}

Shape3D createCuboid(float Lx, float Ly, float H, int color)
{
    Shape3D box;
    box.color = color;
    // bottom
    box.vertices.push_back({-Lx, 0.0f, -Ly}); // 0
    box.vertices.push_back({+Lx, 0.0f, -Ly}); // 1
    box.vertices.push_back({+Lx, 0.0f, +Ly}); // 2
    box.vertices.push_back({-Lx, 0.0f, +Ly}); // 3
    
    // top
    box.vertices.push_back({-Lx, H, -Ly});    // 4
    box.vertices.push_back({+Lx, H, -Ly});    // 5
    box.vertices.push_back({+Lx, H, +Ly});    // 6
    box.vertices.push_back({-Lx, H, +Ly});    // 7
    
    // Triangles for each face
    box.faces.push_back({0, 1, 2});
    box.faces.push_back({0, 2, 3});
    box.faces.push_back({4, 6, 5});
    box.faces.push_back({4, 7, 6});

    box.faces.push_back({0, 4, 1});
    box.faces.push_back({1, 4, 5});
    box.faces.push_back({3, 2, 7});
    box.faces.push_back({2, 6, 7});

    box.faces.push_back({1, 5, 2});
    box.faces.push_back({2, 5, 6});
    box.faces.push_back({0, 3, 4});
    box.faces.push_back({3, 7, 4});

    // Assign random-ish color to each face
    box.faceColors.reserve(box.faces.size());
    for (size_t i = 0; i < box.faces.size(); i++) {
        box.faceColors.push_back( box.color );
    }

    return box;
}


// Draw a Shape3D by transforming each vertex into camera space
// and passing that to the DepthBuffer.

// cameraPos   = where the camera is in world coordinates
// cameraYaw   = rotation around Y axis (in radians)
// cameraPitch = rotation around X axis (in radians)
void drawShape(
    DepthBuffer& depthBuffer,
    const Shape3D& shape,
    const vec& cameraPos,
    float cameraYaw,
    float cameraPitch)
{
    // Loop through faces
    for (size_t i = 0; i < shape.faces.size(); i++) {
        const Face& face = shape.faces[i];
        
        const vec &v0World = shape.vertices[face.v0];
        const vec &v1World = shape.vertices[face.v1];
        const vec &v2World = shape.vertices[face.v2];

        vec v0Cam = v0World - cameraPos;
        vec v1Cam = v1World - cameraPos;
        vec v2Cam = v2World - cameraPos;

        v0Cam = rotY(v0Cam, {0,0,0}, -cameraYaw);
        v1Cam = rotY(v1Cam, {0,0,0}, -cameraYaw);
        v2Cam = rotY(v2Cam, {0,0,0}, -cameraYaw);

        v0Cam = rotX(v0Cam, {0,0,0}, -cameraPitch);
        v1Cam = rotX(v1Cam, {0,0,0}, -cameraPitch);
        v2Cam = rotX(v2Cam, {0,0,0}, -cameraPitch);

        int faceColor = shape.faceColors[i];
        depthBuffer.drawTriangle(v0Cam, v1Cam, v2Cam, faceColor);
    }
}


// Tower struct holds 3 shapes: a base cuboid, a hex prism,
// and a hex pyramid, all stacked. It also spins at constant speed.
struct Tower {
    Shape3D baseCuboid;
    Shape3D hexPrism;
    Shape3D hexPyramid;
    float rotationSpeed;  
    vec position;

    Tower(float cuboidHeight, float prismHeight, float pyramidHeight,
          float radius, float spinSpeed, const vec& position)
        : rotationSpeed(spinSpeed)
        , position(position)
    {
        // Create each shape
        baseCuboid = createCuboid(radius * 1.2f, radius * 1.2f, cuboidHeight, 0xFF00FF); 
        hexPrism   = createHexPrism(radius, prismHeight, 0x00FF00);                     
        hexPyramid = createHexPyramid(radius, pyramidHeight, 0xFFFF00);                 

        // Stack them by shifting in Y
        for (auto &v : hexPrism.vertices) {
            v.y += cuboidHeight + 1;  
        }
        for (auto &v : hexPyramid.vertices) {
            v.y += (cuboidHeight + prismHeight + 1); 
        }

        for (auto &v : baseCuboid.vertices) {
            v = v + position;
        }
        for (auto &v : hexPrism.vertices) {
            v = v + position;
        }
        for (auto &v : hexPyramid.vertices) {
            v = v + position;
        }
    }

    void update() {
        for (auto &v : baseCuboid.vertices) {
            v = rotY(v, position, rotationSpeed);
        }
        for (auto &v : hexPrism.vertices) {
            v = rotY(v, position, rotationSpeed);
        }
        for (auto &v : hexPyramid.vertices) {
            v = rotY(v, position, rotationSpeed);
        }
    }

    void draw(DepthBuffer& depthBuffer, const vec& cameraPos, float cameraYaw, float cameraPitch)
    {
        drawShape(depthBuffer, baseCuboid, cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPrism,   cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPyramid, cameraPos, cameraYaw, cameraPitch);
    }
};

int main(){
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    int screenWidth = 1280;
    int screenHeight = 720;

    SDL_Window* window = SDL_CreateWindow("SDL Tower (Hex Pyramid + Prism + Cuboid)",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          screenWidth, screenHeight, SDL_WINDOW_SHOWN);

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             screenWidth, screenHeight);

    initGPU();
    DepthBuffer depthBuffer(screenWidth, screenHeight, 1000);

    // Generate 10 towers with random positions
    srand(static_cast<unsigned>(time(nullptr)));

    std::vector<Tower> towers;
    towers.reserve(5);

    for (int i = 0; i < 5; ++i) {
        float x = static_cast<float>((rand() % 6001) - 3000);  
        float z = static_cast<float>((rand() % 6001) + 3000);   
        vec towerPos = { x, -100.0f, z };

        Tower t(200.0f, 400.0f, 250.0f, 300.0f, 0.01f, towerPos);
        towers.push_back(std::move(t));
    }

    // Camera parameters
    vec  cameraPos   = {0, 300, -900};
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

        // Handle all events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEMOTION) {
                cameraYaw   += e.motion.xrel * mouseSens;
                cameraPitch -= e.motion.yrel * mouseSens;

                const float maxPitch = 1.57f; 
                if (cameraPitch >  maxPitch) cameraPitch =  maxPitch;
                if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;
            }
        }

        // Keyboard state
        const Uint8* keyStates = SDL_GetKeyboardState(NULL);

        if (keyStates[SDL_SCANCODE_ESCAPE]) {
            quit = true;
        }

        // Basic "FPS" movement ignoring pitch
        float cy = cos(cameraYaw);
        float sy = sin(cameraYaw);
        vec forward = { sy, 0.0f, cy };
        vec right   = { cy, 0.0f, -sy };

        if (keyStates[SDL_SCANCODE_W]) cameraPos = cameraPos + forward * moveSpeed;
        if (keyStates[SDL_SCANCODE_S]) cameraPos = cameraPos - forward * moveSpeed;
        if (keyStates[SDL_SCANCODE_D]) cameraPos = cameraPos + right   * moveSpeed;
        if (keyStates[SDL_SCANCODE_A]) cameraPos = cameraPos - right   * moveSpeed;
        if (keyStates[SDL_SCANCODE_SPACE]) cameraPos.y += moveSpeed;
        if (keyStates[SDL_SCANCODE_C])     cameraPos.y -= moveSpeed;

        // Render
        SDL_RenderClear(renderer);

        // Update + Draw every tower
        for (auto &tower : towers) {
            tower.update();
            tower.draw(depthBuffer, cameraPos, cameraYaw, cameraPitch);
        }

        // Present
        Uint32* colorBuffer = depthBuffer.finishFrame();
        SDL_UpdateTexture(texture, NULL, colorBuffer, screenWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Frame cap
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }else{
            std::cerr<<"OUCH"<<std::endl;
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
