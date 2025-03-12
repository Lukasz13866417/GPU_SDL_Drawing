
__kernel void draw(__global float *depthArray, __global int *colorArray, float z1, float z2, float z3, int minX, int minY, int scr_w, int scr_h, int color, float u, float v, float dux, float duy, float dvx, float dvy){
    int xL = minX + 6*get_global_id(0), xR = xL+5, y = minY + get_global_id(1);
    float3 pL = (float3)(xL,y,0);//, pR = (float3)(xR,y,0);
    /*float u,v;//, uR,vR;
    barycentric(&pL,&v1,&v2,&v3,&u,&v);*/
    float uL = u + xL*dux + y*duy;
    float vL = v + xL*dvx + y*dvy;
    float uR = uL + dux*5.0f;
    float vR = vL + dvx*5.0f;
    int maskL = (uL > 0) + ((vL > 0)<<1) + ((uL + vL < 1)<<2);
    int maskR = (uR > 0) + ((vR > 0)<<1) + ((uR + vR < 1)<<2);
    if(maskL==maskR){
        if(maskL==7){
            int fullw = scr_w*2;
          //  float depthL = vL*v3.z + uL*v2.z + (1-uL-vL)*v1.z, depthR = vR*v3.z + uR*v2.z + (1-uR-vR)*v1.z;
            float depth = (vL+vR)*z3 + (uL+uR)*z2 + (2-uL-vL-uR-vR)*z1;//(depthL+depthR)/2;
            int index = xL+scr_w + (y+scr_h)*fullw;
            while(xL<=xR){
                if(depthArray[index] < depth){
                    depthArray[index] = depth;
                    colorArray[index] = color;
                }
                ++xL;
                ++index;
            }
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
