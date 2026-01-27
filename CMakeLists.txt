cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Ogre
find_package(fmt CONFIG REQUIRED)
    
file(GLOB_RECURSE  SOURCES "src/*.cpp")

add_library(fog-util ${SOURCES})


target_link_libraries(fog-util
    PUBLIC    
    PRIVATE        
        fmt::fmt
)
target_compile_options(fog-util PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-finput-charset=UTF-8 -fexec-charset=UTF-8>
)

target_include_directories(fog-util
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src  #
)
