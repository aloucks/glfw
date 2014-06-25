link_libraries(glfw ${glfw_LIBRARIES})

set(TINYCTHREAD ${GLFW_SOURCE_DIR}/deps/tinycthread.h
                ${GLFW_SOURCE_DIR}/deps/tinycthread.c)

if (BUILD_SHARED_LIBS)
    add_definitions(-DGLFW_DLL)
    link_libraries(${OPENGL_gl_LIBRARY} ${MATH_LIBRARY})
else()
    link_libraries(${glfw_LIBRARIES})
endif()

include_directories(${GLFW_SOURCE_DIR}/include
                    ${GLFW_SOURCE_DIR}/deps
                    ${CMAKE_CURRENT_BINARY_DIR}/Debug/generated/
                    ${D3D_SDK_PATH}/Include)

if (MSVC10) 
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        link_directories(${D3D_SDK_PATH}/Lib/x64)
    else()
        link_directories(${D3D_SDK_PATH}/Lib/x86)
    endif()
endif()
                    
add_executable(d3d-window-create d3d-window-create.cpp)
add_executable(d3d-window-create-multithreaded d3d-window-create-multithreaded.cpp ${TINYCTHREAD})
add_executable(d3d-cube d3d-cube.cpp d3d-cube.vs.hlsl d3d-cube.ps.hlsl ${TINYCTHREAD})             

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Debug/generated)

add_custom_command(
    TARGET d3d-cube
    PRE_BUILD
    COMMAND ${FXC} /T vs_5_0 /E VS /Od /Zi /Fh ${CMAKE_CURRENT_BINARY_DIR}/Debug/generated/d3d-cube.vs.h d3d-cube.vs.hlsl
    DEPENDS ${PROJECT_SOURCE_DIR}/d3d-cube.vs.hlsl
)

add_custom_command(
    TARGET d3d-cube
    PRE_BUILD
    COMMAND ${FXC} /T ps_5_0 /E PS /Od /Zi /Fh ${CMAKE_CURRENT_BINARY_DIR}/Debug/generated/d3d-cube.ps.h d3d-cube.ps.hlsl
    DEPENDS ${PROJECT_SOURCE_DIR}/d3d-cube.ps.hlsl
)