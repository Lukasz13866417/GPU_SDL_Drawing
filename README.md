# Hardware Renderer from scratch with Open Computing Language (OpenCL)
> For continuation, visit [here](https://github.com/Lukasz13866417/HardwareRenderer)

This is the **simplest, most straightforward way** to do 3D rendering on the GPU without any renderer like OpenGL, Vulkan etc. \
This API can draw colored 3D triangles using the GPU with self-made OpenCL scripts (kernels) \
The main part is in ```src/``` and ```include/```. It's my library that handles all the rendering.\
There's a ```demo``` folder with two demos: ```demo_towers.cpp``` and ```demo_minecraft.cpp``` and some helper files. 
### Demo Minecraft


You can move around a scene with a minecraft dirt block that's spinning
### Demo Towers
You can walk around a scene with randomly spawned 3D towers.
![image](https://github.com/user-attachments/assets/1e715f8a-d027-4e30-a6a5-72deed56b24a)

## Build & Run
You must first install SDL and OpenCL on your system. On Debian-style systems, this worked for me:
```bash
sudo apt-get install opencl-headers
sudo apt-get install ocl-icd-libopencl1
sudo apt install ocl-icd-opencl-dev
sudo apt install libsdl2-dev libsdl2-2.0-0 -y
sudo apt-get install libsdl2-ttf-dev
```
The vendor-specific (NVidia/Intel/AMD) implementation of OpenCL might already be on your machine, but if it's not, you need to install it too. Without it, the app will crash on launch and say that it can't find an OpenCL platform. <br>
After installation, just go to project root directory and build with CMake:
```bash
mkdir build && cd build
cmake ..
make
```
To run, navigate to ```build``` directory and run:
```bash
./app 
```
## Overview
The library is in ```src\``` and ```include\```. Rendering a colored 3D triangle onto a bitmap is the main feature it provides. <br>The ```util``` file has some basic CPU-side math functions, color utilities and the only math objects in this renderer: ```tri``` and ```vec``` - 3D triangle and 3D vector. <br><br>
The ```rendering``` file:
- Has a class ```GPU``` that abstracts away the whole OpenCL setup, like creating a context, platform and finding the GPU. 
- Most importantly, there's the ```DepthBuffer``` class that has a bitmap that stores the color of every pixel on the screen, as well as a depth buffer. 
- All of 3D rendering in this project is done with DepthBuffer's ```drawTriangle()``` method. 
- To fetch results after you're done with 3D rendering in a given frame, you simply call ```DepthBuffer```'s ```finishFrame()```. This method returns the color bitmap for this frame. You can draw it onto the screen with libs like SDL (used here). 
- You can also ```clear``` the buffer (e.g. at the beginning of 3D rendering in a given frame).
## My idea
My goal was to challenge myself and do as LITTLE research about 3D rendering as possible. I didn't even know how a 3D point is projected onto a screen. With very basic knowledge (like, what a depth buffer is) I derived all the formulas myself. <br><br>
This resulted in **a very concise, self-contained** project that **doesn't confuse the user** by doing unnecessary stuff. <br><br> In the future I might add my derivations to this README. I think my approach might have been somewhat nonstandard. <br><br>
