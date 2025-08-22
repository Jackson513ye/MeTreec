#include "preprocessing/mesh_fill.h"
#include "preprocessing/structure_extract.h"
#include "metric/height.h"
#include "metric/CD.h"
#include "metric/DBH.h"
#include "metric/CR.h"      // 新增：冠幅半径
#include "metric/volume.h"   // 新增：体积/材积
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

// 树木指标结构
struct TreeMetrics {
    std::string tree_id;           // 树木ID（文件名）
    double height = 0.0;            // 树高
    double h0 = 0.0;                // 活冠基部高度
    double crown_depth = 0.0;       // 冠幅深度（活冠深度）
    double dbh = 0.0;               // 胸径（厘米）
    double crown_radius = 0.0;      // 冠幅半径
    double crown_diameter = 0.0;    // 冠幅直径
    double max_crown_width = 0.0;   // 最大冠幅
    double min_crown_width = 0.0;   // 最小冠幅
    double crown_aspect_ratio = 0.0;// 冠幅长宽比
    double volume = 0.0;            // 体积/材积
    double surface_area = 0.0;      // 表面积
    bool mesh_is_closed = false;    // 网格是否封闭
    int leaf_nodes_total = 0;       // 总叶节点数
    int leaf_nodes_filtered = 0;    // 筛选后叶节点数
    bool has_skeleton_data = false; // 是否有骨架数据
    std::string dbh_method;         // DBH计算方法
    
    // 处理时间戳
    std::string processing_time;
};

// 执行外部程序
int execute_command(const std::string& command) {
    return std::system(command.c_str());
}

// 查找AdTree可执行文件
std::string find_adtree() {
    std::vector<std::string> search_paths = {
        "../reconstruction/AdTree/build/bin",
        "../reconstruction/AdTree/build",
        "./bin", "."
    };
    
#ifdef __APPLE__
    for (const auto& base_path : search_paths) {
        fs::path app_exe = fs::path(base_path) / "AdTree.app/Contents/MacOS/AdTree";
        if (fs::exists(app_exe)) return app_exe.string();
    }
#endif
    
    for (const auto& path : search_paths) {
        fs::path exe_path = fs::path(path) / "AdTree";
        if (fs::exists(exe_path)) return exe_path.string();
#ifdef _WIN32
        exe_path = fs::path(path) / "AdTree.exe";
        if (fs::exists(exe_path)) return exe_path.string();
#endif
    }
    return "";
}

// Pipeline配置
struct Config {
    std::string input_path;
    std::string output_dir;         // 最终输出目录 (data/temp)
    std::string adtree_output_dir;  // AdTree输出目录 (data/output/models)
    std::string report_dir;         // 报告输出目录 (data/output/report)
    std::string adtree_exe;
    bool fill_holes = true;
    int max_hole_size = -1;
    bool use_default_paths = false;
    bool process_skeleton = true;   // 是否处理骨架
    double filter_ratio = 0.15;     // 叶节点筛选比例
    bool verbose = false;            // 详细输出
    bool calculate_volume = true;   // 是否计算体积
    bool calculate_crown = true;    // 是否计算冠幅
};

