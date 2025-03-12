void barycentric(float3 *p, float3 *v1, float3 *v2, float3 *v3, float *u, float *v) {
    *u = (-p->x * v1->y + p->x * v3->y + p->y * v1->x - p->y * v3->x - v1->x * v3->y + v1->y * v3->x) ;
    *v = (p->x * v1->y - p->x * v2->y - p->y * v1->x + p->y * v2->x + v1->x * v2->y - v1->y * v2->x);
}

__kernel void draw(__global float *depthArray, __global int *colorArray, float3 v1, float3 v2, float3 v3, int minX, int minY, int scr_w, int scr_h, int color, float3 du, float3 dv){
    int xL = minX + 4*get_global_id(0), xR = xL+3, y = minY + get_global_id(1);
    float3 pL = (float3)(xL,y,0);//, pR = (float3)(xR,y,0);
    /*float u,v;//, uR,vR;
    barycentric(&pL,&v1,&v2,&v3,&u,&v);*/
    float u = du.x + xL*du.y + y*du.z;
    float v = dv.x + xL*dv.y + y*dv.z;
    
   //u*=inv;
   // v*=inv;
   int fullw= (scr_w<<1);
    while(xL <= xR){
        if(u+v<1.0f && u>0 && v>0){
            int index = xL+scr_w + (y+scr_h)*fullw;
            float d = (1-u-v)*v1.z + u*v2.z + v*v3.z;
            if(depthArray[index] < d){
                depthArray[index] = d;
                colorArray[index] = color;
            }
        }
        ++xL;
        u += du.y;
        v += dv.y;
    }
}

__kernel void clear(__global float* depthBuffer,__global int* colorArray, int screen_width) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index = y * screen_width + x;
    depthBuffer[index] = -1000000000000.0f;
    colorArray[index] = 255<<24; //  black
}
