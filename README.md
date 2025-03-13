# Basic Hardware Renderer
Proof of concept. Can draw colored 3D triangles using the GPU with self-made OpenCL scripts that use barycentric coords and interpolation to compute Z-buffer values. 
The main part is in ```src/``` and ```include/```. It's my static linked library that handles all the rendering.
There's a ```demo``` folder with ```main.cpp``` and some helper files. You can walk around a scene with randomly spawned 3D towers.
![image](https://github.com/user-attachments/assets/35dde0e4-a13f-437c-a290-200cabee1cc4)
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
The whole point of this project for me was to challenge myself and to do as LITTLE research about 3D rendering as possible. With very basic knowledge (mostly learning what a Z-buffer is) I derived all the formulas myself. In the furture I will update this README with my derivations (After completing the project, I found my math approach was somewhat nonstandard)
