cmake_minimum_required(VERSION 3.10)

project(MiProyectoGrafico VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/imgui  
    ${CMAKE_SOURCE_DIR}/src        
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c" 
    "src/*.cc"
)

add_executable(MiApp ${SOURCES})

target_link_libraries(MiApp 
    PRIVATE
    glfw3     
    opengl32       
    gdi32    
)

configure_file(${CMAKE_SOURCE_DIR}/scene.txt ${CMAKE_BINARY_DIR}/scene.txt COPYONLY)