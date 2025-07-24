cmake_minimum_required(VERSION 3.10)
project(GPU_SDL_Drawing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")


find_package(OpenCL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)


add_executable(demo_minecraft
    demo/demo_minecraft.cpp
    demo/shapes/shape3d.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
    src/log.cpp
    src/camera.cpp
)

add_executable(demo_towers
    demo/demo_towers.cpp
    demo/shapes/shape3d.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
    src/log.cpp
    src/camera.cpp
)

add_executable(test_buffers
    demo/test_buffers.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
    src/log.cpp
    src/camera.cpp
)

add_executable(vertex_buffer_demo
    demo/vertex_buffer_demo.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
    src/log.cpp
    src/camera.cpp
)

add_executable(binning_demo
    demo/binning_demo.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
    src/log.cpp
    src/camera.cpp
)


target_include_directories(demo_minecraft PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/shapes"
)

target_include_directories(demo_towers PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/shapes"
)

target_include_directories(test_buffers PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(vertex_buffer_demo PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(binning_demo PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


add_compile_options("-Ofast")


target_include_directories(demo_minecraft PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(demo_minecraft PRIVATE ${OpenCL_LIBRARIES})
target_include_directories(demo_minecraft PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(demo_minecraft PRIVATE ${SDL2_LIBRARIES})
target_include_directories(demo_minecraft PRIVATE ${SDL2_ttf_INCLUDE_DIRS})
target_link_libraries(demo_minecraft PRIVATE ${SDL2_ttf_LIBRARIES})
target_link_libraries(demo_minecraft PRIVATE SDL2_ttf::SDL2_ttf)

target_include_directories(demo_towers PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(demo_towers PRIVATE ${OpenCL_LIBRARIES})
target_include_directories(demo_towers PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(demo_towers PRIVATE ${SDL2_LIBRARIES})
target_include_directories(demo_towers PRIVATE ${SDL2_ttf_INCLUDE_DIRS})
target_link_libraries(demo_towers PRIVATE ${SDL2_ttf_LIBRARIES})
target_link_libraries(demo_towers PRIVATE SDL2_ttf::SDL2_ttf)

target_include_directories(test_buffers PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(test_buffers PRIVATE ${OpenCL_LIBRARIES})

target_include_directories(vertex_buffer_demo PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(vertex_buffer_demo PRIVATE ${OpenCL_LIBRARIES})
target_include_directories(vertex_buffer_demo PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(vertex_buffer_demo PRIVATE ${SDL2_LIBRARIES})
target_include_directories(vertex_buffer_demo PRIVATE ${SDL2_ttf_INCLUDE_DIRS})
target_link_libraries(vertex_buffer_demo PRIVATE ${SDL2_ttf_LIBRARIES})
target_link_libraries(vertex_buffer_demo PRIVATE SDL2_ttf::SDL2_ttf)

target_include_directories(binning_demo PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(binning_demo PRIVATE ${OpenCL_LIBRARIES})