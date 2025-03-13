#include "shape3d.hpp"
#include <cmath>       
#include <cstdlib>         

static const float PI = 3.1415926535f;

int makeColorDarker(int baseColor)
{
    int r = (baseColor >> 16) & 0xFF;
    int g = (baseColor >>  8) & 0xFF;
    int b = (baseColor      ) & 0xFF;

    r = static_cast<int>(r * 0.8f);
    g = static_cast<int>(g * 0.8f);
    b = static_cast<int>(b * 0.8f);

    return (r << 16) | (g << 8) | b;
}


void drawShape(
    DepthBuffer& depthBuffer,
    const Shape3D& shape,
    const vec& cameraPos,
    float cameraYaw,
    float cameraPitch)
{
    for (size_t i = 0; i < shape.faces.size(); i++) {
        const Face& face = shape.faces[i];

        // World coords
        vec v0World = shape.vertices[face.v0];
        vec v1World = shape.vertices[face.v1];
        vec v2World = shape.vertices[face.v2];

        // Translate so camera is at origin
        v0World = v0World - cameraPos;
        v1World = v1World - cameraPos;
        v2World = v2World - cameraPos;

        // Rotate around Y by -cameraYaw
        v0World = rotY(v0World, {0,0,0}, -cameraYaw);
        v1World = rotY(v1World, {0,0,0}, -cameraYaw);
        v2World = rotY(v2World, {0,0,0}, -cameraYaw);

        // Rotate around X by -cameraPitch
        v0World = rotX(v0World, {0,0,0}, -cameraPitch);
        v1World = rotX(v1World, {0,0,0}, -cameraPitch);
        v2World = rotX(v2World, {0,0,0}, -cameraPitch);

        // Draw
        int faceColor = shape.faceColors[i];
        depthBuffer.drawTriangle(v0World, v1World, v2World, faceColor);
    }
}

Shape3D createPyramid(int N, float radius, float height, int color)
{
    Shape3D pyramid;
    pyramid.color = color;

    float angleStep = 2.0f * PI / N;

    for(int i = 0; i < N; ++i) {
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        pyramid.vertices.push_back({x, 0.0f, z});
    }
    pyramid.vertices.push_back({0.0f, height, 0.0f});
    int apexIndex = (int)pyramid.vertices.size() - 1;

    int darkColor = makeColorDarker(color);

    for(int i = 0; i < N; ++i) {
        int next = (i+1) % N;
        pyramid.faces.push_back({i, next, apexIndex});
        if (i % 2 == 0) {
            pyramid.faceColors.push_back(color);
        } else {
            pyramid.faceColors.push_back(darkColor);
        }
    }

    for(int i = 1; i < N - 1; ++i) {
        pyramid.faces.push_back({0, i, i+1});
        pyramid.faceColors.push_back(color);
    }

    return pyramid;
}


Shape3D createPrism(int N, float radius, float height, int color)
{
    Shape3D prism;
    prism.color = color;

    float angleStep = 2.0f * PI / N;

    for(int i = 0; i < N; ++i){
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        prism.vertices.push_back({x, 0.0f, z});
    }
    for(int i = 0; i < N; ++i){
        float theta = i * angleStep;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        prism.vertices.push_back({x, height, z});
    }

    auto bottomIndex = [&](int i){ return i; };
    auto topIndex    = [&](int i){ return i + N; };

    int darkColor = makeColorDarker(color);

    for(int i = 1; i < N-1; ++i) {
        prism.faces.push_back({ topIndex(0), topIndex(i), topIndex(i+1) });
        prism.faceColors.push_back(color);
    }

    for(int i = 1; i < N-1; ++i) {
        prism.faces.push_back({ bottomIndex(0), bottomIndex(i+1), bottomIndex(i) });
        prism.faceColors.push_back(color);
    }

    for(int i = 0; i < N; ++i) {
        int next = (i+1) % N;
        // pick color or darkColor based on i%2
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

    // 8 vertices
    box.vertices.push_back({-Lx, 0.0f, -Ly}); // 0
    box.vertices.push_back({+Lx, 0.0f, -Ly}); // 1
    box.vertices.push_back({+Lx, 0.0f, +Ly}); // 2
    box.vertices.push_back({-Lx, 0.0f, +Ly}); // 3

    box.vertices.push_back({-Lx, H, -Ly});    // 4
    box.vertices.push_back({+Lx, H, -Ly});    // 5
    box.vertices.push_back({+Lx, H, +Ly});    // 6
    box.vertices.push_back({-Lx, H, +Ly});    // 7

    int darkColor = makeColorDarker(color);

    box.faces.push_back({0, 1, 2});  box.faceColors.push_back(color);
    box.faces.push_back({0, 2, 3});  box.faceColors.push_back(color);

    box.faces.push_back({4, 6, 5});  box.faceColors.push_back(color);
    box.faces.push_back({4, 7, 6});  box.faceColors.push_back(color);

    box.faces.push_back({0, 4, 1});  box.faceColors.push_back(darkColor);
    box.faces.push_back({1, 4, 5});  box.faceColors.push_back(darkColor);

    box.faces.push_back({3, 2, 7});  box.faceColors.push_back(darkColor);
    box.faces.push_back({2, 6, 7});  box.faceColors.push_back(darkColor);

    box.faces.push_back({1, 5, 2});  box.faceColors.push_back(color);
    box.faces.push_back({2, 5, 6});  box.faceColors.push_back(color);

    box.faces.push_back({0, 3, 4});  box.faceColors.push_back(color);
    box.faces.push_back({3, 7, 4});  box.faceColors.push_back(color);

    return box;
}

