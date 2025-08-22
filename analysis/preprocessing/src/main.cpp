#include "preprocessing/mesh_fill.h"
#include <iostream>
#include <cstring>

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input_file> <output_file> [options]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --max-hole-size <n>  Maximum hole size to fill (boundary edges)" << std::endl;
    std::cerr << "  --quiet              Suppress verbose output" << std::endl;
    std::cerr << "  --help               Show this help message" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Example:" << std::endl;
    std::cerr << "  " << program_name << " input.obj output.obj --max-hole-size 100" << std::endl;
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 检查是否请求帮助
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int max_hole_size = -1;
    bool verbose = true;
    
    // 解析可选参数
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--max-hole-size") == 0 && i + 1 < argc) {
            max_hole_size = std::atoi(argv[++i]);
        } else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            verbose = false;
        }
    }
    
    // 使用便利的静态方法处理
    auto result = preprocessing::MeshFill::processFile(
        input_file, output_file, max_hole_size, verbose
    );
    
    if (!result.success) {
        if (!result.error_message.empty()) {
            std::cerr << "Error: " << result.error_message << std::endl;
        }
        return 1;
    }
    
    // 输出统计信息（如果不是静默模式）
    if (verbose) {
        std::cout << "\n=== 填洞处理完成 ===" << std::endl;
        std::cout << "初始洞数: " << result.initial_stats.num_holes << std::endl;
        std::cout << "剩余洞数: " << result.final_stats.num_holes << std::endl;
        
        int filled_count = 0;
        int failed_count = 0;
        for (const auto& hole : result.holes) {
            if (hole.success) filled_count++;
            else failed_count++;
        }
        
        std::cout << "成功填补: " << filled_count << " 个洞" << std::endl;
        if (failed_count > 0) {
            std::cout << "失败: " << failed_count << " 个洞" << std::endl;
        }
        
        std::cout << "新增面数: " << 
                     (result.final_stats.num_faces - result.initial_stats.num_faces) << 
                     std::endl;
    }
    
    return 0;
}