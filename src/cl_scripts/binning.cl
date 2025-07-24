// Triangle binning kernels
// This file contains kernels for the binning pass that determines which screen tiles 
// each triangle affects, enabling tile-based rendering

// Screen tile configuration
#define TILE_SIZE 32           // Each tile is 32x32 pixels
#define MAX_TRIANGLES_PER_TILE 256  // Maximum triangles that can be assigned to a tile

// Tile data structure - contains list of triangle IDs affecting this tile
typedef struct __attribute__((packed)) {
    int triangle_ids[MAX_TRIANGLES_PER_TILE];  // Triangle IDs in this tile
    int triangle_count;                        // Number of triangles in this tile
} TileData;

// Kernel that processes each triangle and determines which tiles it affects
__kernel void binTriangles(__global TriangleData* triangles,
                          int triangle_count,
                          __global TileData* tiles, 
                          int screen_width, int screen_height,
                          int tiles_per_row, int tiles_per_column) {
    
    int triangle_id = get_global_id(0);
    if (triangle_id >= triangle_count) return;
    
    TriangleData triangle = triangles[triangle_id];
    
    // Get the three vertices from the triangle's vertex buffer
    packed_vec3 v0 = triangle.vertexBuffer[triangle.v0_idx];
    packed_vec3 v1 = triangle.vertexBuffer[triangle.v1_idx];
    packed_vec3 v2 = triangle.vertexBuffer[triangle.v2_idx];
    
    // Project vertices to screen space (same logic as in rasterization)
    float z1 = v0.z, z2 = v1.z, z3 = v2.z;
    
    // Cull triangles too close to camera
    if(z1 < 10 || z2 < 10 || z3 < 10) {
        return;
    }
    
    // Project to screen space
    float scr_z = 1000.0f;
    float x1 = v0.x * scr_z / fabs(z1);
    float y1 = -v0.y * scr_z / fabs(z1);
    float x2 = v1.x * scr_z / fabs(z2);
    float y2 = -v1.y * scr_z / fabs(z2);
    float x3 = v2.x * scr_z / fabs(z3);
    float y3 = -v2.y * scr_z / fabs(z3);
    
    // Calculate triangle bounding box in screen space
    int boxLeft = (int)fmin(fmin(x1, x2), x3);
    int boxRight = (int)fmax(fmax(x1, x2), x3);
    int boxTop = (int)fmin(fmin(y1, y2), y3);
    int boxBottom = (int)fmax(fmax(y1, y2), y3);
    
    // Convert screen coordinates to tile coordinates
    // Screen center is at (0,0), so adjust for tile grid
    int screen_center_x = screen_width / 2;
    int screen_center_y = screen_height / 2;
    
    int tile_left = (boxLeft + screen_center_x) / TILE_SIZE;
    int tile_right = (boxRight + screen_center_x) / TILE_SIZE;
    int tile_top = (boxTop + screen_center_y) / TILE_SIZE;
    int tile_bottom = (boxBottom + screen_center_y) / TILE_SIZE;
    
    // Clamp to valid tile ranges
    tile_left = max(0, min(tile_left, tiles_per_row - 1));
    tile_right = max(0, min(tile_right, tiles_per_row - 1));
    tile_top = max(0, min(tile_top, tiles_per_column - 1));
    tile_bottom = max(0, min(tile_bottom, tiles_per_column - 1));
    
    // Add this triangle to all tiles it overlaps
    for (int ty = tile_top; ty <= tile_bottom; ty++) {
        for (int tx = tile_left; tx <= tile_right; tx++) {
            int tile_index = ty * tiles_per_row + tx;
            
            // Atomically add triangle to tile's list
            int slot = atomic_inc(&tiles[tile_index].triangle_count);
            if (slot < MAX_TRIANGLES_PER_TILE) {
                tiles[tile_index].triangle_ids[slot] = triangle_id;
            }
            // If tile is full, triangles will be dropped (could add overflow handling)
        }
    }
}

