typedef float3 vec3;

// Packed vec3 structure matching the C++ vec struct (no padding)
typedef struct __attribute__((packed)) {
    float x, y, z;
} packed_vec3;

typedef struct global_data {
    __global float* depthBuffer;
    __global int* colorArray;
    int screen_width, screen_height;
} global_data_t;

// Triangle metadata for binning pass
typedef struct __attribute__((packed)) {
    __global packed_vec3* vertexBuffer;  // Pointer to vertex buffer
    int v0_idx, v1_idx, v2_idx;          // Vertex indices
    float tex_coords[6];                 // 6 texture coordinates (u0,v0,u1,v1,u2,v2)
    __global int* texture;               // Texture buffer (null for solid color triangles)
    int tex_width, tex_height;           // Texture dimensions
    int color;                           // Solid color (used when texture is null)
    int triangle_id;                     // Unique triangle ID for this frame
} TriangleData;

__kernel void makeGlobalData(__global float* depthBuffer,
                             __global int* colorArray,
                             __global global_data_t* res,
                             uint screen_width,
                             uint screen_height) {
    if (get_global_id(0) == 0) {
        res->depthBuffer = depthBuffer;
        res->colorArray  = colorArray;
        res->screen_width  = screen_width;
        res->screen_height = screen_height;
    }
}

__kernel void getGlobalDataSize(__global int* res){
    if (get_global_id(0) == 0) {
        *res = sizeof(global_data_t);
    }
}

int makeColorDarker(int color, float f)
{
    // 1) Extract the ARGB components
    int a = (color >> 24) & 0xFF;
    int r = (color >> 16) & 0xFF;
    int g = (color >>  8) & 0xFF;
    int b =  color        & 0xFF;

    float factor = 1.0f - 0.00125f / f;  // bigger f => color gets darker
    if (factor < 0.0f) factor = 0.0f;    // clamp minimum so we don't go negative
    if (factor > 1.0f) factor = 1.0f;    // clamp maximum if needed

    // 2) Apply the factor
    r = (int)(r * factor);
    g = (int)(g * factor);
    b = (int)(b * factor);

    // 3) Recombine into ARGB
    int darkerColor = (a << 24) | (r << 16) | (g << 8) | b;
    return darkerColor;
}

int sampleTexture(__global int* texture, int tex_width, int tex_height, float u, float v) {
    // Convert to pixel coordinates
    // UV coordinates are already guaranteed to be in [0,1] from barycentric interpolation
    int px = (int)(u * (tex_width - 1));
    int py = (int)(v * (tex_height - 1));
    
    // Sample texture
    return texture[py * tex_width + px];
}

__kernel void clear(__global float* depthBuffer,__global int* colorArray, uint screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f;
    colorArray[index] = 255<<24; //  black
} 