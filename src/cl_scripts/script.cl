typedef float3 vec3;

typedef struct global_data {
    __global float* depthBuffer;
    __global int* colorArray;
    int screen_width,screen_height;
} global_data_t;

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

                                  // output buffers
__kernel void draw(__global float* depthBuffer,__global int* colorArray, 
                   int screen_width, int screen_height, 
                   // inverse z coords of triangle verts,   triangle color
                   float z1, float z2, float z3,               int clr,
                   
                   // top left corner of triangle projection's bbox
                   int minX, int minY,  
                   // preprocessed values for math calculations. Further explained below.
                   float dx1, float dx2, float dy1, 
                   float dy2, float lambda1, float lambda2) {

    // coords of current pixel
    int x = 2*(get_global_id(0))+minX; 
    int y = 2*(get_global_id(1))+minY; 

    // Preprocessed denominator in many expressions in the code:
    //   inv = 1/(v1.x*v2.y - v1.x*v3.y - v1.y*v2.x + v1.y*v3.x + v2.x*v3.y - v2.y*v3.x)

    // Barycentric coordinates of point (x,y) relative to triangle's projection (v1,2,3): 
    //   lambda1 = (-v1.x*v3.y + v1.x*y + v1.y*v3.x - v1.y*x - v3.x*y + v3.y*x)*inv
    //   lambda2 = (v1.x*v2.y - v1.x*y - v1.y*v2.x + v1.y*x + v2.x*y - v2.y*x)*inv

    // Not all parts of these expressions depend on x,y.

    // Lambda1,2 are "common part" of these coords throughout all (x,y) coords.
    // I took formulas from lines 22,23 but threw away terms containing x and y.
    // Thats because we can preprocess that, and take x,y into consideration here.
    // These values are also the barycentric coords of (0,0)
    // These coords increase by dx (dy) value when x (y) increases by 1.
    //  for lambda1:
    //   dx1 = (v3.y - v1.y)*inv, dx2 = (v1.y - v2.y)*inv
    //   dy is similar
    //  for lambda2, it's dx2, dy2.
    
    // To get coords of (x,y):
    lambda1 += x*dx1 + y*dy1;
    lambda2 += x*dx2 + y*dy2;

    // index of pixel in color buffer / z buffer
    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    float f; // distance from camera to point on original triangle, 
             // that corresponds to this pixel (if this px lies in triangle projection)
             // Actually, it's the inverse of that distance.
    // camera is at (0,0,0)
    // test if pixel is inside triangle's projection 
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){ 
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        // Test if point on 3D triangle, corresponding to this pixel inside projection, 
        // is closer to cam than last candidate
        if (f < 800 && f > depthBuffer[index]) { 
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }

    // Repeat for every pixel in 2x2px tile
    // Instead of recalculating the bar. coords, distance etc,
    // update values for current pixel, based on same values for previous pixel
    
    ++index;
    lambda1 += dx1;
    lambda2 += dx2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f < 800 && f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }

    index+=screen_width;
    lambda1 += dy1; 
    lambda2 += dy2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f < 800 && f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }

    --index;
    lambda1 -= dx1;
    lambda2 -= dx2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f < 800 && f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }    
}

__kernel void clear(__global float* depthBuffer,__global int* colorArray, uint screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f;
    colorArray[index] = 255<<24; //  black
}

int sampleTexture(__global int* texture, int tex_width, int tex_height, float u, float v) {
    // Convert to pixel coordinates
    // UV coordinates are already guaranteed to be in [0,1] from barycentric interpolation
    int px = (int)(u * (tex_width - 1));
    int py = (int)(v * (tex_height - 1));
    
    // Sample texture
    return texture[py * tex_width + px];
}

