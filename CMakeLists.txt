cmake_minimum_required(VERSION 3.10)
project(GPU_SDL_Drawing LANGUAGES CXX)

find_package(OpenCL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(app
    demo/main.cpp
    demo/shapes/shape3d.cpp
    src/rendering2.cpp
    src/util.cpp
)

target_include_directories(app PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/shapes"
)

target_include_directories(app PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${OpenCL_LIBRARIES})
target_include_directories(app PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${SDL2_LIBRARIES})
target_include_directories(app PRIVATE ${SDL2_ttf_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${SDL2_ttf_LIBRARIES})
target_link_libraries(app PRIVATE SDL2_ttf::SDL2_ttf)