// 获取当前时间字符串
std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 生成单个树木的JSON报告
bool generate_single_json_report(const TreeMetrics& metrics, const std::string& report_dir) {
    // 生成文件名：tree_id_timestamp.json
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp;
    timestamp << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    fs::path json_path = fs::path(report_dir) / 
        (metrics.tree_id + "_" + timestamp.str() + ".json");
    
    std::ofstream json_file(json_path);
    if (!json_file.is_open()) {
        std::cerr << "无法创建JSON报告文件: " << json_path << std::endl;
        return false;
    }
    
    json_file << "{\n";
    json_file << "  \"tree_info\": {\n";
    json_file << "    \"id\": \"" << metrics.tree_id << "\",\n";
    json_file << "    \"processing_time\": \"" << metrics.processing_time << "\",\n";
    json_file << "    \"software\": \"MeTreec Pipeline v1.0\"\n";
    json_file << "  },\n";
    json_file << "  \"metrics\": {\n";
    json_file << "    \"height\": " << std::fixed << std::setprecision(3) << metrics.height << ",\n";
    json_file << "    \"h0_crown_base\": " << std::fixed << std::setprecision(3) << metrics.h0 << ",\n";
    json_file << "    \"crown_depth\": " << std::fixed << std::setprecision(3) << metrics.crown_depth << ",\n";
    json_file << "    \"dbh\": {\n";
    json_file << "      \"value_cm\": " << std::fixed << std::setprecision(2) << metrics.dbh << ",\n";
    json_file << "      \"method\": \"" << metrics.dbh_method << "\"\n";
    json_file << "    },\n";
    json_file << "    \"crown\": {\n";
    json_file << "      \"radius\": " << std::fixed << std::setprecision(3) << metrics.crown_radius << ",\n";
    json_file << "      \"diameter\": " << std::fixed << std::setprecision(3) << metrics.crown_diameter << ",\n";
    json_file << "      \"max_width\": " << std::fixed << std::setprecision(3) << metrics.max_crown_width << ",\n";
    json_file << "      \"min_width\": " << std::fixed << std::setprecision(3) << metrics.min_crown_width << ",\n";
    json_file << "      \"aspect_ratio\": " << std::fixed << std::setprecision(2) << metrics.crown_aspect_ratio << "\n";
    json_file << "    },\n";
    json_file << "    \"volume\": {\n";
    json_file << "      \"value_m3\": " << std::fixed << std::setprecision(3) << metrics.volume << ",\n";
    json_file << "      \"surface_area_m2\": " << std::fixed << std::setprecision(3) << metrics.surface_area << ",\n";
    json_file << "      \"mesh_closed\": " << (metrics.mesh_is_closed ? "true" : "false") << "\n";
    json_file << "    }\n";
    json_file << "  },\n";
    json_file << "  \"skeleton_info\": {\n";
    json_file << "    \"has_data\": " << (metrics.has_skeleton_data ? "true" : "false") << ",\n";
    json_file << "    \"total_leaf_nodes\": " << metrics.leaf_nodes_total << ",\n";
    json_file << "    \"filtered_leaf_nodes\": " << metrics.leaf_nodes_filtered << "\n";
    json_file << "  }\n";
    json_file << "}\n";
    
    json_file.close();
    std::cout << "     JSON报告已保存: " << json_path.filename() << std::endl;
    return true;
}

// 生成汇总CSV报告
bool generate_csv_report(const std::vector<TreeMetrics>& metrics_list, const std::string& csv_path) {
    std::ofstream csv_file(csv_path);
    if (!csv_file.is_open()) {
        std::cerr << "无法创建CSV报告文件: " << csv_path << std::endl;
        return false;
    }
    
    // CSV头
    csv_file << "Tree_ID,Processing_Time,Height,H0_Crown_Base,Crown_Depth,DBH_cm,DBH_Method,"
             << "Crown_Radius,Crown_Diameter,Max_Crown_Width,Min_Crown_Width,Aspect_Ratio,"
             << "Volume_m3,Surface_Area_m2,Mesh_Closed,"
             << "Has_Skeleton,Total_Leaf_Nodes,Filtered_Leaf_Nodes\n";
    
    // 数据行
    for (const auto& m : metrics_list) {
        csv_file << m.tree_id << ","
                 << m.processing_time << ","
                 << std::fixed << std::setprecision(3) << m.height << ","
                 << std::fixed << std::setprecision(3) << m.h0 << ","
                 << std::fixed << std::setprecision(3) << m.crown_depth << ","
                 << std::fixed << std::setprecision(2) << m.dbh << ","
                 << m.dbh_method << ","
                 << std::fixed << std::setprecision(3) << m.crown_radius << ","
                 << std::fixed << std::setprecision(3) << m.crown_diameter << ","
                 << std::fixed << std::setprecision(3) << m.max_crown_width << ","
                 << std::fixed << std::setprecision(3) << m.min_crown_width << ","
                 << std::fixed << std::setprecision(2) << m.crown_aspect_ratio << ","
                 << std::fixed << std::setprecision(3) << m.volume << ","
                 << std::fixed << std::setprecision(3) << m.surface_area << ","
                 << (m.mesh_is_closed ? "Yes" : "No") << ","
                 << (m.has_skeleton_data ? "Yes" : "No") << ","
                 << m.leaf_nodes_total << ","
                 << m.leaf_nodes_filtered << "\n";
    }
    
    csv_file.close();
    return true;
}

