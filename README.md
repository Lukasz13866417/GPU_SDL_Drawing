# Basic Hardware Renderer
Mostly proof of concept Can draw colored 3D triangles using the GPU with self-made OpenCL scripts that use barycentric coords and interpolation to compute Z-buffer values. 
There's a demo in ```main.cpp```. You can walk around a scene with rotating 3D towers.
![image](https://github.com/user-attachments/assets/facabfe3-f67e-4228-8c19-6ff5d7ba09a7)

## Build & Run
You must first install SDL and OpenCL on your system. On Debian-style systems, this worked for me:
```bash
sudo apt-get install opencl-headers
sudo apt-get install ocl-icd-libopencl1
sudo apt install libsdl2-dev libsdl2-2.0-0 -y
```
Then, just build with CMake:
```bash
mkdir build && cd build
cmake ..
make
```
To run,
```bash
./app
```
Or if you're not currently in ```build```:
```bash
cd build
./app
```
