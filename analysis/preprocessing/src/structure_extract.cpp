#include "preprocessing/structure_extract.h"
#include "preprocessing/happly.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>

namespace fs = std::filesystem;

namespace preprocessing {

SkeletonFilterResult StructureExtractor::filterLeafNodes(
    const std::string& input_ply_path,
    const std::string& output_dir,
    double filter_percentage,
    bool verbose) {
    
    SkeletonFilterResult result;
    fs::path input_path(input_ply_path);
    
    // 检查输入文件
    if (!fs::exists(input_path)) {
        result.error_message = "Input file does not exist: " + input_ply_path;
        return result;
    }
    
    if (input_path.extension() != ".ply") {
        result.error_message = "Input file is not a PLY file: " + input_ply_path;
        return result;
    }
    
    // 确定输出目录
    fs::path out_dir = output_dir.empty() ? 
        input_path.parent_path() : fs::path(output_dir);
    
    // 创建输出目录
    if (!fs::exists(out_dir)) {
        fs::create_directories(out_dir);
    }
    
    // 生成输出文件名 - 只输出筛选后的叶节点
    std::string base_name = input_path.stem().string();
    result.output_file = (out_dir / (base_name + "_filtered.xyz")).string();
    
    if (verbose) {
        std::cout << "Processing skeleton: " << base_name << std::endl;
    }
    
    try {
        // 1. 读取骨架数据
        TreeSkeleton skeleton;
        if (!readSkeletonFromPLY(input_ply_path, skeleton, verbose)) {
            result.error_message = "Failed to read PLY file";
            return result;
        }
        
        // 2. 计算骨架总高度（用于筛选算法）
        double skeleton_height = calculateSkeletonHeight(skeleton);
        
        // 3. 提取所有叶节点
        LeafNodes allLeafNodes = extractLeafNodes(skeleton);
        result.total_leaves = allLeafNodes.size();
        
        if (verbose) {
            std::cout << "  Total leaf nodes: " << result.total_leaves << std::endl;
        }
        
        if (allLeafNodes.empty()) {
            result.error_message = "No leaf nodes found";
            return result;
        }
        
        // 4. 筛选叶节点
        LeafNodes filteredLeafNodes = filterLeafNodes(
            allLeafNodes, 
            skeleton_height, 
            filter_percentage
        );
        result.filtered_leaves = filteredLeafNodes.size();
        
        if (verbose) {
            std::cout << "  Filtered leaf nodes: " << result.filtered_leaves << std::endl;
        }
        
        if (filteredLeafNodes.empty()) {
            result.error_message = "No leaf nodes after filtering";
            return result;
        }
        
        // 5. 保存筛选后的叶节点
        if (!writeNodesToXYZ(filteredLeafNodes, result.output_file, verbose)) {
            result.error_message = "Failed to write filtered leaf nodes";
            return result;
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
    }
    
    return result;
}

std::vector<SkeletonFilterResult> StructureExtractor::processDirectory(
    const std::string& input_dir,
    const std::string& output_dir,
    double filter_percentage,
    bool verbose) {
    
    std::vector<SkeletonFilterResult> results;
    
    // 检查输入目录
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        if (verbose) {
            std::cerr << "Error: Invalid input directory: " << input_dir << std::endl;
        }
        return results;
    }
    
    // 收集所有PLY文件
    std::vector<fs::path> ply_files;
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ply") {
            ply_files.push_back(entry.path());
        }
    }
    
    if (ply_files.empty()) {
        if (verbose) {
            std::cout << "No PLY files found in: " << input_dir << std::endl;
        }
        return results;
    }
    
    // 按文件名排序
    std::sort(ply_files.begin(), ply_files.end());
    
    if (verbose) {
        std::cout << "Found " << ply_files.size() << " PLY files" << std::endl;
    }
    
    // 处理每个文件
    for (const auto& ply_file : ply_files) {
        auto result = filterLeafNodes(
            ply_file.string(),
            output_dir,
            filter_percentage,
            verbose
        );
        results.push_back(result);
    }
    
    return results;
}

