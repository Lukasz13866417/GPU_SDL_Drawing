#ifndef GPU_HPP
#define GPU_HPP
#include <iostream>
#include "../include/util.hpp"
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>

class _GPU;

class GPU{
    private:
        _GPU* pimpl;
    public:
        GPU();
        ~GPU();
        bool isInitialized();
        cl::Device& getDevice();
        cl::Platform& getPlatform();
        cl::CommandQueue& getQueue();
        cl::Context& getContext();

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
        void clear();
};

#endif // GPU_HPP