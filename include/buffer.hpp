#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "log.hpp"
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <algorithm> 
#include <span>
#include <CL/opencl.hpp>

// Forward declare helper accessors implemented in rendering2.cpp
cl::Context& gpuContext();
cl::CommandQueue& gpuQueue();

// Forward declaration of GPU with the minimal API used in this header
class GPU; // opaque; full definition elsewhere.
GPU& getGPU();

namespace lr {

// Base class holds the common state.
template<typename T>
class BaseBuffer {
protected:
    size_t m_size;
    cl::Buffer m_buffer;

    BaseBuffer(size_t elementCount) : m_size(elementCount) {
        static_assert(std::is_trivially_copyable<T>::value, 
                            "T must be trivially copyable");
    }

public:
    virtual ~BaseBuffer() = default;
    size_t size() const { return m_size; }
    // Used when we want to pass it to a kernel. 
    const cl::Buffer& getCLBuffer() const { return m_buffer; }
};

// These flags determine what can be done with the buffer.
// More user-friendly than OpenCL's raw flags (in my opinion).
// Used only at compile time to deduce actual cl_mem_flags.
enum BufferFlag {
    HOST_WRITE,   // Host is allowed to write (used by writeFrom()).
    HOST_READ,    // Host is allowed to read (used by readTo()).
    GPU_WRITE,    // Device (kernel) is allowed to write.
    GPU_READ,     // Device (kernel) is allowed to read.
};

// Helper to check if a flag is present in the parameter pack
template<BufferFlag Target, BufferFlag... Flags>
constexpr inline bool has_flag() {
    return ((Flags == Target) || ...);
}

// Helper that translates user-friendly BufferFlags into OpenCL memory flags
template<BufferFlag... Flags>
constexpr inline cl_mem_flags deduceFlags(){
    // Deduce device (GPU) access flags:
    constexpr bool gpuRead  = has_flag<GPU_READ, Flags...>();
    constexpr bool gpuWrite = has_flag<GPU_WRITE, Flags...>();
    cl_mem_flags clFlags = 0;
    if (gpuRead && !gpuWrite){
        clFlags |= CL_MEM_READ_ONLY;
    } else if (gpuWrite && !gpuRead){
        clFlags |= CL_MEM_WRITE_ONLY;
    } else{
        clFlags |= CL_MEM_READ_WRITE; // Default: both read & write.
        // Same as in OpenCL - default flag is CL_MEM_READ_WRITE.
    }

    // Deduce host access flags:
    constexpr bool hostRead  = has_flag<HOST_READ, Flags...>();
    constexpr bool hostWrite = has_flag<HOST_WRITE, Flags...>();
    if (hostRead && !hostWrite)
        clFlags |= CL_MEM_HOST_READ_ONLY;
    else if (hostWrite && !hostRead)
        clFlags |= CL_MEM_HOST_WRITE_ONLY;
    // If both or neither are specified, we don't restrict host access further.
    return clFlags;
} 

// The GeneralBuffer class - forward declaration
template<typename T, BufferFlag... Flags>
class GeneralBuffer;

// Convenient type aliases for common flag combinations
template<typename T>
using AllPurposeBuffer = GeneralBuffer<T, HOST_READ, HOST_WRITE, GPU_READ, GPU_WRITE>;

template<typename T>
using GPUOnlyBuffer = GeneralBuffer<T, GPU_READ, GPU_WRITE>;

template<typename T>
using ConstBuffer = GeneralBuffer<T, GPU_READ>;

template<typename T>
using ConstBufferHostReadable = GeneralBuffer<T, GPU_READ, HOST_READ>;

template<typename T>
using GPUProducedBuffer = GeneralBuffer<T, GPU_WRITE, HOST_READ>;

template<typename T>
using GPUProducedAndReadBuffer = GeneralBuffer<T, GPU_READ, GPU_WRITE, HOST_READ>;

template<typename T>
using HostProducedBuffer = GeneralBuffer<T, HOST_WRITE, GPU_READ>;

template<typename T>
using HostProducedAndReadBuffer = GeneralBuffer<T, HOST_READ, HOST_WRITE, GPU_READ>;

// Template class implementation - now included directly in header for simplicity
template<typename T, BufferFlag... Flags>
class GeneralBuffer : public BaseBuffer<T> {
public:
    // Constructor with optional initial data
    GeneralBuffer(size_t elementCount, const std::vector<T> &data = {})
    : BaseBuffer<T>(elementCount) {
        
        cl_mem_flags clFlags = deduceFlags<Flags...>();
        // Use initial data if provided:
        const T* hostPtr = nullptr;
        if (!data.empty()) {
            if (data.size() != elementCount) {
                LOG_FATAL("GeneralBuffer: Data size doesn't match element count");
            }
            clFlags |= CL_MEM_COPY_HOST_PTR;
            hostPtr = data.data();
        }

        // Create the OpenCL buffer.
        this->m_buffer = cl::Buffer(
            gpuContext(),
            clFlags,
            sizeof(T) * elementCount,
            // OpenCL wants void* anyway.
            const_cast<void*>(static_cast<const void*>(hostPtr))
        );
        
        LOG_DEBUG("Created GeneralBuffer with " + std::to_string(elementCount) + " elements of size " + std::to_string(sizeof(T)));
    }

