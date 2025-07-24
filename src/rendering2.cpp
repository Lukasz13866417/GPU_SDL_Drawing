#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cassert>
#include <optional>
#include <cstddef>
#include "../include/rendering.hpp"
#include "../include/texture.hpp" // For Texture and TexCoord definitions
#include "../include/util.hpp"
#include "../include/buffer.hpp" // ensure prototypes match

// Helper functions for buffer.hpp
cl::Context& gpuContext() {
    return getGPU().getContext();
}

cl::CommandQueue& gpuQueue() {
    return getGPU().getQueue();
}

class _GPU {
    friend class GPU; // Allow facade to access private members
    private:
        bool initialized = false;
        cl::Platform plat;
        cl::Context context;
        cl::CommandQueue queue;
        cl::Device device;

    public:
        bool isInitialized(){
            return initialized;
        }
        _GPU(){
            std::vector<cl::Platform> platforms;
            cl::Platform::get(&platforms);
            for (auto &p : platforms) {
                std::string platver = p.getInfo<CL_PLATFORM_VERSION>();
                if (platver.find("OpenCL 2.") != std::string::npos ||
                    platver.find("OpenCL 3.") != std::string::npos) {
                    plat = p;
                    break; // Exit after finding the first matching platform
                }
            }
            
            if (plat() == nullptr) { 
                std::cerr << "No OpenCL 2.0 or newer platform found.\n";
                throw std::runtime_error("No OpenCL 2.0 or newer platform found.");
            }
            std::vector<cl::Device> devices;
            plat.getDevices(CL_DEVICE_TYPE_GPU, &devices); // Prefer GPU devices
            if (devices.empty()) {
                std::cerr << "No GPU device found.\n";
                throw std::runtime_error("No GPU device found.");
            }

            device = devices.front(); // Use the first available device
            std::string deviceName;
            device.getInfo(CL_DEVICE_NAME, &deviceName);

            // Create a context and command queue
            context = cl::Context(device);
            queue = cl::CommandQueue(context, device);

            initialized = true;
        }
        cl::Device& getDevice(){
            return device;
        }
        cl::Platform& getPlatform(){
            return plat;
        }
        cl::CommandQueue& getQueue(){
            return queue;
        }
        cl::Context& getContext(){
            return context;
        }


};

GPU* gpu;


GPU::GPU(){
    pimpl = new _GPU();
}
GPU::~GPU(){
    delete pimpl;
}
bool GPU::isInitialized(){
    return pimpl->isInitialized();
}
cl::Device& GPU::getDevice(){
    return pimpl->getDevice();
}
cl::Platform& GPU::getPlatform(){
    return pimpl->getPlatform();
}
cl::CommandQueue& GPU::getQueue(){
    return pimpl->getQueue();
}
cl::Context& GPU::getContext(){
    return pimpl->getContext();
}



void initGPU(){
    gpu = new GPU();
}

GPU& getGPU(){
    return *gpu;
}

void deleteGPU(){
    delete gpu;
}

// Triangle metadata structure for CPU-side processing
struct TriangleMetadata {
    cl::Buffer* vertexBuffer;  // Pointer to vertex buffer
    int v0_idx, v1_idx, v2_idx;          // Vertex indices
    float tex_coords[6];                 // 6 texture coordinates (u0,v0,u1,v1,u2,v2)
    cl::Buffer* texture;                 // Texture buffer (null for solid color triangles)
    int tex_width, tex_height;           // Texture dimensions
    int color;                           // Solid color (used when texture is null)
    int triangle_id;                     // Unique triangle ID for this frame
};

// GPU-compatible triangle data structure (matches OpenCL TriangleData)
#pragma pack(push, 1)
struct GPUTriangleData {
    cl_mem vertexBuffer;     // OpenCL buffer handle
    int v0_idx, v1_idx, v2_idx;
    float tex_coords[6];
    cl_mem texture;          // OpenCL texture handle (0 for solid color)
    int tex_width, tex_height;
    int color;
    int triangle_id;
};
#pragma pack(pop)

// Debug the actual sizes at compile time
static_assert(sizeof(cl_mem) == 8, "cl_mem should be 8 bytes on 64-bit systems");
static_assert(sizeof(int) == 4, "int should be 4 bytes");
static_assert(sizeof(float) == 4, "float should be 4 bytes");

// Compile-time verification that GPUTriangleData has the expected packed size
// Actual layout: cl_mem (8) + 3*int (12) + 6*float (24) + cl_mem (8) + 2*int (8) = 60 bytes of data + 8 bytes padding = 68 bytes
static_assert(sizeof(GPUTriangleData) == 68, "GPUTriangleData must be exactly 68 bytes to match OpenCL TriangleData");

