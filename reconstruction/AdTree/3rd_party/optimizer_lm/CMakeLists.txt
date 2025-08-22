# Add include directory for cminpack headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cminpack)

# Original content with include fix
add_library(3rd_optimizer_lm STATIC optimizer_lm.cpp optimizer_lm.h)

# Link with cminpack
target_link_libraries(3rd_optimizer_lm 3rd_cminpack)

# Ensure cminpack is built first
add_dependencies(3rd_optimizer_lm 3rd_cminpack)
