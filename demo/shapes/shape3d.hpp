#ifndef SHAPE3D_HPP
#define SHAPE3D_HPP

#include <vector>
#include <optional>
#include "../include/util.hpp"    
#include "../include/texture.hpp" // Include texture.hpp directly
#include "../include/rendering.hpp" // Include for Renderer 

struct Face {
    int v0, v1, v2;
};

struct Shape3D {
    std::vector<vec>  vertices;    
    std::vector<Face> faces;       
    std::vector<int>  faceColors;  
    std::vector<TexCoord> texCoords; // texture coordinates for each vertex
    std::optional<Texture> texture; // Optional texture to use
    int color; // base color
};

// Functions to create shapes:
Shape3D createPyramid(int N, float radius, float height, int color);
Shape3D createPrism(int N, float radius, float height, int color);
Shape3D createCuboid(float Lx, float Ly, float H, int color);

// Function to draw a Shape3D using the Renderer's camera
void drawShape(
    Renderer& renderer,
    const Shape3D& shape
);

// Function to draw a textured Shape3D using the Renderer's camera
void drawTexturedShape(
    Renderer& renderer,
    const Shape3D& shape
);

Shape3D createMinecraftDirtBlock(float size);

#endif // SHAPE3D_HPP