bool StructureExtractor::readSkeletonFromPLY(
    const std::string& filename,
    TreeSkeleton& skeleton,
    bool verbose) {
    
    try {
        if (verbose) {
            std::cout << "  Reading PLY file: " << filename << std::endl;
        }
        
        // 使用happly读取PLY文件
        happly::PLYData plyIn(filename);
        
        // 读取顶点数据
        std::vector<float> x = plyIn.getElement("vertex").getProperty<float>("x");
        std::vector<float> y = plyIn.getElement("vertex").getProperty<float>("y");
        std::vector<float> z = plyIn.getElement("vertex").getProperty<float>("z");
        
        // 尝试读取半径数据
        try {
            skeleton.radii = plyIn.getElement("vertex").getProperty<float>("radius");
        } catch (const std::exception&) {
            // 如果没有半径数据，使用默认值
            skeleton.radii.resize(x.size(), 1.0f);
        }
        
        // 构建顶点数组
        skeleton.vertices.clear();
        skeleton.vertices.reserve(x.size());
        for (size_t i = 0; i < x.size(); ++i) {
            skeleton.vertices.emplace_back(x[i], y[i], z[i]);
        }
        
        // 读取边数据
        try {
            std::vector<std::vector<int>> edgeIndices =
                plyIn.getElement("edge").getListProperty<int>("vertex_indices");
            
            skeleton.edges.clear();
            skeleton.edges.reserve(edgeIndices.size());
            
            for (const auto& edge : edgeIndices) {
                if (edge.size() == 2) {
                    skeleton.edges.push_back({edge[0], edge[1]});
                }
            }
            
        } catch (const std::exception&) {
            // 边数据是可选的
            if (verbose) {
                std::cout << "  Note: No edge data found in PLY file" << std::endl;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (verbose) {
            std::cerr << "  Error reading PLY file: " << e.what() << std::endl;
        }
        return false;
    }
}

LeafNodes StructureExtractor::extractLeafNodes(const TreeSkeleton& skeleton) {
    LeafNodes leafNodes;
    
    // 计算每个顶点的度
    std::vector<int> degree(skeleton.vertices.size(), 0);
    for (const auto& edge : skeleton.edges) {
        degree[edge[0]]++;
        degree[edge[1]]++;
    }
    
    // 收集所有度为1的节点（叶节点）
    for (size_t i = 0; i < degree.size(); ++i) {
        if (degree[i] == 1) {
            LeafNode node;
            node.original_index = static_cast<int>(i);
            node.position = skeleton.vertices[i];
            node.height = skeleton.vertices[i].z();
            node.radius = (i < skeleton.radii.size()) ? skeleton.radii[i] : 1.0f;
            leafNodes.nodes.push_back(node);
        }
    }
    
    return leafNodes;
}

LeafNodes StructureExtractor::filterLeafNodes(
    const LeafNodes& allLeafNodes,
    double skeletonHeight,
    double percentage) {
    
    LeafNodes filteredNodes;
    
    // 步骤1：计算h_l（总高度的percentage）
    double h_l = skeletonHeight * percentage;
    
    // 步骤2：计算n（叶节点数的percentage）
    int totalLeafCount = static_cast<int>(allLeafNodes.size());
    int n = std::max(1, static_cast<int>(totalLeafCount * percentage));
    
    // 计算用于高度平均的点数（percentage的n）
    int topN = std::max(1, static_cast<int>(n * percentage));
    
    // 步骤3-4：对每个叶节点进行筛选
    for (size_t i = 0; i < allLeafNodes.nodes.size(); ++i) {
        const LeafNode& currentNode = allLeafNodes.nodes[i];
        double h_a = currentNode.height;
        
        // 找到最近的n个邻居
        std::vector<int> neighbors = findNearestNeighbors(allLeafNodes, static_cast<int>(i), n);
        
        // 找出邻居中最高的topN个点
        std::vector<double> neighborHeights;
        for (int idx : neighbors) {
            neighborHeights.push_back(allLeafNodes.nodes[idx].height);
        }
        std::sort(neighborHeights.rbegin(), neighborHeights.rend());  // 降序排序
        
        // 计算最高topN个点的平均高度
        double h_ne = 0.0;
        int actualTopN = std::min(topN, static_cast<int>(neighborHeights.size()));
        for (int j = 0; j < actualTopN; ++j) {
            h_ne += neighborHeights[j];
        }
        if (actualTopN > 0) {
            h_ne /= actualTopN;
        }
        
        // 计算高度差
        double h_d = std::abs(h_ne - h_a);
        
        // 判断是否保留
        if (h_d <= h_l) {
            filteredNodes.nodes.push_back(currentNode);
        }
    }
    
    return filteredNodes;
}

double StructureExtractor::calculateSkeletonHeight(const TreeSkeleton& skeleton) {
    if (skeleton.vertices.empty()) return 0.0;
    
    double minZ = skeleton.vertices[0].z();
    double maxZ = skeleton.vertices[0].z();
    
    for (const auto& vertex : skeleton.vertices) {
        minZ = std::min(minZ, vertex.z());
        maxZ = std::max(maxZ, vertex.z());
    }
    
    return maxZ - minZ;
}

bool StructureExtractor::writeNodesToXYZ(
    const LeafNodes& nodes,
    const std::string& filename,
    bool verbose) {
    
    std::ofstream output(filename);
    
    if (!output) {
        if (verbose) {
            std::cerr << "  Error: Cannot create output file " << filename << std::endl;
        }
        return false;
    }
    
    // 写入节点坐标和半径
    for (const auto& node : nodes.nodes) {
        output << std::fixed << std::setprecision(6)
               << node.position.x() << " "
               << node.position.y() << " "
               << node.position.z() << " "
               << node.radius << std::endl;
    }
    
    output.close();
    
    if (verbose) {
        std::cout << "  Written " << nodes.size() << " filtered nodes to: " << filename << std::endl;
    }
    
    return true;
}

double StructureExtractor::calculateDistance(const Point_3& p1, const Point_3& p2) {
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    double dz = p1.z() - p2.z();
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

std::vector<int> StructureExtractor::findNearestNeighbors(
    const LeafNodes& leafNodes,
    int targetIndex,
    int n) {
    
    std::vector<std::pair<double, int>> distances;
    
    for (size_t i = 0; i < leafNodes.nodes.size(); ++i) {
        if (static_cast<int>(i) != targetIndex) {
            double dist = calculateDistance(
                leafNodes.nodes[targetIndex].position,
                leafNodes.nodes[i].position
            );
            distances.push_back({dist, static_cast<int>(i)});
        }
    }
    
    // 按距离排序
    std::sort(distances.begin(), distances.end());
    
    // 取前n个
    std::vector<int> neighbors;
    int count = std::min(n, static_cast<int>(distances.size()));
    for (int i = 0; i < count; ++i) {
        neighbors.push_back(distances[i].second);
    }
    
    return neighbors;
}

} // namespace preprocessing