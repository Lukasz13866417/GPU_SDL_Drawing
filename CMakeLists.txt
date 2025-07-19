cmake_minimum_required(VERSION 3.10)
project(GPU_SDL_Drawing LANGUAGES CXX)


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
)

add_executable(demo_towers
    demo/demo_towers.cpp
    demo/shapes/shape3d.cpp
    src/rendering2.cpp
    src/texture.cpp
    src/util.cpp
)


target_include_directories(demo_minecraft PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/shapes"
)

target_include_directories(demo_towers PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/shapes"
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