// Constants matching the OpenCL binning.cl definitions
const int TILE_SIZE = 32;  // Each tile is 32x32 pixels
const int MAX_TRIANGLES_PER_TILE = 256;  // Maximum triangles that can be assigned to a tile

// Binner class - manages triangle collection and binning for tile-based rendering
class Binner {
private:
    std::vector<TriangleMetadata> frameTriangles;
    lr::AllPurposeBuffer<GPUTriangleData>* triangleBuffer;
    lr::AllPurposeBuffer<uint8_t>* tileBuffer;  // Using uint8_t for raw bytes
    std::shared_ptr<cl::Kernel> binTrianglesKernel, clearTilesKernel, renderTileKernel;
    
    int maxTriangles;
    int screenWidth, screenHeight;
    int tilesPerRow, tilesPerColumn, totalTiles;
    int nextTriangleId;
    
    static constexpr int TILE_SIZE = 32;
    static constexpr int MAX_TRIANGLES_PER_TILE = 256;
    
    // Calculate size of TileData structure (triangle_ids array + triangle_count)
    size_t getTileDataSize() const {
        return MAX_TRIANGLES_PER_TILE * sizeof(int) + sizeof(int);
    }

public:
    Binner(int screen_w, int screen_h, int max_triangles = 10000) 
        : maxTriangles(max_triangles), screenWidth(screen_w), screenHeight(screen_h), nextTriangleId(0) {
        
        // Calculate tile grid dimensions
        tilesPerRow = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
        tilesPerColumn = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;
        totalTiles = tilesPerRow * tilesPerColumn;
        
        LOG_DEBUG("Initializing Binner: " + std::to_string(tilesPerRow) + "x" + std::to_string(tilesPerColumn) + " tiles (" + std::to_string(totalTiles) + " total)");
        
        // Create buffers - triangle buffer will be created dynamically in runBinningPass
        triangleBuffer = nullptr;
        tileBuffer = new lr::AllPurposeBuffer<uint8_t>(totalTiles * getTileDataSize());
        
        frameTriangles.reserve(maxTriangles);
    }
    
    ~Binner() {
        if (triangleBuffer) delete triangleBuffer;
        delete tileBuffer;
    }
    
    void initKernels(cl::Program& program) {
        binTrianglesKernel = std::make_shared<cl::Kernel>(program, "binTriangles");
        clearTilesKernel = std::make_shared<cl::Kernel>(program, "clearTiles");
        renderTileKernel = std::make_shared<cl::Kernel>(program, "renderTile");
        
        LOG_DEBUG("Binner kernels initialized successfully");
    }
    
    // Add a solid color triangle to the frame
    void addTriangle(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx, int color) {
        if (frameTriangles.size() >= maxTriangles) {
            LOG_ERR("Maximum triangles per frame exceeded!");
            return;
        }
        
        TriangleMetadata triangle = {};
        triangle.vertexBuffer = const_cast<cl::Buffer*>(&vertexBuffer.getCLBuffer());
        triangle.v0_idx = v0_idx;
        triangle.v1_idx = v1_idx;
        triangle.v2_idx = v2_idx;
        triangle.texture = nullptr;  // No texture
        triangle.color = color;
        triangle.triangle_id = nextTriangleId++;
        
        frameTriangles.push_back(triangle);
    }
    
    // Add a textured triangle to the frame
    void addTexturedTriangle(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx,
                           const TexCoord& ta, const TexCoord& tb, const TexCoord& tc, const Texture& texture) {
        if (frameTriangles.size() >= maxTriangles) {
            LOG_ERR("Maximum triangles per frame exceeded!");
            return;
        }
        
        TriangleMetadata triangle = {};
        triangle.vertexBuffer = const_cast<cl::Buffer*>(&vertexBuffer.getCLBuffer());
        triangle.v0_idx = v0_idx;
        triangle.v1_idx = v1_idx;
        triangle.v2_idx = v2_idx;
        triangle.tex_coords[0] = ta.u; triangle.tex_coords[1] = ta.v;
        triangle.tex_coords[2] = tb.u; triangle.tex_coords[3] = tb.v;
        triangle.tex_coords[4] = tc.u; triangle.tex_coords[5] = tc.v;
        triangle.texture = const_cast<cl::Buffer*>(&texture.getCLBuffer());
        triangle.tex_width = texture.getWidth();
        triangle.tex_height = texture.getHeight();
        triangle.triangle_id = nextTriangleId++;
        
        frameTriangles.push_back(triangle);
    }
    
