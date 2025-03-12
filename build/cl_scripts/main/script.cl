typedef float3 vec3;

inline float interpolate(vec3 pos, vec3 v1, vec3 v2, vec3 v3, float inv){
    //float inv = 1.0f/(v1.x*v2.y - v1.x*v3.y - v1.y*v2.x + v1.y*v3.x + v2.x*v3.y - v2.y*v3.x)
    float lambda1 = (-v1.x*v3.y + v1.x*pos.y + v1.y*v3.x - v1.y*pos.x - v3.x*pos.y + v3.y*pos.x)*inv;
    float lambda2 = (v1.x*v2.y - v1.x*pos.y - v1.y*v2.x + v1.y*pos.x + v2.x*pos.y - v2.y*pos.x)*inv;
    if(!(lambda1 < 0 || lambda2 < 0 || lambda1+lambda2>1)){
        float res = lambda2*v3.z + lambda1*v2.z + (1-lambda1-lambda2)*v1.z;
        return res > 0 ? res : -10000000000000.0f;  // pseudo infinity
    }else{
        return -10000000000000.0f;
    }
}

__kernel void draw(__global float* depthBuffer,__global int* colorArray,
                   vec3 v1, vec3 v2, vec3 v3, int clr,
                   int screen_width, int screen_height, int minX, int minY, float inv) {

    int x = (get_global_id(0))+minX;
    int y = (get_global_id(1))+minY;
    vec3 pos = (vec3)(x, y, 0);
    int index = (pos.y + (screen_height / 2)) * screen_width + (pos.x + (screen_width / 2));
    float f = interpolate(pos, v1, v2, v3, inv);
    if (f > depthBuffer[index]) {
        depthBuffer[index] = f;
        colorArray[index] = clr;
    }
}

__kernel void clear(__global float* depthBuffer,__global int* colorArray, int screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f;
    colorArray[index] = 255<<24; //  black
}
