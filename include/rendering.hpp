#ifndef GPU_HPP
#define GPU_HPP
#include <iostream>
#include <optional>
#include <string>
#include "../include/util.hpp"
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>

#include "texture.hpp" // Include the new texture header

class _GPU;
class DepthBuffer; // Forward-declaration for friendship

class GPU{
    friend class DepthBuffer;
    private:
        _GPU* pimpl;
        Texture* getTexture(size_t id);
    public:
        GPU();
        ~GPU();
        bool isInitialized();
        cl::Device& getDevice();
        cl::Platform& getPlatform();
        cl::CommandQueue& getQueue();
        cl::Context& getContext();
        
        // Texture management
        size_t createTexture(int width, int height, const uint32_t* data);
};


void initGPU();
GPU& getGPU();
void deleteGPU();

class _DepthBuffer;

class DepthBuffer{
    private:
    _DepthBuffer* pimpl;       
    public:
        DepthBuffer(int scr_s, int scr_h, int scr_z);
        ~DepthBuffer();
        uint32_t *finishFrame();
        void enqueueDrawTriangle(const vec &a,const vec &b,const vec &c, int clr);
        void enqueueDrawTexturedTriangle(const vec &a, const vec &b, const vec &c, 
                                       const TexCoord &ta, const TexCoord &tb, const TexCoord &tc,
                                       size_t textureID);
        void clear();
};

#endif // GPU_HPP