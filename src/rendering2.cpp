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
#include "../include/rendering.hpp"
#include "../include/util.hpp"

class _GPU{
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

class _DepthBuffer {
    private:
        int n, maxx, maxy, scr_z;
        Uint32 *colorArr;
        float *depthArr;
        std::shared_ptr<cl::Buffer> depth, color; 
        std::shared_ptr<cl::Program> drawFunctions;
        std::shared_ptr<cl::Kernel> clearingKernel, drawingKernel;

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
            depth = std::make_shared<cl::Buffer>(getGPU().getContext(), CL_MEM_READ_WRITE, sizeof(float) * n);
            color = std::make_shared<cl::Buffer>(getGPU().getContext(), CL_MEM_READ_WRITE, sizeof(Uint32) * n);
            getGPU().getQueue().enqueueWriteBuffer(*depth, CL_TRUE, 0, sizeof(float) * n, depthArr);
            getGPU().getQueue().enqueueWriteBuffer(*color, CL_TRUE, 0, sizeof(Uint32) * n, colorArr);

            cl::Program::Sources sources;
           
            std::string sourceCode = getCode("../src/cl_scripts/script.cl");
            sources.push_back({sourceCode.c_str(), sourceCode.length()});

            cl::Program program(getGPU().getContext(), sources);
            program.build("-cl-std=CL3.0");

            // Create the OpenCL kernels
            clearingKernel = std::make_shared<cl::Kernel>(program,"clear");
            drawingKernel = std::make_shared<cl::Kernel>(program,"draw");
        }    

        void _drawTriangle(vec *a, vec *b, vec *c, int clr){
            if(triangleNormal(*a,*b,*c).z > 1){
                return;
            }
            if(isFirstDraw){
                assert(clearingKernel->setArg(0, *depth)==CL_SUCCESS);
                assert(clearingKernel->setArg(1, *color)==CL_SUCCESS);
                assert(clearingKernel->setArg(2, maxx)==CL_SUCCESS);
                cl::NDRange global_work_size(maxx+1, maxy+1);
                assert(getGPU().getQueue().enqueueNDRangeKernel(*clearingKernel, cl::NullRange, global_work_size, cl::NullRange)==CL_SUCCESS);
            }
        
            float x1 = a->x, y1 = -a->y, z1 = a->z;
            float x2 = b->x, y2 = -b->y, z2 = b->z;
            float x3 = c->x, y3 = -c->y, z3 = c->z;

            if(z1 < 10 && z2 < 10 && z3 < 10){
                return;
            }
            
        
            x1 *= (float)(scr_z) / abs(z1); x2 *= (float)(scr_z) / abs(z2); x3 *= (float)(scr_z) / abs(z3);
            y1 *= (float)(scr_z) / abs(z1); y2 *= (float)(scr_z) / abs(z2); y3 *= (float)(scr_z) / abs(z3); 

            int boxLeft = std::min({x1,x2,x3}), boxRight = std::max({x1,x2,x3}), boxTop = std::min({y1,y2,y3}), boxBottom = std::max({y1,y2,y3});
            if(boxRight < (-maxx/2) + 1 || boxLeft > (maxx/2) + 3 || boxTop > (maxy/2)+3 || boxBottom < (-maxy/2)+1){
                return;
            }
            boxLeft = std::max(boxLeft,(-maxx/2) + 1);
            boxRight = std::min(boxRight,(maxx/2) - 3);
            boxTop = std::max(boxTop,(-maxy/2) + 1);
            boxBottom = std::min(boxBottom,(maxy/2) - 3);
            if(boxLeft>=boxRight || boxTop>=boxBottom){
                return;
            }

            assert(drawingKernel->setArg(0, *depth)==CL_SUCCESS);
            assert(drawingKernel->setArg(1, *color)==CL_SUCCESS);

            cl_float3 v1 = {{x1, y1, 1.0f/z1}};
            cl_float3 v2 = {{x2, y2, 1.0f/z2}};
            cl_float3 v3 = {{x3, y3, 1.0f/z3}};

            assert(drawingKernel->setArg(2, 1.0f/z1)==CL_SUCCESS);
            assert(drawingKernel->setArg(3, 1.0f/z2)==CL_SUCCESS);
            assert(drawingKernel->setArg(4, 1.0f/z3)==CL_SUCCESS);

            assert(drawingKernel->setArg(5, clr)==CL_SUCCESS); 
            
           
            assert(drawingKernel->setArg(6, (maxx))==CL_SUCCESS); 
            assert(drawingKernel->setArg(7, (maxy))==CL_SUCCESS); 
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

    public:

        _DepthBuffer(int scr_w, int scr_h, int scr_z, GPU* gpu) : n((scr_w+1)*(scr_h+1)+1), maxx(scr_w), maxy(scr_h), scr_z(scr_z){
            colorArr = new Uint32[n];
            depthArr = new float[n];
            initOpenCL();
        }
        ~_DepthBuffer(){
            delete colorArr;
            delete depthArr;
        }
        Uint32* finishFrame() {
            isFirstDraw = true;
            getGPU().getQueue().enqueueReadBuffer(*color, CL_TRUE, 0, sizeof(Uint32) * n, colorArr);
            return colorArr;
        }

        void drawTriangle(vec &a, vec &b, vec &c, int clr){
            _drawTriangle(&a,&b,&c,clr);
        }
};

DepthBuffer::DepthBuffer(int scr_w, int scr_h, int scr_y){
    pimpl = new _DepthBuffer(scr_w,scr_h,scr_y,gpu);
}

DepthBuffer::~DepthBuffer(){
    delete pimpl;
}

Uint32* DepthBuffer::finishFrame() {
    return pimpl->finishFrame();
}

void DepthBuffer::drawTriangle(vec a, vec b, vec c, int clr){
    pimpl->drawTriangle(a,b,c,clr);
}
