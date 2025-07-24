#ifndef GPU_HPP
#define GPU_HPP
#include <iostream>
#include <optional>
#include <string>
#include "../include/util.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>

// Forward declarations
class Texture;
struct TexCoord;

class _GPU;
class Renderer; // Forward-declaration for friendship

class GPU{
    friend class Renderer;
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

class _Renderer;

class Renderer{
    private:
    _Renderer* pimpl;       
    public:
        Renderer(int scr_s, int scr_h, int scr_z);
        ~Renderer();
        uint32_t *finishFrame();
        
        // Modern binning-based rendering methods only
        
        void clear();
        
        // Binning-based rendering methods - collect triangles and render tiles efficiently
        void startNewFrame();
        void submitTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx, int color);
            void submitTexturedTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx,
                                         const TexCoord& ta, const TexCoord& tb, const TexCoord& tc, const Texture& texture);
    void executeBinningPass();
    void executeFinishFrameTileBased();  // Render using tile-based approach after binning
    int getBinnedTriangleCount() const;
    
    // Camera management
    void setCamera(const Camera& camera);
    Camera& getCamera();
    const Camera& getCamera() const;
};

#endif // GPU_HPP