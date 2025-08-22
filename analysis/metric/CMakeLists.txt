cmake_minimum_required(VERSION 3.14)
project(metric VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找CGAL（CR和volume模块需要）
find_package(CGAL REQUIRED)

# 查找Boost（CGAL的依赖）
find_package(Boost REQUIRED)

# 检查是否有happly.h
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/metric/happly.h")
    set(HAS_HAPPLY TRUE)
    message(STATUS "Found happly.h - PLY support enabled")
else()
    set(HAS_HAPPLY FALSE)
    message(STATUS "happly.h not found - PLY support disabled")
endif()

# 创建metric库
add_library(metric
    src/height.cpp
    src/CD.cpp
    src/DBH.cpp
    src/CR.cpp      # 冠幅半径模块
    src/volume.cpp  # 体积/材积模块
)

# 设置目标属性
target_include_directories(metric
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接CGAL和相关库
target_link_libraries(metric
    PUBLIC
        CGAL::CGAL
)

# 如果有happly，添加定义
if(HAS_HAPPLY)
    target_compile_definitions(metric PRIVATE HAS_HAPPLY)
endif()

# 设置编译选项
target_compile_features(metric PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(metric PRIVATE /W4)
    target_compile_definitions(metric PRIVATE NOMINMAX)
else()
    target_compile_options(metric PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 为库设置版本
set_target_properties(metric PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建测试可执行文件（可选）
option(BUILD_METRIC_TESTS "Build metric tests" OFF)

if(BUILD_METRIC_TESTS)
    # 测试CR模块
    add_executable(test_cr tests/test_cr.cpp)
    target_link_libraries(test_cr PRIVATE metric)
    
    # 测试volume模块
    add_executable(test_volume tests/test_volume.cpp)
    target_link_libraries(test_volume PRIVATE metric)
    
    set_target_properties(test_cr test_volume PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# 安装库和头文件
install(TARGETS metric
    EXPORT metricTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 导出targets
install(EXPORT metricTargets
    FILE metricTargets.cmake
    NAMESPACE metric::
    DESTINATION lib/cmake/metric
)

# 创建配置文件
include(CMakePackageConfigHelpers)

# 创建cmake目录（如果不存在）
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 创建配置模板文件（如果不存在）
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/metricConfig.cmake.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/metricConfig.cmake.in"
"@PACKAGE_INIT@

include(\"\${CMAKE_CURRENT_LIST_DIR}/metricTargets.cmake\")

# 查找依赖
find_package(CGAL REQUIRED)
")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/metricConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/metricConfig.cmake"
    INSTALL_DESTINATION lib/cmake/metric
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/metricConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/metricConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/metricConfigVersion.cmake"
    DESTINATION lib/cmake/metric
)

# 显示配置信息
message(STATUS "Metric module configuration:")
message(STATUS "  Build tests:     ${BUILD_METRIC_TESTS}")
message(STATUS "  Modules enabled:")
message(STATUS "    - Height calculation (h_t)")
message(STATUS "    - Crown depth (CD)")
message(STATUS "    - DBH (Diameter at Breast Height)")
message(STATUS "    - Crown Radius (CR)")
message(STATUS "    - Volume calculation")
message(STATUS "  PLY support:     ${HAS_HAPPLY}")