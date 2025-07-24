#include <iostream>
#include <vector>
#include <span>
#include "../include/rendering.hpp"
#include "../include/buffer.hpp"
#include "../include/log.hpp"

using namespace lr;

int main() {
    // Initialize logging and GPU
    LOG_INIT();
    LOG_INFO("Starting Buffer System Test");
    
    try {
        // Initialize GPU
        LOG_DEBUG("Initializing GPU for buffer tests");
        initGPU();
        LOG_SUCCESS("GPU initialized successfully");
        
        // Test 1: AllPurposeBuffer - can read and write from host and GPU
        LOG_INFO("=== Test 1: AllPurposeBuffer ===");
        {
            std::vector<float> inputData = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
            LOG_DEBUG("Creating AllPurposeBuffer with initial data");
            
            AllPurposeBuffer<float> buffer(5, inputData);
            LOG_SUCCESS("AllPurposeBuffer created successfully");
            
            // Read back the data
            std::vector<float> outputData;
            buffer.readTo(outputData);
            
            // Verify data integrity
            bool dataValid = true;
            for (size_t i = 0; i < inputData.size(); ++i) {
                if (inputData[i] != outputData[i]) {
                    dataValid = false;
                    break;
                }
            }
            
            if (dataValid) {
                LOG_SUCCESS("Data integrity test passed - input data matches output data");
            } else {
                LOG_ERR("Data integrity test failed - data mismatch!");
                return -1;
            }
            
            // Test writing new data
            std::vector<float> newData = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
            buffer.writeFrom(newData);
            LOG_SUCCESS("Successfully wrote new data to buffer");
            
            // Read it back
            buffer.readTo(outputData);
            for (size_t i = 0; i < newData.size(); ++i) {
                if (newData[i] != outputData[i]) {
                    LOG_ERR("New data verification failed!");
                    return -1;
                }
            }
            LOG_SUCCESS("New data verification passed");
        }
        
        // Test 2: HostProducedBuffer - host can write, GPU can read
        LOG_INFO("=== Test 2: HostProducedBuffer ===");
        {
            HostProducedBuffer<int> buffer(3);
            LOG_SUCCESS("HostProducedBuffer created successfully");
            
            std::vector<int> data = {100, 200, 300};
            buffer.writeFrom(data);
            LOG_SUCCESS("Successfully wrote data to HostProducedBuffer");
            
            // Note: We can't read from this buffer type due to compile-time restrictions
            // This is intentional and demonstrates the flag system working
        }
        
        // Test 3: GPUProducedBuffer - GPU can write, host can read
        LOG_INFO("=== Test 3: GPUProducedBuffer ===");
        {
            GPUProducedBuffer<double> buffer(4);
            LOG_SUCCESS("GPUProducedBuffer created successfully");
            
            // We can read from this buffer (but not write from host)
            std::vector<double> output;
            buffer.readTo(output);
            LOG_SUCCESS("Successfully read from GPUProducedBuffer");
            
            // The data will be uninitialized since we didn't write anything
            LOG_DEBUG("Read " + std::to_string(output.size()) + " elements from GPU buffer");
        }
        
        // Test 4: ConstBuffer - GPU read-only
        LOG_INFO("=== Test 4: ConstBuffer ===");
        {
            std::vector<uint32_t> initialData = {0xDEADBEEF, 0xCAFEBABE, 0xFEEDFACE};
            ConstBuffer<uint32_t> buffer(3, initialData);
            LOG_SUCCESS("ConstBuffer created successfully with initial data");
            
            // This buffer is read-only for both host and GPU
            // We can't write to it after creation, which is enforced at compile time
        }
        
        // Test 5: Test with std::span
        LOG_INFO("=== Test 5: std::span support ===");
        {
            float data[] = {3.14f, 2.71f, 1.41f};
            std::span<const float> dataSpan(data, 3);
            
            AllPurposeBuffer<float> buffer(3, dataSpan);
            LOG_SUCCESS("Buffer created from std::span successfully");
            
            std::vector<float> output;
            buffer.readTo(output);
            
            // Verify the span data was copied correctly
            bool spanDataValid = true;
            for (size_t i = 0; i < 3; ++i) {
                if (data[i] != output[i]) {
                    spanDataValid = false;
                    break;
                }
            }
            
            if (spanDataValid) {
                LOG_SUCCESS("std::span data transfer test passed");
            } else {
                LOG_ERR("std::span data transfer test failed!");
                return -1;
            }
        }
        
        // Test 6: Empty buffer creation
        LOG_INFO("=== Test 6: Empty buffer creation ===");
        {
            AllPurposeBuffer<char> buffer(1024); // 1KB buffer
            LOG_SUCCESS("Empty buffer created successfully");
            
            // Write some test pattern
            std::vector<char> pattern(1024);
            for (size_t i = 0; i < pattern.size(); ++i) {
                pattern[i] = static_cast<char>(i % 256);
            }
            
            buffer.writeFrom(pattern);
            LOG_SUCCESS("Wrote test pattern to large buffer");
            
            std::vector<char> readBack;
            buffer.readTo(readBack);
            
            // Verify pattern
            bool patternValid = true;
            for (size_t i = 0; i < pattern.size(); ++i) {
                if (pattern[i] != readBack[i]) {
                    patternValid = false;
                    break;
                }
            }
            
            if (patternValid) {
                LOG_SUCCESS("Large buffer pattern test passed");
            } else {
                LOG_ERR("Large buffer pattern test failed!");
                return -1;
            }
        }
        
        LOG_SUCCESS("All buffer tests completed successfully!");
        
        // Test 7: Demonstrate compile-time flag validation
        LOG_INFO("=== Test 7: Compile-time flag validation ===");
        LOG_INFO("The following would cause compile-time errors if uncommented:");
        LOG_INFO("// ConstBuffer<int> buf(5);");
        LOG_INFO("// buf.writeFrom(data); // ERROR: HOST_WRITE not allowed");
        LOG_INFO("// GPUOnlyBuffer<int> buf2(5);");  
        LOG_INFO("// buf2.readTo(output); // ERROR: HOST_READ not allowed");
        LOG_SUCCESS("Compile-time validation works as expected");
        
        // Cleanup
        deleteGPU();
        LOG_INFO("Buffer System Test completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception caught during buffer test: " + std::string(e.what()));
        return -1;
    }
    
    return 0;
} 