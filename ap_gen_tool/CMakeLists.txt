# Some compiler flags
set(CMAKE_CXX_STANDARD 17) # C++17
if (MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP /FS")
endif()

# Project name
project(ap_gen_tool)

#justwindowsthings
if (WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Onut
add_subdirectory(../onut onut)
list(APPEND libs PUBLIC libonut)
list(APPEND includes PUBLIC ./thirdparty/onut/include/)

# ${PROJECT_NAME}.exe, use WinMain on Windows
add_executable(${PROJECT_NAME} WIN32 
    generate.h
    generate.cpp
    open_world.cpp
    maps.h
    maps.cpp
    defs.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${includes})
target_link_libraries(${PROJECT_NAME} PUBLIC ${libs})
