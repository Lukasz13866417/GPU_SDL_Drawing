__kernel void rightTriangle(__global float* depthBuffer, __global int* colorArray, 
                        int x1, int y1, int z1,
                        int x2, int y2, int z2, // adjacents are 1-3 (horizontal), 2-3 (vertical). Hypotenuse is 1-2
                        int x3, int y3, int z3, int minX, int minY, int screen_width, int screen_height,  int color){ 
    float x = minX + get_global_id(0), y = minY + get_global_id(1);
    float alpha1 = (x - x3) / (x1 - x3), alpha2 = (y - y3) / (y2 - y3);
    if(alpha1+alpha2<=1){
        int index = (x + screen_width/2) + (y + screen_height/2) * screen_width;
        depthBuffer[index] = 1000;
        colorArray[index] = color;
    }
}

__kernel void obtuseTriangle(__global float* depthBuffer, __global int* colorArray, 
                        int x1, int y1, int z1,  // edge 1-2 is horizontal
                        int x2, int y2, int z2,  // vert 3 is closer to vert 1
                        int x3, int y3, int z3, int minX, int minY, int screen_width, int screen_height, int color){ 
    float x = minX + get_global_id(0), y = minY + get_global_id(1);
    float alpha1 = (x - x3) / (x1 - x3), alpha2 = (y - y1) / (y3 - y1);
    float beta1 = (x - x3) / (x2 - x3), beta2 = (y - y1) / (y3 - y1);

    if(beta1+beta2<=1 && alpha1 + alpha2 >= 1){
        int index = (x + screen_width/2) + (y + screen_height/2) * screen_width;
        depthBuffer[index] = 1000;
        colorArray[index] = color;
    }
}

__kernel void clear(__global float* depthBuffer, __global int* colorArray, int screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f; // pseudo infinity
    colorArray[index] = 255<<24; //  black
}