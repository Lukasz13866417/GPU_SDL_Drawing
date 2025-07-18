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
#include "../include/rendering.hpp"
#include "../include/util.hpp"
#include <map>

class _GPU {
    friend class GPU; // Allow facade to access private members
    private:
        bool initialized = false;
        cl::Platform plat;
        cl::Context context;
        cl::CommandQueue queue;
        cl::Device device;
        std::map<size_t, std::unique_ptr<Texture>> texture_map;
        size_t next_texture_id = 0;

        Texture* getTexture(size_t id) {
            auto it = texture_map.find(id);
            if (it != texture_map.end()) {
                return it->second.get();
            }
            return nullptr;
        }

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

        size_t createTexture(int width, int height, const uint32_t* data) {
            size_t id = next_texture_id++;
            // Use new Texture(...) directly as _GPU is a friend
            texture_map[id] = std::unique_ptr<Texture>(new Texture(width, height, data));
            return id;
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

size_t GPU::createTexture(int width, int height, const uint32_t* data) {
    return pimpl->createTexture(width, height, data);
}

Texture* GPU::getTexture(size_t id) {
    return pimpl->getTexture(id);
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

class _DepthBuffer {
    private:
        int32_t n, maxx, maxy;
        int32_t scr_z;
        uint32_t *colorArr;
        std::shared_ptr<cl::Buffer> depth, color, globalData; 
        std::shared_ptr<cl::Program> drawFunctions;
        std::shared_ptr<cl::Kernel> clearingKernel, drawingKernel, texturedDrawingKernel;

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


        void initOpenCL() {
            assert(getGPU().isInitialized());

            cl::Program::Sources sources;
           
            std::string sourceCode = getCode("../src/cl_scripts/script.cl");
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

            drawingKernel = std::make_shared<cl::Kernel>(program,"draw");
            assert(drawingKernel->setArg(0, *depth) == CL_SUCCESS);
            assert(drawingKernel->setArg(1, *color) == CL_SUCCESS);
            assert(drawingKernel->setArg(2, maxx) == CL_SUCCESS); 
            assert(drawingKernel->setArg(3, maxy) == CL_SUCCESS);
            
            texturedDrawingKernel = std::make_shared<cl::Kernel>(program,"drawTextured");
            assert(texturedDrawingKernel->setArg(0, *depth) == CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(1, *color) == CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(2, maxx) == CL_SUCCESS); 
            assert(texturedDrawingKernel->setArg(3, maxy) == CL_SUCCESS); 
        }    


        void _enqueueDrawTriangle(const vec &a, const vec &b, const vec &c, int clr){
            if(triangleNormal(a,b,c).z > 1){
                return;
            }
            float x1 = a.x, y1 = -a.y, z1 = a.z;
            float x2 = b.x, y2 = -b.y, z2 = b.z;
            float x3 = c.x, y3 = -c.y, z3 = c.z;

            // Cull the triangle if any vertex is too close to or behind the camera's near plane.
            if(z1 < 10 || z2 < 10 || z3 < 10){
                return;
            }
            
        
            x1 *= (float)(scr_z) / abs(z1); x2 *= (float)(scr_z) / abs(z2); x3 *= (float)(scr_z) / abs(z3);
            y1 *= (float)(scr_z) / abs(z1); y2 *= (float)(scr_z) / abs(z2); y3 *= (float)(scr_z) / abs(z3); 

            int boxLeft = std::min({x1,x2,x3}), boxRight = std::max({x1,x2,x3}), boxTop = std::min({y1,y2,y3}), boxBottom = std::max({y1,y2,y3});
            if(boxRight < (-maxx/2) + 1 || boxLeft > (maxx/2) + 3 || boxTop > (maxy/2)+3 || boxBottom < (-maxy/2)+1){
                return;
            }
            boxLeft = std::max(boxLeft,(-(int32_t)maxx/2) + 1);
            boxRight = std::min(boxRight,((int32_t)maxx/2) - 3);
            boxTop = std::max(boxTop,(-(int32_t)maxy/2) + 1);
            boxBottom = std::min(boxBottom,((int32_t)maxy/2) - 3);
            if(boxLeft>=boxRight || boxTop>=boxBottom){
                return;
            }


            cl_float3 v1 = {{x1, y1, 1.0f/z1}};
            cl_float3 v2 = {{x2, y2, 1.0f/z2}};
            cl_float3 v3 = {{x3, y3, 1.0f/z3}};

            assert(drawingKernel->setArg(4, 1.0f/z1)==CL_SUCCESS);
            assert(drawingKernel->setArg(5, 1.0f/z2)==CL_SUCCESS);
            assert(drawingKernel->setArg(6, 1.0f/z3)==CL_SUCCESS);

            assert(drawingKernel->setArg(7, clr)==CL_SUCCESS); 
           
            assert(drawingKernel->setArg(8, (boxLeft))==CL_SUCCESS); 
            assert(drawingKernel->setArg(9, (boxTop))==CL_SUCCESS); 
            float inv = 1.0f/(x1*y2 - x1*y3 - y1*x2 + y1*x3 + x2*y3 - y2*x3);
            //assert(drawingKernel->setArg(10, inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(10, (y3 - y1)*inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(11, (y1 - y2)*inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(12, (x1 - x3)*inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(13, (x2 - x1)*inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(14, (-x1*y3 + y1*x3)*inv)==CL_SUCCESS); 
            assert(drawingKernel->setArg(15, (x1*y2 - y1*x2)*inv)==CL_SUCCESS); 
            
            size_t global_work_size_x = (boxRight - boxLeft + 2)/2;
            size_t global_work_size_y = (boxBottom - boxTop + 2)/2;
            cl::NDRange global_work_size(global_work_size_x, global_work_size_y);
            assert(getGPU().getQueue().enqueueNDRangeKernel(*drawingKernel, cl::NullRange, global_work_size, cl::NullRange)==CL_SUCCESS);

            
            isFirstDraw = false;
        }

        void flushTriangles(){

        }

    public:

        _DepthBuffer(int scr_w, int scr_h, int scr_z, GPU* gpu) : n((scr_w+1)*(scr_h+1)+1), maxx(scr_w), maxy(scr_h), scr_z(scr_z){
            colorArr = new uint32_t[n];
            initOpenCL();
        }
        ~_DepthBuffer(){
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

        void enqueueDrawTriangle(const vec &a, const vec &b, const vec &c, int clr){
            _enqueueDrawTriangle(a,b,c,clr);
        }

        void _enqueueDrawTexturedTriangle(const vec &a, const vec &b, const vec &c, 
                                        const TexCoord &ta, const TexCoord &tb, const TexCoord &tc,
                                        Texture& texture) {
            
            if(triangleNormal(a,b,c).z > 1){
                return;
            }
            float x1 = a.x, y1 = -a.y, z1 = a.z;
            float x2 = b.x, y2 = -b.y, z2 = b.z;
            float x3 = c.x, y3 = -c.y, z3 = c.z;

            // Cull the triangle if any vertex is too close to or behind the camera's near plane.
            // This is a simple fix to prevent division by zero in the perspective-correct calculations.
            if(z1 < 10 || z2 < 10 || z3 < 10){
                return;
            }
            
            x1 *= (float)(scr_z) / abs(z1); x2 *= (float)(scr_z) / abs(z2); x3 *= (float)(scr_z) / abs(z3);
            y1 *= (float)(scr_z) / abs(z1); y2 *= (float)(scr_z) / abs(z2); y3 *= (float)(scr_z) / abs(z3); 

            int boxLeft = std::min({x1,x2,x3}), boxRight = std::max({x1,x2,x3}), boxTop = std::min({y1,y2,y3}), boxBottom = std::max({y1,y2,y3});
            if(boxRight < (-maxx/2) + 1 || boxLeft > (maxx/2) + 3 || boxTop > (maxy/2)+3 || boxBottom < (-maxy/2)+1){
                return;
            }
            boxLeft = std::max(boxLeft,(-(int32_t)maxx/2) + 1);
            boxRight = std::min(boxRight,((int32_t)maxx/2) - 3);
            boxTop = std::max(boxTop,(-(int32_t)maxy/2) + 1);
            boxBottom = std::min(boxBottom,((int32_t)maxy/2) - 3);
            if(boxLeft>=boxRight || boxTop>=boxBottom){
                return;
            }

            float w1 = 1.0f/z1;
            float w2 = 1.0f/z2;
            float w3 = 1.0f/z3;

            // Set kernel arguments for textured triangle
            assert(texturedDrawingKernel->setArg(4, w1)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(5, w2)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(6, w3)==CL_SUCCESS);
            
            assert(texturedDrawingKernel->setArg(7, texture.getBuffer())==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(8, texture.getWidth())==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(9, texture.getHeight())==CL_SUCCESS);
            
            // Pass perspective-correct texture coordinates (u/z, v/z)
            assert(texturedDrawingKernel->setArg(10, ta.u * w1)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(11, ta.v * w1)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(12, tb.u * w2)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(13, tb.v * w2)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(14, tc.u * w3)==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(15, tc.v * w3)==CL_SUCCESS);
           
            assert(texturedDrawingKernel->setArg(16, (boxLeft))==CL_SUCCESS);
            assert(texturedDrawingKernel->setArg(17, (boxTop))==CL_SUCCESS);
            
            // Barycentric coordinate calculation parameters
            float inv = 1.0f/(x1*y2 - x1*y3 - y1*x2 + y1*x3 + x2*y3 - y2*x3);
            assert(texturedDrawingKernel->setArg(18, (y3 - y1)*inv)==CL_SUCCESS);  // dx1
            assert(texturedDrawingKernel->setArg(19, (y1 - y2)*inv)==CL_SUCCESS);  // dx2
            assert(texturedDrawingKernel->setArg(20, (x1 - x3)*inv)==CL_SUCCESS);  // dy1
            assert(texturedDrawingKernel->setArg(21, (x2 - x1)*inv)==CL_SUCCESS);  // dy2
            assert(texturedDrawingKernel->setArg(22, (-x1*y3 + y1*x3)*inv)==CL_SUCCESS);  // lambda1
            assert(texturedDrawingKernel->setArg(23, (x1*y2 - y1*x2)*inv)==CL_SUCCESS);   // lambda2
            
            size_t global_work_size_x = (boxRight - boxLeft + 2)/2;
            size_t global_work_size_y = (boxBottom - boxTop + 2)/2;
            cl::NDRange global_work_size(global_work_size_x, global_work_size_y);
            assert(getGPU().getQueue().enqueueNDRangeKernel(*texturedDrawingKernel, cl::NullRange, global_work_size, cl::NullRange)==CL_SUCCESS);

            isFirstDraw = false;
        }
};

DepthBuffer::DepthBuffer(int scr_w, int scr_h, int scr_y){
    pimpl = new _DepthBuffer(scr_w,scr_h,scr_y,gpu);
}

DepthBuffer::~DepthBuffer(){
    delete pimpl;
}

uint32_t* DepthBuffer::finishFrame() {
    return pimpl->finishFrame();
}

void DepthBuffer::enqueueDrawTriangle(const vec &a, const vec &b, const vec &c, int clr){
    pimpl->enqueueDrawTriangle(a,b,c,clr);
}

void DepthBuffer::enqueueDrawTexturedTriangle(const vec &a, const vec &b, const vec &c, 
                                            const TexCoord &ta, const TexCoord &tb, const TexCoord &tc,
                                            size_t textureID) {
    // Look up the texture using the now-private getTexture method
    Texture* texture = getGPU().getTexture(textureID);
    if (texture) {
        pimpl->_enqueueDrawTexturedTriangle(a, b, c, ta, tb, tc, *texture);
    }
    // Silently fail if texture ID is invalid
}

void DepthBuffer::clear(){
    pimpl->clear();
}
