// Triangle rasterization kernels
// This file contains the kernels for actually drawing triangles to the framebuffer

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

// Indexed triangle drawing - solid color
__kernel void drawIndexed(__global float* depthBuffer, __global int* colorArray, 
                         int screen_width, int screen_height,
                         __global packed_vec3* vertices, int v0_idx, int v1_idx, int v2_idx,
                         int clr, int minX, int minY,
                         float dx1, float dx2, float dy1, 
                         float dy2, float lambda1, float lambda2) {

    // Get vertex data from buffer
    packed_vec3 v0 = vertices[v0_idx];
    packed_vec3 v1 = vertices[v1_idx];
    packed_vec3 v2 = vertices[v2_idx];
    
    // Perform perspective projection (similar to CPU version)
    float z1 = v0.z, z2 = v1.z, z3 = v2.z;
    
    // Cull triangles too close to camera
    if(z1 < 10 || z2 < 10 || z3 < 10) {
        return;
    }
    
    // Project to screen space
    float scr_z = 1000.0f; // Same as used in CPU version
    float x1 = v0.x * scr_z / fabs(z1);
    float y1 = -v0.y * scr_z / fabs(z1);
    float x2 = v1.x * scr_z / fabs(z2);
    float y2 = -v1.y * scr_z / fabs(z2);
    float x3 = v2.x * scr_z / fabs(z3);
    float y3 = -v2.y * scr_z / fabs(z3);
    
    // Calculate triangle bounding box
    int boxLeft = (int)fmin(fmin(x1, x2), x3);
    int boxRight = (int)fmax(fmax(x1, x2), x3);
    int boxTop = (int)fmin(fmin(y1, y2), y3);
    int boxBottom = (int)fmax(fmax(y1, y2), y3);
    
    // Clip to screen bounds
    int maxx = screen_width;
    int maxy = screen_height;
    if(boxRight < (-maxx/2) + 1 || boxLeft > (maxx/2) + 3 || 
       boxTop > (maxy/2)+3 || boxBottom < (-maxy/2)+1) {
        return;
    }
    
    // coords of current pixel
    int x = 2*(get_global_id(0))+minX; 
    int y = 2*(get_global_id(1))+minY; 
    
    // Check if pixel is within triangle bounds
    if(x < boxLeft || x > boxRight || y < boxTop || y > boxBottom) {
        return;
    }

    // Calculate barycentric coordinates for this pixel
    float denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
    if(fabs(denom) < 0.001f) return; // Degenerate triangle
    
    float l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
    float l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
    float l3 = 1.0f - l1 - l2;

    // index of pixel in color buffer / z buffer
    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    
    // test if pixel is inside triangle
    if(l1 >= 0 && l2 >= 0 && l3 >= 0) { 
        // Interpolate depth
        float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
        
        // Test if point is closer to camera than last candidate
        if (inv_z < 800 && inv_z > depthBuffer[index]) { 
            depthBuffer[index] = inv_z;
            colorArray[index] = clr;
        }
    }

    // Repeat for every pixel in 2x2px tile
    ++index;
    x++;
    if(x >= boxLeft && x <= boxRight && y >= boxTop && y <= boxBottom) {
        denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
        if(fabs(denom) >= 0.001f) {
            l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
            l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
            l3 = 1.0f - l1 - l2;
            if(l1 >= 0 && l2 >= 0 && l3 >= 0) {
                float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
                if (inv_z < 800 && inv_z > depthBuffer[index]) {
                    depthBuffer[index] = inv_z;
                    colorArray[index] = clr;
                }
            }
        }
    }

    index += screen_width;
    y++;
    if(x >= boxLeft && x <= boxRight && y >= boxTop && y <= boxBottom) {
        denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
        if(fabs(denom) >= 0.001f) {
            l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
            l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
            l3 = 1.0f - l1 - l2;
            if(l1 >= 0 && l2 >= 0 && l3 >= 0) {
                float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
                if (inv_z < 800 && inv_z > depthBuffer[index]) {
                    depthBuffer[index] = inv_z;
                    colorArray[index] = clr;
                }
            }
        }
    }

    --index;
    x--;
    if(x >= boxLeft && x <= boxRight && y >= boxTop && y <= boxBottom) {
        denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
        if(fabs(denom) >= 0.001f) {
            l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
            l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
            l3 = 1.0f - l1 - l2;
            if(l1 >= 0 && l2 >= 0 && l3 >= 0) {
                float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
                if (inv_z < 800 && inv_z > depthBuffer[index]) {
                    depthBuffer[index] = inv_z;
                    colorArray[index] = clr;
                }
            }
        }
    }
}

