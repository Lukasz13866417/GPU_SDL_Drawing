#include "../include/texture.hpp"
#include "../include/rendering.hpp" // For getGPU()

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

Texture::Texture(int w, int h, const uint32_t* data) : width(w), height(h) {
    assert(getGPU().isInitialized());
    
    if (data != nullptr) {
        // Create OpenCL buffer for texture data
        textureBuffer = std::make_shared<cl::Buffer>(
            getGPU().getContext(), 
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
            sizeof(uint32_t) * width * height,
            (void*)data
        );
    } else {
        // This case should ideally not be used with the new factory model,
        // but is kept for robustness.
        textureBuffer = std::make_shared<cl::Buffer>(
            getGPU().getContext(), 
            CL_MEM_READ_ONLY, 
            sizeof(uint32_t) * width * height
        );
    }
}

Texture::~Texture() {}

int Texture::getWidth() const {
    return width;
}

int Texture::getHeight() const {
    return height;
}

cl::Buffer& Texture::getBuffer() {
    return *textureBuffer;
}

std::optional<size_t> Texture::loadFromFile(const std::string& filename) {
    int width, height, channels;
    
    // Load image using stb_image
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4); // Force RGBA
    
    if (!imageData) {
        std::cerr << "Failed to load texture: " << filename << " - " << stbi_failure_reason() << std::endl;
        return std::nullopt;
    }
    
    // Convert to uint32_t format expected by our renderer (ARGB)
    uint32_t* textureData = new uint32_t[width * height];
    for (int i = 0; i < width * height; i++) {
        unsigned char r = imageData[i * 4 + 0];
        unsigned char g = imageData[i * 4 + 1];
        unsigned char b = imageData[i * 4 + 2];
        unsigned char a = imageData[i * 4 + 3];
        textureData[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    
    stbi_image_free(imageData);
    
    try {
        // Create the texture via the GPU context and get its ID
        size_t textureID = getGPU().createTexture(width, height, textureData);
        delete[] textureData;
        
        std::cout << "Successfully loaded texture: " << filename << " (" << width << "x" << height << ") with ID: " << textureID << std::endl;
        return textureID;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create OpenCL texture for: " << filename << " - " << e.what() << std::endl;
        delete[] textureData;
        return std::nullopt;
    }
} 