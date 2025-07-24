#include "../include/rendering.hpp"
#include "../include/log.hpp"
#include "../include/util.hpp"

int main() {
    LOG_INIT();
    LOG_INFO("Starting Binning Demo");
    
    try {
        // Initialize GPU and Renderer
        LOG_DEBUG("Initializing GPU and Renderer");
        initGPU();
        
        int screen_width = 800, screen_height = 600;
        Renderer renderer(screen_width, screen_height, 1000);
        
        LOG_SUCCESS("GPU and Renderer initialized successfully");
        
        // Create a simple vertex buffer with a few triangles
        LOG_DEBUG("Creating vertex buffer with sample triangles");
        // Note: Using camera-space coordinates for now since indexed methods don't support camera transformation yet
        std::vector<vec> vertices = {
            {-100, -100, 100},  // 0: bottom-left
            { 100, -100, 100},  // 1: bottom-right  
            {   0,  100, 100},  // 2: top-center
            
            {-200,  -50, 150},  // 3: second triangle
            { -50,  -50, 150},  // 4
            {-125,   50, 150},  // 5
            
            { 150, -100, 200},  // 6: third triangle
            { 250, -100, 200},  // 7
            { 200,    0, 200},  // 8
        };
        
        // Set up camera to view the triangles
        LOG_DEBUG("Setting up camera to view the scene");
        Camera& camera = renderer.getCamera();
        camera.setPosition(0, 0, 0);     // Camera at origin
        camera.setYawDegrees(0);         // Looking straight ahead
        camera.setPitchDegrees(0);       // No pitch
        camera.setRollDegrees(0);        // No roll
        LOG_SUCCESS("Camera configured - positioned at origin looking towards negative Z");
        
        // Apply camera transformations to vertices
        std::vector<vec> transformedVertices;
        transformedVertices.reserve(vertices.size());
        
        for (const vec& vertex : vertices) {
            transformedVertices.push_back(camera.transformVertex(vertex));
        }
        
        // Create vertex buffer with camera-transformed vertices
        lr::AllPurposeBuffer<vec> vertexBuffer(transformedVertices.size(), transformedVertices);
        LOG_SUCCESS("Vertex buffer created with " + std::to_string(vertices.size()) + " camera-transformed vertices");
        
        // Test the binning system
        LOG_INFO("=== Testing Binning System ===");
        
        // Start a new frame for binning
        renderer.startNewFrame();
        LOG_DEBUG("Started new frame for binning");
        
        // Submit triangles for binning
        renderer.submitTriangleForBinning(vertexBuffer, 0, 1, 2, 0xFF0000FF); // Red triangle
        renderer.submitTriangleForBinning(vertexBuffer, 3, 4, 5, 0xFF00FF00); // Green triangle  
        renderer.submitTriangleForBinning(vertexBuffer, 6, 7, 8, 0xFFFF0000); // Blue triangle
        
        LOG_SUCCESS("Submitted " + std::to_string(renderer.getBinnedTriangleCount()) + " triangles for binning");
        
        // Execute the binning pass
        LOG_DEBUG("Executing binning pass...");
        renderer.executeBinningPass();
        LOG_SUCCESS("Binning pass completed successfully!");
        
        // Test tile-based rendering using binned data
        LOG_INFO("=== Testing Tile-Based Rendering ===");
        
        renderer.clear();
        
        // Execute tile-based rendering using the binned triangle data
        LOG_DEBUG("Executing tile-based rendering...");
        renderer.executeFinishFrameTileBased();
        LOG_SUCCESS("Tile-based rendering completed successfully!");
        
        // Traditional indexed drawing methods have been removed - only binning system now
        
        // Finish the frame
        uint32_t* framebuffer = renderer.finishFrame();
        LOG_SUCCESS("Frame completed - framebuffer ready");
        
        LOG_SUCCESS("Binning Demo completed successfully!");
        LOG_INFO("Both binning and tile-based rendering are now working!");
        LOG_INFO("Tile-based rendering uses binned triangle data for efficient GPU processing");
        
        deleteGPU();
        LOG_DEBUG("GPU resources cleaned up");
        
    } catch (const std::exception& e) {
        LOG_ERR("Demo failed with exception: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
} 