// Indexed triangle drawing - textured
__kernel void drawTexturedIndexed(__global float* depthBuffer, __global int* colorArray, 
                                 int screen_width, int screen_height,
                                 __global packed_vec3* vertices, int v0_idx, int v1_idx, int v2_idx,
                                 __global int* texture, int tex_width, int tex_height,
                                 float ta_u, float ta_v, float tb_u, float tb_v, float tc_u, float tc_v,
                                 int minX, int minY,
                                 float dx1, float dx2, float dy1, 
                                 float dy2, float lambda1, float lambda2) {

    // Get vertex data from buffer
    packed_vec3 v0 = vertices[v0_idx];
    packed_vec3 v1 = vertices[v1_idx];
    packed_vec3 v2 = vertices[v2_idx];
    
    // Perform perspective projection (similar to CPU version)
    float z1 = v0.z, z2 = v1.z, z3 = v2.z;
    
    // Cull triangles too close to camera
    if(z1 < 10 || z2 < 10 || z3 < 10) {
        return;
    }
    
    // Project to screen space
    float scr_z = 1000.0f; // Same as used in CPU version
    float x1 = v0.x * scr_z / fabs(z1);
    float y1 = -v0.y * scr_z / fabs(z1);
    float x2 = v1.x * scr_z / fabs(z2);
    float y2 = -v1.y * scr_z / fabs(z2);
    float x3 = v2.x * scr_z / fabs(z3);
    float y3 = -v2.y * scr_z / fabs(z3);
    
    // Calculate triangle bounding box
    int boxLeft = (int)fmin(fmin(x1, x2), x3);
    int boxRight = (int)fmax(fmax(x1, x2), x3);
    int boxTop = (int)fmin(fmin(y1, y2), y3);
    int boxBottom = (int)fmax(fmax(y1, y2), y3);
    
    // Clip to screen bounds
    int maxx = screen_width;
    int maxy = screen_height;
    if(boxRight < (-maxx/2) + 1 || boxLeft > (maxx/2) + 3 || 
       boxTop > (maxy/2)+3 || boxBottom < (-maxy/2)+1) {
        return;
    }
    
    // coords of current pixel
    int x = 2*(get_global_id(0))+minX; 
    int y = 2*(get_global_id(1))+minY; 
    
    // Check if pixel is within triangle bounds
    if(x < boxLeft || x > boxRight || y < boxTop || y > boxBottom) {
        return;
    }

    // Calculate barycentric coordinates for this pixel
    float denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
    if(fabs(denom) < 0.001f) return; // Degenerate triangle
    
    float l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
    float l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
    float l3 = 1.0f - l1 - l2;

    // index of pixel in color buffer / z buffer
    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    
    // test if pixel is inside triangle
    if(l1 >= 0 && l2 >= 0 && l3 >= 0) { 
        // Interpolate depth
        float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
        
        // Test if point is closer to camera than last candidate
        if (inv_z < 800 && inv_z > depthBuffer[index]) { 
            // Interpolate texture coordinates with perspective correction
            float u = l1 * ta_u + l2 * tb_u + l3 * tc_u;
            float v = l1 * ta_v + l2 * tb_v + l3 * tc_v;
            
            // Sample texture and write to color buffer
            int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
            
            depthBuffer[index] = inv_z;
            colorArray[index] = texColor;
        }
    }

    // Repeat for every pixel in 2x2px tile (simplified version for now)
    ++index;
    x++;
    if(x >= boxLeft && x <= boxRight && y >= boxTop && y <= boxBottom) {
        denom = (x2 - x3) * (y1 - y3) + (y3 - y2) * (x1 - x3);
        if(fabs(denom) >= 0.001f) {
            l1 = ((x2 - x3) * (y - y3) + (y3 - y2) * (x - x3)) / denom;
            l2 = ((x3 - x1) * (y - y3) + (y1 - y3) * (x - x3)) / denom;
            l3 = 1.0f - l1 - l2;
            if(l1 >= 0 && l2 >= 0 && l3 >= 0) {
                float inv_z = l1 * (1.0f/z1) + l2 * (1.0f/z2) + l3 * (1.0f/z3);
                if (inv_z < 800 && inv_z > depthBuffer[index]) {
                    float u = l1 * ta_u + l2 * tb_u + l3 * tc_u;
                    float v = l1 * ta_v + l2 * tb_v + l3 * tc_v;
                    int texColor = sampleTexture(texture, tex_width, tex_height, u, v);
                    depthBuffer[index] = inv_z;
                    colorArray[index] = texColor;
                }
            }
        }
    }
} 