#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <CL/opencl.hpp>
#include <memory>
#include <optional>
#include <string>

#include "util.hpp"

class _GPU; // Forward declaration

class Texture{
    friend class _GPU; // Allow _GPU to access private constructor
    private:
        int width, height;
        std::shared_ptr<cl::Buffer> textureBuffer;
        
        // Constructor is private to enforce creation via factory in GPU class
        Texture(int w, int h, const uint32_t* data);

    public:
        ~Texture();
        
        // Textures are non-copyable and non-movable to enforce single ownership by GPUContext
        Texture(const Texture& other) = delete;
        Texture& operator=(const Texture& other) = delete;
        Texture(Texture&& other) noexcept = delete;
        Texture& operator=(Texture&& other) noexcept = delete;
        
        int getWidth() const;
        int getHeight() const;
        cl::Buffer& getBuffer();
        
        // Static factory method returns an ID instead of an object
        static std::optional<size_t> loadFromFile(const std::string& filename);
};

struct TexCoord {
    float u, v;
    TexCoord(float u = 0.0f, float v = 0.0f) : u(u), v(v) {}
};

#endif // TEXTURE_HPP 