    // Upload triangle data to GPU and run binning pass
    void runBinningPass() {
        if (frameTriangles.empty()) {
            LOG_DEBUG("No triangles to bin - skipping binning pass");
            return;
        }
        
        LOG_DEBUG("Running binning pass for " + std::to_string(frameTriangles.size()) + " triangles");
        
        // Convert CPU triangle metadata to GPU format
        std::vector<GPUTriangleData> gpuTriangles;
        gpuTriangles.reserve(frameTriangles.size());
        
        for (const auto& triangle : frameTriangles) {
            GPUTriangleData gpuTriangle = {};
            gpuTriangle.vertexBuffer = triangle.vertexBuffer->get();  // Get raw cl_mem handle
            gpuTriangle.v0_idx = triangle.v0_idx;
            gpuTriangle.v1_idx = triangle.v1_idx;
            gpuTriangle.v2_idx = triangle.v2_idx;
            for (int i = 0; i < 6; i++) {
                gpuTriangle.tex_coords[i] = triangle.tex_coords[i];
            }
            gpuTriangle.texture = triangle.texture ? triangle.texture->get() : 0;  // 0 for solid color
            gpuTriangle.tex_width = triangle.tex_width;
            gpuTriangle.tex_height = triangle.tex_height;
            gpuTriangle.color = triangle.color;
            gpuTriangle.triangle_id = triangle.triangle_id;
            
            gpuTriangles.push_back(gpuTriangle);
        }
        
        // Create triangle buffer with correct size for this frame
        if (triangleBuffer) delete triangleBuffer;
        triangleBuffer = new lr::AllPurposeBuffer<GPUTriangleData>(gpuTriangles.size(), gpuTriangles);
        LOG_DEBUG("Created triangle buffer with " + std::to_string(gpuTriangles.size()) + " triangles");
        
        // Clear tile data
        assert(clearTilesKernel->setArg(0, tileBuffer->getCLBuffer()) == CL_SUCCESS);
        assert(clearTilesKernel->setArg(1, totalTiles) == CL_SUCCESS);
        
        cl::NDRange clearWorkSize(totalTiles);
        assert(getGPU().getQueue().enqueueNDRangeKernel(*clearTilesKernel, cl::NullRange, clearWorkSize, cl::NullRange) == CL_SUCCESS);
        
        // Run binning kernel
        assert(binTrianglesKernel->setArg(0, triangleBuffer->getCLBuffer()) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(1, (int)gpuTriangles.size()) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(2, tileBuffer->getCLBuffer()) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(3, screenWidth) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(4, screenHeight) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(5, tilesPerRow) == CL_SUCCESS);
        assert(binTrianglesKernel->setArg(6, tilesPerColumn) == CL_SUCCESS);
        
        cl::NDRange binWorkSize(gpuTriangles.size());
        assert(getGPU().getQueue().enqueueNDRangeKernel(*binTrianglesKernel, cl::NullRange, binWorkSize, cl::NullRange) == CL_SUCCESS);
        
        LOG_DEBUG("Binning pass completed");
    }
    
    // Reset for next frame
    void startNewFrame() {
        frameTriangles.clear();
        nextTriangleId = 0;
        LOG_DEBUG("Started new frame - triangle list cleared");
    }
    
    // Provide access to tile and triangle data for _Renderer to use in tile-based rendering
    const lr::AllPurposeBuffer<uint8_t>* getTileBuffer() const { return tileBuffer; }
    const lr::AllPurposeBuffer<GPUTriangleData>* getTriangleBuffer() const { return triangleBuffer; }
    std::shared_ptr<cl::Kernel> getRenderTileKernel() const { return renderTileKernel; }
    
    // Get statistics
    int getTriangleCount() const { return frameTriangles.size(); }
    int getTileCount() const { return totalTiles; }
    int getTilesPerRow() const { return tilesPerRow; }
    int getTilesPerColumn() const { return tilesPerColumn; }
};

class _Renderer {
    private:
        int32_t n, maxx, maxy;
        int32_t scr_z;
        uint32_t *colorArr;
        std::shared_ptr<cl::Buffer> depth, color, globalData; 
        std::shared_ptr<cl::Program> drawFunctions;
        std::shared_ptr<cl::Kernel> clearingKernel;  // Only clearing kernel still needed
        
        // Binner for tile-based rendering
        std::unique_ptr<Binner> binner;
        
        // Camera for 3D transformations
        Camera camera;

        bool isFirstDraw = true;