__kernel void drawTextured(__global float* depthBuffer, __global int* colorArray, 
                          int screen_width, int screen_height, 
                          // inverse z coords of triangle verts
                          float w1, float w2, float w3,
                          // texture data
                          __global int* texture, int tex_width, int tex_height,
                          // pre-multiplied texture coordinates for each vertex (u/z, v/z)
                          float ta_u_w, float ta_v_w, float tb_u_w, float tb_v_w, float tc_u_w, float tc_v_w,
                          // top left corner of triangle projection's bbox
                          int minX, int minY,  
                          // preprocessed values for barycentric calculations
                          float dx1, float dx2, float dy1, 
                          float dy2, float lambda1, float lambda2) {

    // coords of current pixel
    int x = 2*(get_global_id(0))+minX; 
    int y = 2*(get_global_id(1))+minY; 

    // Calculate barycentric coordinates for current pixel
    float l1 = lambda1 + x*dx1 + y*dy1;
    float l2 = lambda2 + x*dx2 + y*dy2;
    float l3 = 1.0f - l1 - l2;

    // index of pixel in color buffer / z buffer
    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    
    // test if pixel is inside triangle's projection 
    if(l1 > 0 && l2 > 0 && l1 + l2 < 1) { 
        // Interpolate inverse depth using screen-space barycentric coordinates
        float w_interp = l3*w1 + l1*w2 + l2*w3;
        
        // Test if point is closer to camera than last candidate
        if (w_interp < 800 && w_interp > depthBuffer[index]) { 
            // Interpolate pre-multiplied texture coordinates
            float u_w = l3*ta_u_w + l1*tb_u_w + l2*tc_u_w;
            float v_w = l3*ta_v_w + l1*tb_v_w + l2*tc_v_w;

            // Divide by interpolated inverse depth to get perspective-correct u,v
            float u = u_w / w_interp;
            float v = v_w / w_interp;
            
            // Sample texture and write to color buffer
            int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
            
            depthBuffer[index] = w_interp;
            colorArray[index] = texColor;
        }
    }

    // Repeat for every pixel in 2x2px tile
    // Process pixel (x+1, y)
    ++index;
    l1 += dx1;
    l2 += dx2;
    l3 = 1.0f - l1 - l2;
    if(l1 > 0 && l2 > 0 && l1 + l2 < 1) {
        float w_interp = l3*w1 + l1*w2 + l2*w3;
        if (w_interp < 800 && w_interp > depthBuffer[index]) {
            float u_w = l3*ta_u_w + l1*tb_u_w + l2*tc_u_w;
            float v_w = l3*ta_v_w + l1*tb_v_w + l2*tc_v_w;
            float u = u_w / w_interp;
            float v = v_w / w_interp;
            int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
            depthBuffer[index] = w_interp;
            colorArray[index] = texColor;
        }
    }

    // Process pixel (x+1, y+1)
    index += screen_width;
    l1 += dy1; 
    l2 += dy2;
    l3 = 1.0f - l1 - l2;
    if(l1 > 0 && l2 > 0 && l1 + l2 < 1) {
        float w_interp = l3*w1 + l1*w2 + l2*w3;
        if (w_interp < 800 && w_interp > depthBuffer[index]) {
            float u_w = l3*ta_u_w + l1*tb_u_w + l2*tc_u_w;
            float v_w = l3*ta_v_w + l1*tb_v_w + l2*tc_v_w;
            float u = u_w / w_interp;
            float v = v_w / w_interp;
            int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
            depthBuffer[index] = w_interp;
            colorArray[index] = texColor;
        }
    }

    // Process pixel (x, y+1)
    --index;
    l1 -= dx1;
    l2 -= dx2;
    l3 = 1.0f - l1 - l2;
    if(l1 > 0 && l2 > 0 && l1 + l2 < 1) {
        float w_interp = l3*w1 + l1*w2 + l2*w3;
        if (w_interp < 800 && w_interp > depthBuffer[index]) {
            float u_w = l3*ta_u_w + l1*tb_u_w + l2*tc_u_w;
            float v_w = l3*ta_v_w + l1*tb_v_w + l2*tc_v_w;
            float u = u_w / w_interp;
            float v = v_w / w_interp;
            int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
            depthBuffer[index] = w_interp;
            colorArray[index] = texColor;
        }
    }    
}