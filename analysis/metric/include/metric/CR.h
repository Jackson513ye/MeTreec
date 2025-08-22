#ifndef METRIC_CR_H
#define METRIC_CR_H

#include <string>
#include <vector>

namespace metric {

// 冠幅半径计算结果
struct CrownRadiusResult {
    bool success = false;
    double crown_radius = 0.0;      // 冠幅半径 (米)
    double max_width = 0.0;          // 最大冠幅 (米)
    double min_width = 0.0;          // 最小冠幅 (米)
    double aspect_ratio = 0.0;       // 长宽比
    int total_points = 0;            // 总点数
    int leaf_nodes = 0;              // 叶节点数
    std::string error_message;
};

class CrownRadius {
public:
    // 从筛选后的XYZ文件计算冠幅半径
    static CrownRadiusResult calculateFromFilteredNodes(
        const std::string& xyz_file,
        bool verbose = false
    );
    
    // 从PLY骨架文件计算冠幅半径（保留原始功能）
    static CrownRadiusResult calculateFromSkeleton(
        const std::string& ply_file,
        bool verbose = false
    );
};

} // namespace metric

#endif // METRIC_CR_H