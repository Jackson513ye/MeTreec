#ifndef PIPELINE_CONFIG_H
#define PIPELINE_CONFIG_H

#define PIPELINE_VERSION "1.0.0"
#define PIPELINE_BUILD_TYPE "Release"

#ifdef ADTREE_DEFAULT_PATH
    #define HAS_ADTREE_PATH 1
    constexpr const char* DEFAULT_ADTREE_PATH = ADTREE_DEFAULT_PATH;
#else
    #define HAS_ADTREE_PATH 0
    constexpr const char* DEFAULT_ADTREE_PATH = nullptr;
#endif

#endif // PIPELINE_CONFIG_H
