// Buffer Usage Examples - Luke's Renderer (lr) namespace
#include "../include/buffer.hpp"
#include "../include/rendering.hpp"
#include "../include/log.hpp"
#include <vector>

using namespace lr;

void demonstrateBufferUsage() {
    // Initialize GPU and logging
    LOG_INIT();
    initGPU();
    
    // Example 1: AllPurposeBuffer - Most flexible, allows all operations
    LOG_INFO("=== AllPurposeBuffer Example ===");
    {
        std::vector<float> vertices = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};
        AllPurposeBuffer<float> vertexBuffer(6, vertices);
        
        // Can write from host
        std::vector<float> newVertices = {1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f};
        vertexBuffer.writeFrom(newVertices);
        
        // Can read to host
        std::vector<float> readBack;
        vertexBuffer.readTo(readBack);
        
        // Buffer can be used in OpenCL kernels for both reading and writing
        const cl::Buffer& clBuffer = vertexBuffer.getCLBuffer();
        // Pass clBuffer to your OpenCL kernel...
    }
    
    // Example 2: HostProducedBuffer - Host writes, GPU reads (common for vertex data)
    LOG_INFO("=== HostProducedBuffer Example ===");
    {
        HostProducedBuffer<float> vertexBuffer(1000); // 1000 vertices
        
        // Generate vertex data on CPU
        std::vector<float> vertices(1000);
        for (size_t i = 0; i < vertices.size(); ++i) {
            vertices[i] = static_cast<float>(i) * 0.1f;
        }
        
        // Upload to GPU
        vertexBuffer.writeFrom(vertices);
        
        // GPU can read this buffer in kernels, but host cannot read it back
        // This is enforced at compile time - attempting vertexBuffer.readTo(output) 
        // would cause a compilation error
    }
    
    // Example 3: GPUProducedBuffer - GPU writes, host reads (common for computation results)
    LOG_INFO("=== GPUProducedBuffer Example ===");
    {
        GPUProducedBuffer<int> resultBuffer(500); // Results from computation
        
        // GPU kernel would write results to this buffer
        // const cl::Buffer& clBuffer = resultBuffer.getCLBuffer();
        // kernel.setArg(0, clBuffer); // GPU can write to this
        
        // Host can read the results after GPU computation
        std::vector<int> results;
        resultBuffer.readTo(results);
        
        // Host cannot write to this buffer - compile-time error if attempted
        // resultBuffer.writeFrom(data); // ERROR: would not compile
    }
    
    // Example 4: ConstBuffer - Read-only data (common for lookup tables, constants)
    LOG_INFO("=== ConstBuffer Example ===");
    {
        std::vector<float> lookupTable = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
        ConstBuffer<float> constantBuffer(5, lookupTable);
        
        // This buffer is read-only for both host and GPU
        // Neither host nor GPU can write to it after creation
        // GPU kernels can only read from this buffer
        
        // Attempting to write would cause compile-time error:
        // constantBuffer.writeFrom(newData); // ERROR: would not compile
    }
    
    // Example 5: Using std::span for zero-copy buffer creation
    LOG_INFO("=== std::span Example ===");
    {
        float rawData[] = {1.0f, 2.0f, 3.0f, 4.0f};
        std::span<const float> dataSpan(rawData, 4);
        
        // Create buffer directly from raw memory without copying to vector first
        AllPurposeBuffer<float> buffer(4, dataSpan);
        
        std::vector<float> verification;
        buffer.readTo(verification);
        // verification now contains {1.0f, 2.0f, 3.0f, 4.0f}
    }
    
    // Example 6: Type safety - only trivially copyable types allowed
    LOG_INFO("=== Type Safety Example ===");
    {
        // These work fine:
        AllPurposeBuffer<int> intBuffer(10);
        AllPurposeBuffer<float> floatBuffer(10);
        AllPurposeBuffer<uint32_t> uintBuffer(10);
        
        // This would cause a compile-time error:
        // AllPurposeBuffer<std::string> stringBuffer(10); // ERROR: std::string not trivially copyable
        // AllPurposeBuffer<std::vector<int>> vectorBuffer(10); // ERROR: not trivially copyable
    }
    
    LOG_SUCCESS("Buffer usage examples completed!");
    deleteGPU();
}

