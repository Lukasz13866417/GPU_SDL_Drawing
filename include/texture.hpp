#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <vector>
#include <optional>
#include <string>
#include <span>
#include "buffer.hpp"



class Texture {
    private:
        int width, height;
    // Use ConstBuffer for textures - immutable GPU-only data
    lr::ConstBuffer<uint32_t> buffer;

public:
    // Constructor to create texture with given dimensions and data
    Texture(int w, int h, const std::vector<uint32_t>& data);
    
    // Constructor from span for more efficient initialization
    Texture(int w, int h, std::span<const uint32_t> data);
    
    
    // Copy semantics: 
    // Note: Copying a Texture creates a shared reference to the same GPU buffer memory.
    // This is different from creating a new buffer with copied data.
    // The underlying cl::Buffer is a handle/reference, so multiple Texture objects
    // can safely reference the same GPU memory. The OpenCL buffer memory is managed
    // by OpenCL's reference counting system.
    //
    // If you need independent GPU buffers with the same data, create a new Texture
    // by reading back the data and creating a new instance.
    Texture(const Texture& other) = default;
    Texture& operator=(const Texture& other) = default;
    
    // Move semantics
    Texture(Texture&& other) noexcept = default;
    Texture& operator=(Texture&& other) noexcept = default;
    
    ~Texture() = default;
        
    // Accessors
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    size_t getPixelCount() const { return width * height; }
    
    // Get the underlying OpenCL buffer for kernel usage
    const cl::Buffer& getCLBuffer() const { return buffer.getCLBuffer(); }
    


    // Static factory method to load texture from file
    static std::optional<Texture> loadFromFile(const std::string& filename);
};

struct TexCoord {
    float u, v;
    TexCoord(float u = 0.0f, float v = 0.0f) : u(u), v(v) {}
};

#endif // TEXTURE_HPP 