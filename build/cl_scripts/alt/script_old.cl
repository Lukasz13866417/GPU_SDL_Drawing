inline bool mollerTrumbore(float dx, float dy, 
                    float x1, float y1, float z1, 
                    float x2, float y2, float z2, 
                    float x3, float y3, float z3,
                    float *t, float *u, float *v, int scr_z){
    float dz = (float)(scr_z); // Assuming a far enough dz to create a directional vector from pixel to triangle
    float3 dir = (float3)(dx, dy, dz); // Direction vector of the ray
    float3 e1 = (float3)(x2 , y2 , z2 );
    float3 e2 = (float3)(x3 , y3 , z3 );
    float3 pvec = cross(dir, e2);
    float det = dot(e1, pvec);

    if(fabs(det) < 0.00000001f) return false; // Parallel, det close to 0
    float invDet = 1.0f / det;

    float3 tvec = (float3)(-x1, -y1, -z1); // Origin to vertex1
    *u = dot(tvec, pvec) * invDet;
    if(*u < 0.0f || *u > 1.0f) return false;

    float3 qvec = cross(tvec, e1);
    *v = dot(dir, qvec) * invDet;
    if(*v < 0.0f || *u + *v > 1.0f) return false;

    *t = dot(e2, qvec) * invDet; // This computes the distance from the origin to the intersection point
    return true;
}

inline float interpolate(float x, float y, 
                    float x1, float y1, float z1, 
                    float x2, float y2, float z2, 
                    float x3, float y3, float z3,
                    int scr_z, float inv){

        float3 V1 = (float3)(x1,y1,0);
        float3 V2 = (float3)(x2,y2,0);
        float3 V3 = (float3)(x3,y3,0);
        
        //float inv = 1.0f/(V1.x*V2.y - V1.x*V3.y - V1.y*V2.x + V1.y*V3.x + V2.x*V3.y - V2.y*V3.x);
        
        float lambda1 = (-V1.x*V3.y + V1.x*y + V1.y*V3.x - V1.y*x - V3.x*y + V3.y*x)*inv;
        float lambda2 = (V1.x*V2.y - V1.x*y - V1.y*V2.x + V1.y*x + V2.x*y - V2.y*x)*inv;

        if(lambda1 < 0 || lambda2 < 0 || lambda1+lambda2>1){
            return -1000000000000.0f;
        }
        
        float res = (1-lambda1-lambda2)/z1 + lambda1/z2 + lambda2/z3;
       // float res = (z1*z2*lambda2 + z1*z3*lambda1 + z2*z3*(1 - lambda1 - lambda2))/(z1*z2*z3);

        return res < 0 ? -1000000000000.0f : res;
        //return (z1*z2*lambda2 + z1*z3*lambda1 - z2*z3*lambda1 - z2*z3*lambda2 + z2*z3)/(z1*z2*z3);

}

__kernel void clear(__global float* depthBuffer, __global int* colorArray, int screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f; // pseudo infinity
    colorArray[index] = 255<<24; //  black
}

__kernel void draw(__global float* depthBuffer, __global int* colorArray,
                        int x1, int y1, int z1,
                        int x2, int y2, int z2,
                        int x3, int y3, int z3, int clr,
                        int screen_width, int screen_height, int scr_z, int minX, int minY,float inv) {
    int x = get_global_id(0)+minX;
    int y = get_global_id(1)+minY;

    int index = (y+(screen_height/2)) * screen_width + (x+(screen_width/2));
    

    float f = interpolate(x, y, x1, y1, z1, x2, y2, z2, x3, y3, z3, scr_z, inv);
    
    if (f>-999999999999.0f) {
        // Update depth buffer and color array if this pixel is closer
        if (f > depthBuffer[index]) {
            depthBuffer[index] = f;
            colorArray[index] = clr;
        }
    }
}

