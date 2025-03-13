#ifndef TOWER_HPP
#define TOWER_HPP

#include "shape3d.hpp"

struct Tower {
    Shape3D basePrism;
    Shape3D hexPrism;
    Shape3D hexPyramid;
    float rotationSpeed;  
    vec position;

    Tower(float basePrismHeight, float prismHeight, float pyramidHeight,
          float radius, float spinSpeed, const vec& pos)
        : rotationSpeed(spinSpeed), position(pos)
    {
        basePrism = createPrism(5, radius * 1.4f, basePrismHeight, fromRgb(100,100,100));
        hexPrism   = createPrism(10,radius, prismHeight, fromRgb(51,51,51));
        hexPyramid = createPyramid(10,radius, pyramidHeight, fromRgb(100,50,50));

        for (auto &v : hexPrism.vertices) {
            v.y += basePrismHeight + 1;
        }
        for (auto &v : hexPyramid.vertices) {
            v.y += (basePrismHeight + prismHeight + 1);
        }

        for (auto &v : basePrism.vertices)  { v = v + pos; }
        for (auto &v : hexPrism.vertices)    { v = v + pos; }
        for (auto &v : hexPyramid.vertices)  { v = v + pos; }
    }

    void update() {
        /*for (auto &v : basePrism.vertices) {
            v = rotY(v, position, rotationSpeed);
        }
        for (auto &v : hexPrism.vertices) {
            v = rotY(v, position, rotationSpeed);
        }
        for (auto &v : hexPyramid.vertices) {
            v = rotY(v, position, rotationSpeed);
        }*/
    }

    void draw(DepthBuffer& depthBuffer,
              const vec& cameraPos,
              float cameraYaw,
              float cameraPitch)
    {
        drawShape(depthBuffer, basePrism, cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPrism,   cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPyramid, cameraPos, cameraYaw, cameraPitch);
    }
};

#endif // TOWER_HPP