    // Constructor from span
    GeneralBuffer(size_t elementCount, const std::span<const T> &data)
    : BaseBuffer<T>(elementCount) {

        if (data.size() != elementCount) {
            LOG_FATAL("GeneralBuffer: Data size doesn't match element count");
        }

        cl_mem_flags clFlags = deduceFlags<Flags...>();
        clFlags |= CL_MEM_COPY_HOST_PTR;
        
        // Create the OpenCL buffer.
        this->m_buffer = cl::Buffer(
            gpuContext(),
            clFlags,
            sizeof(T) * elementCount,
            // OpenCL wants void* anyway.
            const_cast<void*>(static_cast<const void*>(data.data()))
        );
        
        LOG_DEBUG("Created GeneralBuffer from span with " + std::to_string(elementCount) + " elements of size " + std::to_string(sizeof(T)));
    }

    // Write methods (require HOST_WRITE flag)
    void writeFrom(const std::vector<T> &data) {
        static_assert(has_flag<HOST_WRITE, Flags...>(), "Buffer must have HOST_WRITE flag to use writeFrom");

        if (data.size() != this->m_size) {
            LOG_FATAL("GeneralBuffer::writeFrom: Data size doesn't match buffer size");
        }
        
        cl_int err = gpuQueue().enqueueWriteBuffer(
            this->m_buffer, CL_TRUE, 0, sizeof(T) * this->m_size, data.data()
        );
        
        if (err != CL_SUCCESS) {
            LOG_FATAL("GeneralBuffer::writeFrom failed with error: " + std::to_string(err));
        }
        
        LOG_DEBUG("Wrote " + std::to_string(data.size()) + " elements to buffer");
    }

    void writeFrom(const std::span<const T> data) {
        static_assert(has_flag<HOST_WRITE, Flags...>(), "Buffer must have HOST_WRITE flag to use writeFrom");

        if (data.size() != this->m_size) {
            LOG_FATAL("GeneralBuffer::writeFrom: Data size doesn't match buffer size");
        }
        
        cl_int err = gpuQueue().enqueueWriteBuffer(
            this->m_buffer, CL_TRUE, 0, sizeof(T) * this->m_size, data.data()
        );
        
        if (err != CL_SUCCESS) {
            LOG_FATAL("GeneralBuffer::writeFrom failed with error: " + std::to_string(err));
        }
        
        LOG_DEBUG("Wrote " + std::to_string(data.size()) + " elements to buffer from span");
    }

    // Read methods (require HOST_READ flag)
    void readTo(std::vector<T> &data) {
        static_assert(has_flag<HOST_READ, Flags...>(), "Buffer must have HOST_READ flag to use readTo");

        data.resize(this->m_size);
        cl_int err = gpuQueue().enqueueReadBuffer(
            this->m_buffer, CL_TRUE, 0, sizeof(T) * this->m_size, data.data()
        );
        
        if (err != CL_SUCCESS) {
            LOG_FATAL("GeneralBuffer::readTo failed with error: " + std::to_string(err));
        }
        
        LOG_DEBUG("Read " + std::to_string(data.size()) + " elements from buffer");
    }

    void readTo(std::span<T> data) {
        static_assert(has_flag<HOST_READ, Flags...>(), "Buffer must have HOST_READ flag to use readTo");
    
        if (data.size() != this->m_size) {
            LOG_FATAL("GeneralBuffer::readTo: Data size doesn't match buffer size");
        }
        
        cl_int err = gpuQueue().enqueueReadBuffer(
            this->m_buffer, CL_TRUE, 0, sizeof(T) * this->m_size, data.data()
        );
        
        if (err != CL_SUCCESS) {
            LOG_FATAL("GeneralBuffer::readTo failed with error: " + std::to_string(err));
        }
        
        LOG_DEBUG("Read " + std::to_string(data.size()) + " elements from buffer to span");
    }
};

} // namespace lr

#endif // BUFFER_HPP 