        std::string getCode(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open kernel source file: " << filename << std::endl;
                exit(1);
            }
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        std::string getCombinedKernelCode() {
            // Load and combine all OpenCL files
            std::string combined = "";
            
            // Common types and utilities first
            combined += getCode("../src/cl_scripts/common.cl");
            combined += "\n\n";
            
            // Rasterization kernels
            combined += getCode("../src/cl_scripts/rasterization.cl");
            combined += "\n\n";
            
            // Binning kernels
            combined += getCode("../src/cl_scripts/binning.cl");
            combined += "\n\n";
            
            return combined;
        }


        void initOpenCL() {
            assert(getGPU().isInitialized());

            cl::Program::Sources sources;
           
            std::string sourceCode = getCombinedKernelCode();
            sources.push_back({sourceCode.c_str(), sourceCode.length()});

            cl::Program program(getGPU().getContext(), sources);
            program.build("-cl-std=CL3.0");

            
            uint32_t addressBits = getGPU().getDevice().getInfo<CL_DEVICE_ADDRESS_BITS>();

            cl::Buffer globalDataSizeBuffer(getGPU().getContext(),CL_MEM_READ_WRITE,sizeof(uint32_t));
            uint32_t globalDataSize;
            cl::Kernel globalDataSizeKernel(program,"getGlobalDataSize");

            cl::NDRange global_work_size(1);

            assert(globalDataSizeKernel.setArg(0, globalDataSizeBuffer) == CL_SUCCESS);
            assert(getGPU().getQueue().enqueueNDRangeKernel(globalDataSizeKernel,cl::NullRange,global_work_size,cl::NullRange) == CL_SUCCESS);
            assert(getGPU().getQueue().enqueueReadBuffer(globalDataSizeBuffer, CL_TRUE, 0, sizeof(uint32_t), &globalDataSize) == CL_SUCCESS);
            std::cout<<"GPU address bits: "<<addressBits<<", GPU global data struct size: "<<globalDataSize<<std::endl;
            getGPU().getQueue().flush();


            depth = std::make_shared<cl::Buffer>(getGPU().getContext(), CL_MEM_READ_WRITE, sizeof(float) * n);
            color = std::make_shared<cl::Buffer>(getGPU().getContext(), CL_MEM_READ_WRITE, sizeof(uint32_t) * n);
            globalData = std::make_shared<cl::Buffer>(getGPU().getContext(),CL_MEM_READ_ONLY,globalDataSize); // wiele

            cl::Kernel globalDataKernel(program,"makeGlobalData"); 
            assert(globalDataKernel.setArg(0, *depth) == CL_SUCCESS); 
            assert(globalDataKernel.setArg(1, *color) == CL_SUCCESS); 
            assert(globalDataKernel.setArg(2, *globalData) == CL_SUCCESS); 
            assert(globalDataKernel.setArg(3, maxx) == CL_SUCCESS); 
            assert(globalDataKernel.setArg(4, maxy) == CL_SUCCESS); 
 
            assert(getGPU().getQueue().enqueueNDRangeKernel(globalDataKernel,cl::NullRange,global_work_size,cl::NullRange) == CL_SUCCESS);
            getGPU().getQueue().flush();


            // Create the OpenCL kernels
            clearingKernel = std::make_shared<cl::Kernel>(program,"clear");
            assert(clearingKernel->setArg(0, *depth) == CL_SUCCESS);
            assert(clearingKernel->setArg(1, *color) == CL_SUCCESS);
            assert(clearingKernel->setArg(2, maxx) == CL_SUCCESS); 

            // Old drawing kernels removed - only binning kernels used now 
            
            // Initialize binner kernels
            binner->initKernels(program);
        }    


        // Old direct triangle drawing method removed - use binning system instead

        void flushTriangles(){

        }

    public:

        _Renderer(int scr_w, int scr_h, int scr_z) : n((scr_w+1)*(scr_h+1)+1), maxx(scr_w), maxy(scr_h), scr_z(scr_z){
            colorArr = new uint32_t[n];
            
            // Initialize binner
            binner = std::make_unique<Binner>(scr_w, scr_h);
            
            initOpenCL();
        }
        ~_Renderer(){
            delete colorArr;
        }
        uint32_t* finishFrame() {
            isFirstDraw = true;
            getGPU().getQueue().flush();
            getGPU().getQueue().enqueueReadBuffer(*color, CL_TRUE, 0, sizeof(uint32_t) * n, colorArr);
            return colorArr;
        }

