#ifndef PREPROCESSING_STRUCTURE_EXTRACT_H
#define PREPROCESSING_STRUCTURE_EXTRACT_H

#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_3.h>
#include <CGAL/Segment_3.h>

namespace preprocessing {

// CGAL类型定义
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point_3;
typedef Kernel::Segment_3 Segment_3;

// 树木骨架结构
struct TreeSkeleton {
    std::vector<Point_3> vertices;
    std::vector<float> radii;
    std::vector<std::array<int, 2>> edges;
    std::vector<Segment_3> segments;
    
    void clear() {
        vertices.clear();
        radii.clear();
        edges.clear();
        segments.clear();
    }
    
    size_t vertexCount() const { return vertices.size(); }
    size_t edgeCount() const { return edges.size(); }
};

// 叶节点结构
struct LeafNode {
    Point_3 position;
    float radius;
    int original_index;
    double height;  // z坐标
    
    bool operator<(const LeafNode& other) const {
        return height < other.height;
    }
};

// 叶节点集合
struct LeafNodes {
    std::vector<LeafNode> nodes;
    
    void clear() { nodes.clear(); }
    size_t size() const { return nodes.size(); }
    bool empty() const { return nodes.empty(); }
};

// 处理结果
struct SkeletonFilterResult {
    bool success;
    std::string error_message;
    std::string output_file;  // 输出的筛选后叶节点文件路径
    int total_leaves;          // 总叶节点数
    int filtered_leaves;       // 筛选后叶节点数
    
    SkeletonFilterResult() : success(false), total_leaves(0), filtered_leaves(0) {}
};

// 结构提取器类 - 简化版，只负责筛选叶节点
class StructureExtractor {
public:
    // 处理单个PLY文件，输出筛选后的叶节点
    static SkeletonFilterResult filterLeafNodes(
        const std::string& input_ply_path,
        const std::string& output_dir = "",
        double filter_percentage = 0.15,
        bool verbose = false
    );
    
    // 批量处理目录中的所有PLY文件
    static std::vector<SkeletonFilterResult> processDirectory(
        const std::string& input_dir,
        const std::string& output_dir = "",
        double filter_percentage = 0.15,
        bool verbose = false
    );
    
private:
    // 从PLY文件读取骨架
    static bool readSkeletonFromPLY(
        const std::string& filename,
        TreeSkeleton& skeleton,
        bool verbose = false
    );
    
    // 提取叶节点（度为1的节点）
    static LeafNodes extractLeafNodes(const TreeSkeleton& skeleton);
    
    // 筛选叶节点
    static LeafNodes filterLeafNodes(
        const LeafNodes& allLeafNodes,
        double skeletonHeight,
        double percentage = 0.15
    );
    
    // 计算骨架高度
    static double calculateSkeletonHeight(const TreeSkeleton& skeleton);
    
    // 写入叶节点到XYZ文件
    static bool writeNodesToXYZ(
        const LeafNodes& nodes,
        const std::string& filename,
        bool verbose = false
    );
    
    // 计算两点间的欧氏距离
    static double calculateDistance(const Point_3& p1, const Point_3& p2);
    
    // 查找最近的n个叶节点的索引
    static std::vector<int> findNearestNeighbors(
        const LeafNodes& leafNodes,
        int targetIndex,
        int n
    );
};

} // namespace preprocessing

#endif // PREPROCESSING_STRUCTURE_EXTRACT_H