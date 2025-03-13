#ifndef TOWER_HPP
#define TOWER_HPP

#include "shape3d.hpp"

struct Tower {
    Shape3D baseCuboid;
    Shape3D hexPrism;
    Shape3D hexPyramid;
    float rotationSpeed;  
    vec position;

    Tower(float cuboidHeight, float prismHeight, float pyramidHeight,
          float radius, float spinSpeed, const vec& pos)
        : rotationSpeed(spinSpeed), position(pos)
    {
        baseCuboid = createCuboid(radius * 1.2f, radius * 1.2f, cuboidHeight, fromRgb(100,100,100));
        hexPrism   = createPrism(12,radius, prismHeight, fromRgb(51,51,51));
        hexPyramid = createPyramid(12,radius, pyramidHeight, fromRgb(100,50,50));

        for (auto &v : hexPrism.vertices) {
            v.y += cuboidHeight + 1;
        }
        for (auto &v : hexPyramid.vertices) {
            v.y += (cuboidHeight + prismHeight + 1);
        }

        for (auto &v : baseCuboid.vertices)  { v = v + pos; }
        for (auto &v : hexPrism.vertices)    { v = v + pos; }
        for (auto &v : hexPyramid.vertices)  { v = v + pos; }
    }

    void update() {
        /*for (auto &v : baseCuboid.vertices) {
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
        drawShape(depthBuffer, baseCuboid, cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPrism,   cameraPos, cameraYaw, cameraPitch);
        drawShape(depthBuffer, hexPyramid, cameraPos, cameraYaw, cameraPitch);
    }
};

#endif // TOWER_HPP
