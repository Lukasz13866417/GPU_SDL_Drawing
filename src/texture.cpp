#include "../include/rendering.hpp" 
#include "../include/texture.hpp"
#include "../include/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

Texture::Texture(int w, int h, const std::vector<uint32_t>& data) 
    : width(w), height(h), buffer(w * h, data) {
    if (data.empty()) {
        LOG_FATAL("ConstBuffer requires initial data - cannot create empty texture");
    }
    LOG_DEBUG("Created immutable texture " + std::to_string(w) + "x" + std::to_string(h) + " with data");
}

Texture::Texture(int w, int h, std::span<const uint32_t> data)
    : width(w), height(h), buffer(w * h, data) {
    LOG_DEBUG("Created texture from span " + std::to_string(w) + "x" + std::to_string(h));
}



std::optional<Texture> Texture::loadFromFile(const std::string& filename) {
    int width, height, channels;
    
    // Load image using stb_image
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4); // Force RGBA
    
    if (!imageData) {
        LOG_ERR("Failed to load texture: " + filename + " - " + std::string(stbi_failure_reason()));
        return std::nullopt;
    }
    
    // Convert to uint32_t format (ARGB)
    std::vector<uint32_t> textureData(width * height);
    for (int i = 0; i < width * height; i++) {
        unsigned char r = imageData[i * 4 + 0];
        unsigned char g = imageData[i * 4 + 1];
        unsigned char b = imageData[i * 4 + 2];
        unsigned char a = imageData[i * 4 + 3];
        textureData[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    
    stbi_image_free(imageData);
    
    try {
        Texture texture(width, height, textureData);
        LOG_SUCCESS("Successfully loaded texture: " + filename + " (" + 
                   std::to_string(width) + "x" + std::to_string(height) + ")");
        return texture;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to create texture for: " + filename + " - " + std::string(e.what()));
        return std::nullopt;
    }
} 