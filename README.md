# Basic Hardware Renderer
Proof of concept. Can draw colored 3D triangles using the GPU with self-made OpenCL scripts that use barycentric coords and interpolation to compute depth buffer values. \
The main part is in ```src/``` and ```include/```. It's my static linked library that handles all the rendering.\
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
To run, just navigate to ```build``` directory and run:
```bash
./app 
```
## My idea
My goal was to challenge myself and do as LITTLE research about 3D rendering as possible. With very basic knowledge (like, what a depth buffer is) I derived all the formulas myself. In the furture I might add my derivations to this README. I think approach was somewhat nonstandard. \\
The library is in ```src\``` and ```include\```. Rendering a colored 3D triangle onto a bitmap is the main feature it provides. The ```util``` file has some basic CPU-side math functions, color utilities and the only math objects in this renderer: ```tri``` and ```vec``` - 3D triangle and 3D vector. \
There ```rednering``` file has a class ```GPU``` that abstracts away the whole OpenCL setup, like creating a context, platform and finding the GPU. The ```DepthBuffer``` class creates a bitmap that stores the color of every pixel on the screen, as well as a depth buffer. In a given frame, the first ```drawTriangle``` call first clears the color and depth buffers. To inform that the frame is finished and to make changes visible, you simply call ```finishFrame()```. That's it.