// Clear tile data before binning pass
__kernel void clearTiles(__global TileData* tiles, int total_tiles) {
    int tile_id = get_global_id(0);
    if (tile_id >= total_tiles) return;
    
    tiles[tile_id].triangle_count = 0;
    // Note: We don't need to clear the triangle_ids array as triangle_count tracks valid entries
}

// Kernel that renders a specific tile using the triangles assigned to it
__kernel void renderTile(__global float* depthBuffer, __global int* colorArray,
                        int screen_width, int screen_height,
                        __global TileData* tiles, __global TriangleData* triangles,
                        int tile_x, int tile_y, int tiles_per_row) {
    
    // Get pixel coordinates within the tile
    int local_x = get_global_id(0);  // 0 to TILE_SIZE-1
    int local_y = get_global_id(1);  // 0 to TILE_SIZE-1
    
    if (local_x >= TILE_SIZE || local_y >= TILE_SIZE) return;
    
    // Convert to screen coordinates
    int screen_x = tile_x * TILE_SIZE + local_x - screen_width/2;
    int screen_y = tile_y * TILE_SIZE + local_y - screen_height/2;
    int pixel_index = (screen_y + screen_height/2) * screen_width + (screen_x + screen_width/2);
    
    // Get the tile data
    int tile_index = tile_y * tiles_per_row + tile_x;
    TileData tile = tiles[tile_index];
    
    // Process all triangles assigned to this tile
    for (int i = 0; i < tile.triangle_count && i < MAX_TRIANGLES_PER_TILE; i++) {
        int triangle_id = tile.triangle_ids[i];
        TriangleData triangle = triangles[triangle_id];
        
        // Get vertices and project them (similar to rasterization kernels)
        packed_vec3 v0 = triangle.vertexBuffer[triangle.v0_idx];
        packed_vec3 v1 = triangle.vertexBuffer[triangle.v1_idx];
        packed_vec3 v2 = triangle.vertexBuffer[triangle.v2_idx];
        
        float z1 = v0.z, z2 = v1.z, z3 = v2.z;
        if(z1 < 10 || z2 < 10 || z3 < 10) continue;
        
        float scr_z = 1000.0f;
        float x1 = v0.x * scr_z / fabs(z1);
        float y1 = -v0.y * scr_z / fabs(z1);
        float x2 = v1.x * scr_z / fabs(z2);
        float y2 = -v1.y * scr_z / fabs(z2);
        float x3 = v2.x * scr_z / fabs(z3);
        float y3 = -v2.y * scr_z / fabs(z3);
        
        // Calculate barycentric coordinates for current pixel
        float denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
        if(fabs(denom) < 0.001f) continue; // Degenerate triangle
        
        float l1 = ((x2 - x3) * (screen_y - y3) + (y3 - y2) * (screen_x - x3)) / denom;
        float l2 = ((x3 - x1) * (screen_y - y3) + (y1 - y3) * (screen_x - x3)) / denom;
        float l3 = 1.0f - l1 - l2;
        
        // Test if pixel is inside triangle
        if(l1 >= 0 && l2 >= 0 && l3 >= 0) {
            // Interpolate depth
            float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
            
            // Test depth and update pixel if closer
            if (inv_z < 800 && inv_z > depthBuffer[pixel_index]) {
                depthBuffer[pixel_index] = inv_z;
                
                // Handle textured vs solid color triangles
                if (triangle.texture != 0) {
                    // Textured triangle - interpolate texture coordinates
                    float u = l1 * triangle.tex_coords[0] + l2 * triangle.tex_coords[2] + l3 * triangle.tex_coords[4];
                    float v = l1 * triangle.tex_coords[1] + l2 * triangle.tex_coords[3] + l3 * triangle.tex_coords[5];
                    
                    // Sample texture
                    int texColor = sampleTexture(triangle.texture, triangle.tex_width, triangle.tex_height, u, v);
                    colorArray[pixel_index] = texColor;
                } else {
                    // Solid color triangle
                    colorArray[pixel_index] = triangle.color;
                }
            }
        }
    }
} 