# cmake/modules/FindSDL2_ttf.cmake
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
          PATH_SUFFIXES SDL2
          PATHS
            /usr/include/SDL2
            /usr/local/include/SDL2)

find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2_ttf
                                  REQUIRED_VARS SDL2_TTF_LIBRARY SDL2_TTF_INCLUDE_DIR
                                  FOUND_VAR SDL2_TTF_FOUND)

if(SDL2_TTF_FOUND)
    set(SDL2_ttf_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIR})
    set(SDL2_ttf_LIBRARIES ${SDL2_TTF_LIBRARY})
    if(NOT TARGET SDL2_ttf::SDL2_ttf)
        add_library(SDL2_ttf::SDL2_ttf UNKNOWN IMPORTED)
        set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
            IMPORTED_LOCATION "${SDL2_TTF_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}")
    endif()
endif()
