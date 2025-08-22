cmake_minimum_required(VERSION 3.14)
project(preprocessing VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(CGAL REQUIRED)
find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)

# 创建库
add_library(preprocessing
    src/mesh_fill.cpp
    src/structure_extract.cpp
)

# 设置目标属性
target_include_directories(preprocessing
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 处理Eigen3的不同目标名称
if(TARGET Eigen3::Eigen)
    set(EIGEN_TARGET Eigen3::Eigen)
else()
    # 旧版本或Homebrew安装的Eigen可能没有导出目标
    set(EIGEN_TARGET "")
    target_include_directories(preprocessing PUBLIC ${EIGEN3_INCLUDE_DIRS})
endif()

# 处理Boost的不同目标名称
if(TARGET Boost::boost)
    set(BOOST_TARGET Boost::boost)
elseif(TARGET Boost::headers)
    set(BOOST_TARGET Boost::headers)
else()
    set(BOOST_TARGET "")
    target_include_directories(preprocessing PRIVATE ${Boost_INCLUDE_DIRS})
endif()

# 链接CGAL和其他依赖
target_link_libraries(preprocessing
    PUBLIC
        CGAL::CGAL
)

if(EIGEN_TARGET)
    target_link_libraries(preprocessing PUBLIC ${EIGEN_TARGET})
endif()

if(BOOST_TARGET)
    target_link_libraries(preprocessing PRIVATE ${BOOST_TARGET})
endif()

# 设置编译选项
target_compile_features(preprocessing PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(preprocessing PRIVATE /W4)
    target_compile_definitions(preprocessing PRIVATE NOMINMAX)
else()
    target_compile_options(preprocessing PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 为库设置版本
set_target_properties(preprocessing PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 创建mesh_fill命令行工具
option(BUILD_PREPROCESSING_CLI "Build preprocessing command line tool" ON)

if(BUILD_PREPROCESSING_CLI)
    add_executable(preprocessing_cli
        src/main.cpp
    )
    
    target_link_libraries(preprocessing_cli
        PRIVATE
            preprocessing
    )
    
    set_target_properties(preprocessing_cli PROPERTIES
        OUTPUT_NAME preprocessing
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    
    # 安装CLI工具
    install(TARGETS preprocessing_cli
        RUNTIME DESTINATION bin
    )
endif()

# 骨架处理功能已集成到preprocessing库中
# 可通过pipeline或其他工具调用StructureExtractor类

# 安装库和头文件
install(TARGETS preprocessing
    EXPORT preprocessingTargets
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
install(EXPORT preprocessingTargets
    FILE preprocessingTargets.cmake
    NAMESPACE preprocessing::
    DESTINATION lib/cmake/preprocessing
)

# 创建配置文件
include(CMakePackageConfigHelpers)

# 首先创建cmake目录（如果不存在）
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 创建配置模板文件（如果不存在）
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/preprocessingConfig.cmake.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/preprocessingConfig.cmake.in"
"@PACKAGE_INIT@

include(CMakeFindDependencyMacro)
find_dependency(CGAL)
find_dependency(Eigen3)
find_dependency(Boost)

# Handle different Eigen target names
if(NOT TARGET Eigen3::Eigen AND EIGEN3_INCLUDE_DIRS)
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES \"\${EIGEN3_INCLUDE_DIRS}\"
    )
endif()

include(\"\${CMAKE_CURRENT_LIST_DIR}/preprocessingTargets.cmake\")
")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/preprocessingConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/preprocessingConfig.cmake"
    INSTALL_DESTINATION lib/cmake/preprocessing
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/preprocessingConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/preprocessingConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/preprocessingConfigVersion.cmake"
    DESTINATION lib/cmake/preprocessing
)

# 检查happly.h是否存在
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/preprocessing/happly.h")
    message(STATUS "Found happly.h")
else()
    message(WARNING "happly.h not found at: ${CMAKE_CURRENT_SOURCE_DIR}/include/preprocessing/")
    message(WARNING "Please download from: https://github.com/nmwsharp/happly")
    message(WARNING "Direct link: https://raw.githubusercontent.com/nmwsharp/happly/master/happly.h")
    message(WARNING "And place it in: ${CMAKE_CURRENT_SOURCE_DIR}/include/preprocessing/")
endif()

# 显示配置信息
message(STATUS "Preprocessing module configuration:")
message(STATUS "  Build mesh_fill CLI:     ${BUILD_PREPROCESSING_CLI}")
message(STATUS "  CGAL found:              ${CGAL_FOUND}")
message(STATUS "  Eigen3 found:            ${Eigen3_FOUND}")
if(EIGEN_TARGET)
    message(STATUS "  Eigen target:            ${EIGEN_TARGET}")
else()
    message(STATUS "  Eigen include:           ${EIGEN3_INCLUDE_DIRS}")
endif()
if(BOOST_TARGET)
    message(STATUS "  Boost target:            ${BOOST_TARGET}")
else()
    message(STATUS "  Boost include:           ${Boost_INCLUDE_DIRS}")
endif()