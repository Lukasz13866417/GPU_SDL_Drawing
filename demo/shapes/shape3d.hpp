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
    int color; // base color
};

// Functions to create shapes:
Shape3D createHexPyramid(float radius, float height, int color);
Shape3D createHexPrism(float radius, float height, int color);
Shape3D createCuboid(float Lx, float Ly, float H, int color);

// Function to draw a Shape3D using your DepthBuffer and camera transforms.
void drawShape(
    DepthBuffer& depthBuffer,
    const Shape3D& shape,
    const vec& cameraPos,
    float cameraYaw,
    float cameraPitch
);

#endif // SHAPE3D_HPP
