#ifndef SHAPE3D_HPP
#define SHAPE3D_HPP

#include <vector>
#include "../include/util.hpp"    
#include "../include/rendering.hpp" 

struct Face {
    int v0, v1, v2;
};

struct Shape3D {
    std::vector<vec>  vertices;    
    std::vector<Face> faces;       
    std::vector<int>  faceColors;  
    std::vector<TexCoord> texCoords; // texture coordinates for each vertex
    size_t textureID; // ID of the texture to use
    int color; // base color
};

// Functions to create shapes:
Shape3D createPyramid(int N, float radius, float height, int color);
Shape3D createPrism(int N, float radius, float height, int color);
Shape3D createCuboid(float Lx, float Ly, float H, int color);

// Function to draw a Shape3D using your DepthBuffer and camera transforms.
void drawShape(
    DepthBuffer& depthBuffer,
    const Shape3D& shape,
    const vec& cameraPos,
    float cameraYaw,
    float cameraPitch
);

// Function to draw a textured Shape3D
void drawTexturedShape(
    DepthBuffer& depthBuffer,
    const Shape3D& shape,
    const vec& cameraPos,
    float cameraYaw,
    float cameraPitch
);

Shape3D createMinecraftDirtBlock(float size);

#endif // SHAPE3D_HPP
