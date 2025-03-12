typedef float3 vec3;

__kernel void draw(__global float* depthBuffer,__global int* colorArray,
                   float z1, float z2, float z3, int clr,
                   int screen_width, int screen_height, int minX, int minY,  float dx1, float dx2, float dy1, float dy2, float lambda1, float lambda2) {

    // do all the computations for a 2x2px tile.
    //float inv=1.0f/(v1.x*v2.y - v1.x*v3.y - v1.y*v2.x + v1.y*v3.x + v2.x*v3.y - v2.y*v3.x);
    int x = 2*(get_global_id(0))+minX; // minX,minY - top left of triangle bounding box
    int y = 2*(get_global_id(1))+minY; 

    //float lambda1 = (-v1.x*v3.y + v1.x*y + v1.y*v3.x - v1.y*x - v3.x*y + v3.y*x)*inv;
    //float lambda2 = (v1.x*v2.y - v1.x*y - v1.y*v2.x + v1.y*x + v2.x*y - v2.y*x)*inv;
    lambda1 += x*dx1 + y*dy1;
    lambda2 += x*dx2 + y*dy2;
    //float dx1 = (v3.y - v1.y)*inv, dx2 = (v1.y - v2.y)*inv; // barycentric coordinates of a point increase by this value when x increases by 1.
    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    float f;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){ // test if pixel is inside triangle's projection
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }
    
    ++index;
    lambda1 += dx1;
    lambda2 += dx2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }

    index+=screen_width;
    lambda1 += dy1; // same as dx1,dx2 but for changing y.
    lambda2 += dy2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }

    --index;
    lambda1 -= dx1;
    lambda2 -= dx2;
    if(lambda1>0 && lambda2>0 && lambda1+lambda2<1){
        f = lambda2*z3 + lambda1*z2 + (1-lambda1-lambda2)*z1;
        if (f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }    
}

__kernel void clear(__global float* depthBuffer,__global int* colorArray, int screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f;
    colorArray[index] = 255<<24; //  black
}