// 处理单个文件
TreeMetrics process_file(const std::string& xyz_file, const Config& config) {
    TreeMetrics metrics;
    
    fs::path input_path(xyz_file);
    std::string base_name = input_path.stem().string();
    
    metrics.tree_id = base_name;
    metrics.processing_time = get_current_time();
    std::string filtered_nodes_path;
    std::cout << "\n处理: " << base_name << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // AdTree输出目录（默认模式下使用 data/output/models）
    fs::path adtree_dir = config.use_default_paths && !config.adtree_output_dir.empty() ? 
        fs::path(config.adtree_output_dir) : fs::path(config.output_dir);
    
    // 步骤1: 运行AdTree重建
    std::cout << "  1. 运行AdTree重建..." << std::endl;
    std::string cmd = "\"" + config.adtree_exe + "\" \"" + 
                      xyz_file + "\" \"" + adtree_dir.string() + "\"";
    
    if (config.process_skeleton) {
        cmd += " -s";
    }
    
    if (config.verbose) {
        std::cout << "     命令: " << cmd << std::endl;
    }
    
    execute_command(cmd);
    
    // 检查AdTree输出文件
    fs::path branches_file = adtree_dir / (base_name + "_branches.obj");
    fs::path leaves_file = adtree_dir / (base_name + "_leaves.obj");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    fs::path skeleton_file = adtree_dir / (base_name + "_skeleton.ply");
    if (!fs::exists(skeleton_file)) {
        fs::path skeleton_obj = adtree_dir / (base_name + "_skeleton.obj");
        if (fs::exists(skeleton_obj)) skeleton_file = skeleton_obj;
    }
    
    if (!fs::exists(branches_file)) {
        std::cerr << "     错误: 未找到branches文件: " << branches_file << std::endl;
        return metrics;
    }
    
    std::cout << "     AdTree重建完成" << std::endl;
    
    // 删除leaves文件（不需要保存）
    if (fs::exists(leaves_file)) {
        fs::remove(leaves_file);
    }
    
    // 若存在骨架：复制到 data/temp（config.output_dir）
    if (fs::exists(skeleton_file)) {
        fs::path dst = fs::path(config.output_dir) / skeleton_file.filename();
        try {
            fs::copy_file(skeleton_file, dst, fs::copy_options::overwrite_existing);
            std::cout << "     已复制骨架到: " << dst << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "     警告: 复制骨架失败: " << e.what() << std::endl;
        }
    }
    
    // 步骤2: 填洞处理
    fs::path final_output_file;
    
    if (config.fill_holes) {
        std::cout << "  2. 进行网格填洞处理..." << std::endl;
        final_output_file = fs::path(config.output_dir) / (base_name + "_branches_filled.obj");
        
        auto result = preprocessing::MeshFill::processFile(
            branches_file.string(),
            final_output_file.string(),
            config.max_hole_size,
            config.verbose
        );
        
        if (result.success && result.initial_stats.num_holes > 0) {
            std::cout << "     填洞: " << result.initial_stats.num_holes << 
                        " -> " << result.final_stats.num_holes << " 洞" << std::endl;
        } else if (!result.success) {
            // 填洞失败，复制原始文件
            final_output_file = fs::path(config.output_dir) / (base_name + "_branches.obj");
            fs::copy_file(branches_file, final_output_file, fs::copy_options::overwrite_existing);
            std::cerr << "     填洞失败，使用原始文件" << std::endl;
        } else {
            std::cout << "     网格无需填洞" << std::endl;
        }
    } else {
        // 不填洞，直接复制
        std::cout << "  2. 跳过填洞处理" << std::endl;
        final_output_file = fs::path(config.output_dir) / (base_name + "_branches.obj");
        fs::copy_file(branches_file, final_output_file, fs::copy_options::overwrite_existing);
    }
    
    // 步骤3: 处理骨架数据（如果存在）
    if (config.process_skeleton && fs::exists(skeleton_file)) {
        std::cout << "  3. 处理骨架数据..." << std::endl;
        
        auto skeleton_result = preprocessing::StructureExtractor::filterLeafNodes(
            skeleton_file.string(),
            config.output_dir,
            config.filter_ratio,
            config.verbose
        );
        
        if (skeleton_result.success) {
            metrics.has_skeleton_data = true;
            metrics.leaf_nodes_total = skeleton_result.total_leaves;
            metrics.leaf_nodes_filtered = skeleton_result.filtered_leaves;
            
            std::cout << "     骨架处理完成:" << std::endl;
            std::cout << "       - 总叶节点: " << metrics.leaf_nodes_total << std::endl;
            std::cout << "       - 筛选后: " << metrics.leaf_nodes_filtered << std::endl;
            std::cout << "       - 输出文件: " << skeleton_result.output_file << std::endl;
            filtered_nodes_path = skeleton_result.output_file;
        } else {
            std::cerr << "     骨架处理失败: " << skeleton_result.error_message << std::endl;
        }
    } else if (config.process_skeleton) {
        std::cout << "  3. 未找到骨架文件，跳过骨架处理" << std::endl;
    }
    
    // 清理 data/output/models 中的原始骨架
    if (config.use_default_paths && !config.adtree_output_dir.empty()) {
        try {
            fs::path sk_ply = adtree_dir / (base_name + "_skeleton.ply");
            fs::path sk_obj = adtree_dir / (base_name + "_skeleton.obj");
            bool removed_any = false;
            if (fs::exists(sk_ply)) { fs::remove(sk_ply); removed_any = true; }
            if (fs::exists(sk_obj)) { fs::remove(sk_obj); removed_any = true; }
            if (removed_any && config.verbose) {
                std::cout << "  已从 AdTree 输出目录删除原始骨架文件" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  警告: 删除骨架文件失败: " << e.what() << std::endl;
        }
    }
    
    // 步骤4: 计算树木指标
    std::cout << "  4. 计算树木指标..." << std::endl;
    
    // 查找筛选后的叶节点文件
    fs::path filtered_xyz_guess = fs::path(config.output_dir) / (base_name + "_filtered.xyz");
    std::string filtered_path = !filtered_nodes_path.empty() ? filtered_nodes_path : filtered_xyz_guess.string();

    if (fs::exists(filtered_path)) {
        std::cout << "     高度/冠幅深度计算输入: " << filtered_path << std::endl;
        
        // 计算树高 h_t
        auto height_result = metric::TreeHeight::calculateFromFilteredNodes(
            filtered_path,
            5,  // 使用最高的5个点
            config.verbose
        );
        if (height_result.success) {
            metrics.height = height_result.tree_height;
            std::cout << "     树高 (h_t): " << std::fixed << std::setprecision(2)
                      << metrics.height << " m" << std::endl;

            // 计算冠幅深度 CD
            auto cd_result = metric::CrownDepth::calculateFromFilteredNodes(
                filtered_path,
                metrics.height,  // 使用刚计算的树高
                5,  // 使用最低的5个点计算h0
                config.verbose
            );
            if (cd_result.success) {
                metrics.h0 = cd_result.h0;
                metrics.crown_depth = cd_result.crown_depth;
                std::cout << "     活冠基部高度 (h0): " << std::fixed << std::setprecision(2)
                          << metrics.h0 << " m" << std::endl;
                std::cout << "     冠幅深度 (CD): " << std::fixed << std::setprecision(2)
                          << metrics.crown_depth << " m" << std::endl;
            } else {
                std::cerr << "     冠幅深度计算失败: " << cd_result.error_message << std::endl;
            }
        } else {
            std::cerr << "     树高计算失败: " << height_result.error_message << std::endl;
        }
        
        // 计算冠幅半径 CR
        if (config.calculate_crown) {
            std::cout << "     计算冠幅半径..." << std::endl;
            auto cr_result = metric::CrownRadius::calculateFromFilteredNodes(
                filtered_path,
                config.verbose
            );
            
            if (cr_result.success) {
                metrics.crown_radius = cr_result.crown_radius;
                metrics.crown_diameter = cr_result.crown_radius * 2;
                metrics.max_crown_width = cr_result.max_width;
                metrics.min_crown_width = cr_result.min_width;
                metrics.crown_aspect_ratio = cr_result.aspect_ratio;
                
                std::cout << "     冠幅半径: " << std::fixed << std::setprecision(2)
                          << metrics.crown_radius << " m" << std::endl;
                std::cout << "     最大冠幅: " << std::fixed << std::setprecision(2)
                          << metrics.max_crown_width << " m" << std::endl;
                std::cout << "     长宽比: " << std::fixed << std::setprecision(2)
                          << metrics.crown_aspect_ratio << std::endl;
            } else {
                std::cerr << "     冠幅计算失败: " << cr_result.error_message << std::endl;
            }
        }
    } else {
        std::cout << "     未找到筛选后的叶节点文件，跳过高度和冠幅计算" << std::endl;
    }

    // 计算DBH - 使用计算得到的h0（活冠基部高度）
    if (metrics.h0 > 0 && !final_output_file.empty()) {
        std::cout << "     计算DBH..." << std::endl;
        auto dbh_result = metric::DBHCalculator::calculateDBH(
            final_output_file.string(),  // 使用填洞后的branches文件
            metrics.h0,                   // 使用计算得到的活冠基部高度
            config.verbose
        );
        
        if (dbh_result.success) {
            metrics.dbh = dbh_result.dbh_cm;
            metrics.dbh_method = dbh_result.method_used;
            std::cout << "     DBH: " << std::fixed << std::setprecision(2) 
                      << metrics.dbh << " cm (" << metrics.dbh_method << ")" << std::endl;
        } else {
            std::cerr << "     DBH计算失败: " << dbh_result.error_message << std::endl;
            metrics.dbh_method = "计算失败";
        }
    } else {
        std::cout << "     跳过DBH计算（缺少必要参数）" << std::endl;
        metrics.dbh_method = "未计算";
    }

    // 计算体积/材积
    if (config.calculate_volume && !final_output_file.empty()) {
        std::cout << "     计算体积..." << std::endl;
        auto volume_result = metric::TreeVolume::calculateFromOBJ(
            final_output_file.string(),
            config.verbose
        );
        
        if (volume_result.success) {
            metrics.volume = volume_result.volume;
            metrics.surface_area = volume_result.surface_area;
            metrics.mesh_is_closed = volume_result.is_closed;
            
            std::cout << "     体积: " << std::fixed << std::setprecision(3)
                      << metrics.volume << " m³" << std::endl;
            std::cout << "     表面积: " << std::fixed << std::setprecision(2)
                      << metrics.surface_area << " m²" << std::endl;
            std::cout << "     网格状态: " << (metrics.mesh_is_closed ? "封闭" : "开放") << std::endl;
        } else {
            std::cerr << "     体积计算失败: " << volume_result.error_message << std::endl;
        }
    }
    
    std::cout << "     指标计算完成" << std::endl;
    
    // 生成单个树木的JSON报告
    if (!config.report_dir.empty()) {
        generate_single_json_report(metrics, config.report_dir);
    }
    
    std::cout << "  完成处理: " << base_name << std::endl;
    
    return metrics;
}

void print_usage(const char* program_name) {
    std::cout << "MeTreec Pipeline - 树木重建与处理\n\n";
    std::cout << "用法:\n";
    std::cout << "  " << program_name << "                    # 默认路径模式\n";
    std::cout << "  " << program_name << " <input> <output>   # 指定输入输出\n\n";
    std::cout << "选项:\n";
    std::cout << "  --adtree-exe <path>    指定AdTree路径\n";
    std::cout << "  --no-fill              不进行填洞处理\n";
    std::cout << "  --max-hole-size <n>    最大填洞尺寸\n";
    std::cout << "  --no-skeleton          不处理骨架数据\n";
    std::cout << "  --no-volume            不计算体积\n";
    std::cout << "  --no-crown             不计算冠幅\n";
    std::cout << "  --filter-ratio <n>     叶节点筛选比例 (默认: 0.15)\n";
    std::cout << "  --verbose              显示详细信息\n";
    std::cout << "  --help, -h             显示帮助\n";
}

int main(int argc, char* argv[]) {
    Config config;
    
    // 帮助信息
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    // 解析参数
    if (argc == 1) {
        // 默认路径模式
        config.use_default_paths = true;
        
        fs::path exe_path = fs::canonical(fs::path(argv[0]));
        fs::path root_dir = exe_path.parent_path().parent_path().parent_path();
        
        // 尝试找到项目根目录
        if (!fs::exists(root_dir / "data")) {
            root_dir = fs::current_path();
            while (root_dir.has_parent_path() && root_dir.filename() != "MeTreec") {
                root_dir = root_dir.parent_path();
            }
        }
        
        config.input_path = (root_dir / "data" / "input").string();
        config.output_dir = (root_dir / "data" / "temp").string();
        config.adtree_output_dir = (root_dir / "data" / "output" / "models").string();
        config.report_dir = (root_dir / "data" / "output" / "report").string();
        
        if (!fs::exists(config.input_path)) {
            std::cerr << "错误: 输入目录不存在: " << config.input_path << std::endl;
            return 1;
        }
        
        // 创建必要的目录
        fs::create_directories(config.output_dir);
        fs::create_directories(config.adtree_output_dir);
        fs::create_directories(config.report_dir);
        
    } else if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    } else {
        // 自定义路径模式
        config.use_default_paths = false;
        config.input_path = argv[1];
        config.output_dir = argv[2];
        config.report_dir = config.output_dir;  // 报告输出到同一目录
        
        // 解析选项
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--adtree-exe" && i + 1 < argc) {
                config.adtree_exe = argv[++i];
            } else if (arg == "--no-fill") {
                config.fill_holes = false;
            } else if (arg == "--max-hole-size" && i + 1 < argc) {
                config.max_hole_size = std::atoi(argv[++i]);
            } else if (arg == "--no-skeleton") {
                config.process_skeleton = false;
            } else if (arg == "--no-volume") {
                config.calculate_volume = false;
            } else if (arg == "--no-crown") {
                config.calculate_crown = false;
            } else if (arg == "--filter-ratio" && i + 1 < argc) {
                config.filter_ratio = std::atof(argv[++i]);
            } else if (arg == "--verbose") {
                config.verbose = true;
            }
        }
    }
    
    // 查找AdTree
    if (config.adtree_exe.empty()) {
        config.adtree_exe = find_adtree();
        if (config.adtree_exe.empty()) {
            std::cerr << "错误: 未找到AdTree可执行文件\n";
            std::cerr << "请构建AdTree或使用 --adtree-exe 指定路径\n";
            return 1;
        }
    }
    
    if (!fs::exists(config.adtree_exe)) {
        std::cerr << "错误: AdTree不存在: " << config.adtree_exe << std::endl;
        return 1;
    }
    
    // 创建输出目录
    fs::create_directories(config.output_dir);
    if (!config.report_dir.empty()) {
        fs::create_directories(config.report_dir);
    }
    
    // 收集xyz文件
    std::vector<std::string> xyz_files;
    
    if (fs::is_regular_file(config.input_path)) {
        if (config.input_path.substr(config.input_path.size() - 4) == ".xyz") {
            xyz_files.push_back(config.input_path);
        }
    } else if (fs::is_directory(config.input_path)) {
        for (const auto& entry : fs::directory_iterator(config.input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xyz") {
                xyz_files.push_back(entry.path().string());
            }
        }
        // 按文件名排序
        std::sort(xyz_files.begin(), xyz_files.end());
    }
    
    if (xyz_files.empty()) {
        std::cerr << "错误: 未找到xyz文件" << std::endl;
        return 1;
    }
    
    // 打印配置信息
    std::cout << "========================================" << std::endl;
    std::cout << "         MeTreec Pipeline" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << "  输入路径: " << config.input_path << std::endl;
    std::cout << "  输出目录: " << config.output_dir << std::endl;
    if (config.use_default_paths) {
        std::cout << "  AdTree输出: " << config.adtree_output_dir << std::endl;
        std::cout << "  报告目录: " << config.report_dir << std::endl;
    }
    std::cout << "  AdTree路径: " << config.adtree_exe << std::endl;
    std::cout << "  文件数量: " << xyz_files.size() << std::endl;
    std::cout << "  填洞处理: " << (config.fill_holes ? "是" : "否") << std::endl;
    std::cout << "  骨架处理: " << (config.process_skeleton ? "是" : "否") << std::endl;
    std::cout << "  体积计算: " << (config.calculate_volume ? "是" : "否") << std::endl;
    std::cout << "  冠幅计算: " << (config.calculate_crown ? "是" : "否") << std::endl;
    if (config.process_skeleton) {
        std::cout << "  筛选比例: " << (config.filter_ratio * 100) << "%" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    // 处理文件
    std::vector<TreeMetrics> all_metrics;
    int success_count = 0;
    int fail_count = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < xyz_files.size(); ++i) {
        std::cout << "\n[" << (i+1) << "/" << xyz_files.size() << "] ";
        
        TreeMetrics metrics = process_file(xyz_files[i], config);
        
        if (!metrics.tree_id.empty()) {
            all_metrics.push_back(metrics);
            success_count++;
        } else {
            fail_count++;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    // 生成汇总报告
    std::cout << "\n========================================" << std::endl;
    std::cout << "           处理完成" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "统计信息:" << std::endl;
    std::cout << "  成功: " << success_count << " 个文件" << std::endl;
    std::cout << "  失败: " << fail_count << " 个文件" << std::endl;
    std::cout << "  总用时: " << duration.count() << " 秒" << std::endl;
    
    if (!all_metrics.empty()) {
        // 生成汇总CSV报告
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        
        fs::path csv_report_path = fs::path(config.report_dir) / 
            ("summary_" + timestamp.str() + ".csv");
        
        if (generate_csv_report(all_metrics, csv_report_path.string())) {
            std::cout << "\n汇总CSV报告已保存: " << csv_report_path << std::endl;
        } else {
            std::cerr << "\n警告: 无法生成CSV报告" << std::endl;
        }
        
        // 打印摘要表格
        std::cout << "\n树木指标摘要:" << std::endl;
        std::cout << std::string(140, '-') << std::endl;
        std::cout << std::left << std::setw(20) << "树木ID"
                  << std::setw(10) << "树高(m)"
                  << std::setw(10) << "h0(m)"
                  << std::setw(12) << "冠深(m)"
                  << std::setw(10) << "DBH(cm)"
                  << std::setw(15) << "冠径(m)"
                  << std::setw(15) << "最大冠幅(m)"
                  << std::setw(12) << "长宽比"
                  << std::setw(12) << "体积(m³)"
                  << std::setw(12) << "表面积(m²)"
                  << std::setw(12) << "叶节点"
                  << std::endl;
        std::cout << std::string(140, '-') << std::endl;
        
        for (const auto& m : all_metrics) {
            std::cout << std::left << std::setw(20) << m.tree_id
                      << std::setw(10) << std::fixed << std::setprecision(2) << m.height
                      << std::setw(10) << std::fixed << std::setprecision(2) << m.h0
                      << std::setw(12) << std::fixed << std::setprecision(2) << m.crown_depth
                      << std::setw(10) << std::fixed << std::setprecision(2) << m.dbh
                      << std::setw(15) << std::fixed << std::setprecision(2) << m.crown_diameter
                      << std::setw(15) << std::fixed << std::setprecision(2) << m.max_crown_width
                      << std::setw(12) << std::fixed << std::setprecision(2) << m.crown_aspect_ratio
                      << std::setw(12) << std::fixed << std::setprecision(3) << m.volume
                      << std::setw(12) << std::fixed << std::setprecision(2) << m.surface_area
                      << std::setw(12) << m.leaf_nodes_filtered
                      << std::endl;
        }
        std::cout << std::string(140, '-') << std::endl;
        
        // 计算平均值
        if (all_metrics.size() > 1) {
            double avg_height = 0, avg_h0 = 0, avg_cd = 0, avg_dbh = 0;
            double avg_crown = 0, avg_volume = 0, avg_surface = 0;
            double avg_max_width = 0, avg_aspect = 0;
            int total_leaves = 0;
            int dbh_count = 0, volume_count = 0, crown_count = 0;
            
            for (const auto& m : all_metrics) {
                avg_height += m.height;
                avg_h0 += m.h0;
                avg_cd += m.crown_depth;
                if (m.dbh > 0) {
                    avg_dbh += m.dbh;
                    dbh_count++;
                }
                if (m.crown_diameter > 0) {
                    avg_crown += m.crown_diameter;
                    avg_max_width += m.max_crown_width;
                    avg_aspect += m.crown_aspect_ratio;
                    crown_count++;
                }
                if (m.volume > 0) {
                    avg_volume += m.volume;
                    avg_surface += m.surface_area;
                    volume_count++;
                }
                total_leaves += m.leaf_nodes_filtered;
            }
            
            size_t n = all_metrics.size();
            avg_height /= n;
            avg_h0 /= n;
            avg_cd /= n;
            if (dbh_count > 0) avg_dbh /= dbh_count;
            if (crown_count > 0) {
                avg_crown /= crown_count;
                avg_max_width /= crown_count;
                avg_aspect /= crown_count;
            }
            if (volume_count > 0) {
                avg_volume /= volume_count;
                avg_surface /= volume_count;
            }
            double avg_leaves = static_cast<double>(total_leaves) / n;
            
            std::cout << std::left << std::setw(20) << "平均值:"
                      << std::setw(10) << std::fixed << std::setprecision(2) << avg_height
                      << std::setw(10) << std::fixed << std::setprecision(2) << avg_h0
                      << std::setw(12) << std::fixed << std::setprecision(2) << avg_cd
                      << std::setw(10) << std::fixed << std::setprecision(2) << avg_dbh
                      << std::setw(15) << std::fixed << std::setprecision(2) << avg_crown
                      << std::setw(15) << std::fixed << std::setprecision(2) << avg_max_width
                      << std::setw(12) << std::fixed << std::setprecision(2) << avg_aspect
                      << std::setw(12) << std::fixed << std::setprecision(3) << avg_volume
                      << std::setw(12) << std::fixed << std::setprecision(2) << avg_surface
                      << std::setw(12) << std::fixed << std::setprecision(0) << avg_leaves
                      << std::endl;
        }
    }
    
    std::cout << "\n输出文件位置:" << std::endl;
    std::cout << "  模型文件: " << (config.use_default_paths ? config.adtree_output_dir : config.output_dir) << std::endl;
    std::cout << "  处理结果: " << config.output_dir << std::endl;
    std::cout << "  分析报告: " << config.report_dir << std::endl;
    std::cout << "    - 每棵树独立JSON文件" << std::endl;
    std::cout << "    - 汇总CSV文件" << std::endl;
    
    std::cout << "\nPipeline执行完成!" << std::endl;
    
    return (success_count > 0) ? 0 : 1;
}