cmake_minimum_required(VERSION 3.14)
project(TreePipeline VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找preprocessing和metric库
if(NOT TARGET preprocessing)
    find_package(preprocessing REQUIRED)
endif()

if(NOT TARGET metric)
    find_package(metric REQUIRED)
endif()

# 创建pipeline可执行文件
add_executable(TreePipeline
    src/main.cpp
)

# 设置include路径
target_include_directories(TreePipeline
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接preprocessing和metric库
if(TARGET preprocessing AND TARGET metric)
    # 如果是同一构建中的目标
    target_link_libraries(TreePipeline
        PRIVATE
            preprocessing
            metric
    )
else()
    # 如果是外部包
    target_link_libraries(TreePipeline
        PRIVATE
            preprocessing::preprocessing
            metric::metric
    )
endif()

# 根据平台设置文件系统库
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(TreePipeline PRIVATE stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(TreePipeline PRIVATE c++fs)
endif()

# 设置编译选项
if(MSVC)
    target_compile_options(TreePipeline PRIVATE /W4)
    # Windows下需要定义一些宏以避免min/max冲突
    target_compile_definitions(TreePipeline PRIVATE NOMINMAX)
else()
    target_compile_options(TreePipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 设置输出路径
set_target_properties(TreePipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# 安装规则
install(TARGETS TreePipeline
    RUNTIME DESTINATION bin
)

# 创建启动脚本（可选）
if(UNIX)
    # 创建scripts目录（如果不存在）
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
    
    # 创建脚本模板（如果不存在）
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_pipeline.sh.in")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_pipeline.sh.in"
"#!/bin/bash
# TreePara Pipeline Runner
SCRIPT_DIR=\"\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\")\" && pwd)\"
export LD_LIBRARY_PATH=\"\${SCRIPT_DIR}/../lib:\$LD_LIBRARY_PATH\"
exec \"\${SCRIPT_DIR}/TreePipeline\" \"\$@\"
")
    endif()
    
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_pipeline.sh.in"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_pipeline.sh"
        @ONLY
    )
    
    file(CHMOD "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_pipeline.sh" 
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                     GROUP_READ GROUP_EXECUTE 
                     WORLD_READ WORLD_EXECUTE)
    
    install(PROGRAMS
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_pipeline.sh"
        DESTINATION bin
    )
endif()

# 将TreePara根目录路径作为编译时常量
target_compile_definitions(TreePipeline PRIVATE
    TREEPARA_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# 查找AdTree可执行文件（用于运行时检查）
find_program(ADTREE_EXECUTABLE 
    NAMES AdTree
    PATHS 
        ${CMAKE_BINARY_DIR}/bin
        ${CMAKE_SOURCE_DIR}/reconstruction/AdTree/build/bin/AdTree.app/Contents/MacOS  # macOS .app bundle
        ${CMAKE_SOURCE_DIR}/reconstruction/AdTree/build/bin
        ${CMAKE_SOURCE_DIR}/reconstruction/AdTree/build
        ${CMAKE_SOURCE_DIR}/reconstruction/AdTree
    NO_DEFAULT_PATH
)

if(ADTREE_EXECUTABLE)
    message(STATUS "Found AdTree executable: ${ADTREE_EXECUTABLE}")
    # 可以将路径作为编译时常量传递给程序
    target_compile_definitions(TreePipeline PRIVATE
        ADTREE_DEFAULT_PATH="${ADTREE_EXECUTABLE}"
    )
else()
    message(STATUS "AdTree executable not found. Pipeline will search at runtime.")
    message(STATUS "  To build AdTree separately:")
    message(STATUS "    cd ${CMAKE_SOURCE_DIR}/reconstruction/AdTree")
    message(STATUS "    mkdir build && cd build")
    message(STATUS "    cmake .. && make")
    message(STATUS "  Or specify path at runtime with --adtree-exe option")
    if(APPLE)
        message(STATUS "  On macOS, AdTree is typically at:")
        message(STATUS "    reconstruction/AdTree/build/bin/AdTree.app/Contents/MacOS/AdTree")
    endif()
endif()

# 配置文件（用于记录构建信息）
# 创建config目录（如果不存在）
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config")

# 创建配置模板（如果不存在）
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/pipeline_config.h.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/config/pipeline_config.h.in"
"#ifndef PIPELINE_CONFIG_H
#define PIPELINE_CONFIG_H

#define PIPELINE_VERSION \"@PROJECT_VERSION@\"
#define PIPELINE_BUILD_TYPE \"@CMAKE_BUILD_TYPE@\"

#ifdef ADTREE_DEFAULT_PATH
    #define HAS_ADTREE_PATH 1
    constexpr const char* DEFAULT_ADTREE_PATH = ADTREE_DEFAULT_PATH;
#else
    #define HAS_ADTREE_PATH 0
    constexpr const char* DEFAULT_ADTREE_PATH = nullptr;
#endif

#endif // PIPELINE_CONFIG_H
")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/pipeline_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pipeline_config.h"
    @ONLY
)

target_include_directories(TreePipeline PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 测试支持（可选）
option(BUILD_PIPELINE_TESTS "Build pipeline tests" OFF)
if(BUILD_PIPELINE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 显示配置信息
message(STATUS "Pipeline module configuration:")
message(STATUS "  Output name:     TreePipeline")
message(STATUS "  AdTree found:    ${ADTREE_EXECUTABLE}")
message(STATUS "  Build tests:     ${BUILD_PIPELINE_TESTS}")