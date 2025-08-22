cmake_minimum_required(VERSION 3.14)
project(TreePara VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 输出路径设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 选项
option(BUILD_ADTREE "Build AdTree reconstruction module" OFF)  # 默认关闭
option(BUILD_PREPROCESSING "Build preprocessing module" ON)
option(BUILD_METRIC "Build metric module" ON)
option(BUILD_PIPELINE "Build pipeline orchestrator" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 添加子项目
if(BUILD_ADTREE)
    # AdTree有自己复杂的构建系统，建议独立构建
    message(WARNING "AdTree integration is experimental. Recommend building AdTree separately.")
    # 检查AdTree是否有CMakeLists.txt
    if(EXISTS "${CMAKE_SOURCE_DIR}/reconstruction/AdTree/CMakeLists.txt")
        # 使用ExternalProject来隔离构建
        include(ExternalProject)
        ExternalProject_Add(AdTree_Build
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/reconstruction/AdTree
            BINARY_DIR ${CMAKE_BINARY_DIR}/AdTree-build
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            BUILD_COMMAND ${CMAKE_COMMAND} --build .
            INSTALL_COMMAND ""
            BUILD_ALWAYS OFF
        )
        message(STATUS "AdTree will be built as an external project")
    else()
        message(WARNING "AdTree CMakeLists.txt not found. AdTree will need to be built separately.")
        message(STATUS "  Expected location: ${CMAKE_SOURCE_DIR}/reconstruction/AdTree/CMakeLists.txt")
    endif()
else()
    message(STATUS "AdTree integration disabled. Build AdTree separately in reconstruction/AdTree/")
    message(STATUS "  cd reconstruction/AdTree && mkdir build && cd build && cmake .. && make")
endif()

if(BUILD_PREPROCESSING)
    add_subdirectory(analysis/preprocessing)
endif()

if(BUILD_METRIC)
    add_subdirectory(analysis/metric)
endif()

if(BUILD_PIPELINE)
    if(NOT BUILD_PREPROCESSING)
        message(FATAL_ERROR "Pipeline requires preprocessing module. Please enable BUILD_PREPROCESSING.")
    endif()
    if(NOT BUILD_METRIC)
        message(FATAL_ERROR "Pipeline requires metric module. Please enable BUILD_METRIC.")
    endif()
    add_subdirectory(pipeline)
endif()

# 安装规则
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION lib)

# 导出配置
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TreeParaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# 显示配置摘要
message(STATUS "")
message(STATUS "========== TreePara Configuration ==========")
message(STATUS "Version:            ${PROJECT_VERSION}")
message(STATUS "Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:       ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Modules:")
message(STATUS "  AdTree:           ${BUILD_ADTREE}")
message(STATUS "  Preprocessing:    ${BUILD_PREPROCESSING}")
message(STATUS "  Metric:           ${BUILD_METRIC}")
message(STATUS "  Pipeline:         ${BUILD_PIPELINE}")
message(STATUS "")
message(STATUS "Output directories:")
message(STATUS "  Binaries:         ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Libraries:        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "============================================")
message(STATUS "")