        void clear(){
            cl::NDRange global_work_size(maxx+1, maxy+1);
            assert(getGPU().getQueue().enqueueNDRangeKernel(*clearingKernel, cl::NullRange, global_work_size, cl::NullRange)==CL_SUCCESS);
        }
        
        // Binner interface methods
        void startNewFrame() {
            binner->startNewFrame();
        }
        
        void submitTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx, int color) {
            binner->addTriangle(vertexBuffer, v0_idx, v1_idx, v2_idx, color);
        }
        
        void submitTexturedTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx,
                                            const TexCoord& ta, const TexCoord& tb, const TexCoord& tc, const Texture& texture) {
            binner->addTexturedTriangle(vertexBuffer, v0_idx, v1_idx, v2_idx, ta, tb, tc, texture);
        }
        
        void executeBinningPass() {
            binner->runBinningPass();
        }
        
        // Execute tile-based rendering using the binned triangle data
        void executeFinishFrameTileBased() {
            if (!binner || binner->getTriangleCount() == 0) {
                LOG_DEBUG("No triangles to render with tile-based approach");
                return;
            }
            
            LOG_DEBUG("Starting tile-based rendering for " + std::to_string(binner->getTilesPerRow()) + 
                      "x" + std::to_string(binner->getTilesPerColumn()) + " tiles");
            
            auto renderTileKernel = binner->getRenderTileKernel();
            
            // Render each tile using the renderTile kernel
            for (int tile_y = 0; tile_y < binner->getTilesPerColumn(); tile_y++) {
                for (int tile_x = 0; tile_x < binner->getTilesPerRow(); tile_x++) {
                    // Set kernel arguments for renderTile
                    assert(renderTileKernel->setArg(0, *depth) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(1, *color) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(2, maxx) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(3, maxy) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(4, binner->getTileBuffer()->getCLBuffer()) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(5, binner->getTriangleBuffer()->getCLBuffer()) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(6, tile_x) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(7, tile_y) == CL_SUCCESS);
                    assert(renderTileKernel->setArg(8, binner->getTilesPerRow()) == CL_SUCCESS);
                    
                    // Launch kernel for this tile (TILE_SIZE x TILE_SIZE work items)
                    cl::NDRange tileWorkSize(TILE_SIZE, TILE_SIZE);
                    assert(getGPU().getQueue().enqueueNDRangeKernel(*renderTileKernel, cl::NullRange, tileWorkSize, cl::NullRange) == CL_SUCCESS);
                }
            }
            
            // Wait for all tile rendering to complete
            getGPU().getQueue().finish();
            
            LOG_DEBUG("Tile-based rendering completed");
        }
        
        int getBinnedTriangleCount() const {
            return binner->getTriangleCount();
        }
        
        // Camera management
        void setCamera(const Camera& camera) {
            this->camera = camera;
        }
        
        Camera& getCamera() {
            return camera;
        }
        
        const Camera& getCamera() const {
            return camera;
        }

};

Renderer::Renderer(int scr_w, int scr_h, int scr_y){
    pimpl = new _Renderer(scr_w,scr_h,scr_y);
}

Renderer::~Renderer(){
    delete pimpl;
}

uint32_t* Renderer::finishFrame() {
    return pimpl->finishFrame();
}

// Old triangle drawing wrapper methods removed - use binning system instead

void Renderer::clear(){
    pimpl->clear();
}

// Binner interface methods
void Renderer::startNewFrame() {
    pimpl->startNewFrame();
}

void Renderer::submitTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx, int color) {
    pimpl->submitTriangleForBinning(vertexBuffer, v0_idx, v1_idx, v2_idx, color);
}

void Renderer::submitTexturedTriangleForBinning(const lr::BaseBuffer<vec>& vertexBuffer, int v0_idx, int v1_idx, int v2_idx,
                                              const TexCoord& ta, const TexCoord& tb, const TexCoord& tc, const Texture& texture) {
    pimpl->submitTexturedTriangleForBinning(vertexBuffer, v0_idx, v1_idx, v2_idx, ta, tb, tc, texture);
}

void Renderer::executeBinningPass() {
    pimpl->executeBinningPass();
}

void Renderer::executeFinishFrameTileBased() {
    pimpl->executeFinishFrameTileBased();
}

int Renderer::getBinnedTriangleCount() const {
    return pimpl->getBinnedTriangleCount();
}

void Renderer::setCamera(const Camera& camera) {
    pimpl->setCamera(camera);
}

Camera& Renderer::getCamera() {
    return pimpl->getCamera();
}

const Camera& Renderer::getCamera() const {
    return pimpl->getCamera();
}
