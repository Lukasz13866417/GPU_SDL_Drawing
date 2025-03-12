typedef float3 vec3;

__kernel void draw(__global float* depthBuffer,__global int* colorArray,
                   vec3 v1, vec3 v2, vec3 v3, int clr,
                   int screen_width, int screen_height, int minX, int minY, float inv, float dx1, float dx2, float dy1, float dy2, float lambda1, float lambda2) {

    int tileW=3,tileH=3;
    //inv=1.0f/(v1.x*v2.y - v1.x*v3.y - v1.y*v2.x + v1.y*v3.x + v2.x*v3.y - v2.y*v3.x)
    int x = tileW*(get_global_id(0))+minX;
    int y = tileH*(get_global_id(1))+minY;

    //lambda1 = (-v1.x*v3.y + v1.y*v3.x)*inv;
    //lambda2 = (v1.x*v2.y - v1.y*v2.x)*inv;
    lambda1 += x*dx1 + y*dy1;
    lambda2 += x*dx2 + y*dy2;
    float old1 = lambda1, old2 = lambda2;
    //float dx1 = (v3.y - v1.y)*inv, dx2 = (v1.y - v2.y)*inv;
    //float dy1  = (v1.x - v3.x)*inv, dy2 = (v2.x - v1.x)*inv;
    
    if(!(lambda1>=0 && lambda2>=0 && lambda1+lambda2<=1)){
        lambda1 += dx1*(tileW-1);
        lambda2 += dx2*(tileW-1);
        if(!(lambda1>=0 && lambda2>=0 && lambda1+lambda2<=1)){
            lambda1 += dy1*(tileH-1);
            lambda2 += dy2*(tileH-1);
            if(!(lambda1>=0 && lambda2>=0 && lambda1+lambda2<=1)){
                lambda1 -= dx1*(tileW-1);
                lambda2 -= dx2*(tileW-1);
                if(!(lambda1>=0 && lambda2>=0 && lambda1+lambda2<=1)){
                    return;
                }    
            }
        }
    }

    int index = (y + (screen_height / 2)) * screen_width + (x + (screen_width / 2));
    lambda1 = old1;
    lambda2 = old2;

    int n=tileW*tileH;
    int level=0,tx=0,ty=tileH-1;
    for(int i=0;i<n;++i){
        if(lambda1>=0 && lambda2>=0 && lambda1+lambda2<=1){
            float f = lambda2*v3.z + lambda1*v2.z + (1-lambda1-lambda2)*v1.z;
            if (f > depthBuffer[index]) {
                depthBuffer[index] = f;
                colorArray[index] = clr;
            }
        }
        if( ( !level && tx==tileW-1 ) || ( level && tx==0 ) ){
            ++ty;
            index += screen_width;
            lambda1 += dy1;
            lambda2 += dy2;
            level^=1;
        }else if( !level ){
            ++tx;
            ++index;
            lambda1 += dx1;
            lambda2 += dx2;
        }else{
            --tx;
            --index;
            lambda1 -= dx1;
            lambda2 -= dx2;
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
