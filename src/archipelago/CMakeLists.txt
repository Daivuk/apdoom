project(archipelago)
add_subdirectory(../../APCpp APCpp)
add_library(${PROJECT_NAME} 
    apdoom.h apdoom.cpp
    apdoom_def_types.h
    apdoom_def.h apdoom_c_def.h
    apdoom2_def.h apdoom2_c_def.h
    apheretic_def.h apheretic_c_def.h
    apdoom_open_world_def.h apdoom_open_world_c_def.h
)
target_include_directories(${PROJECT_NAME} PRIVATE ../../APCpp)
target_link_libraries(${PROJECT_NAME} APCpp)
