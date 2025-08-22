#ifndef METRIC_CD_H
#define METRIC_CD_H

#include <string>
#include <vector>
#include "metric/height.h"  // 使用Point3D结构

namespace metric {

// 冠幅深度计算结果
struct CrownDepthResult {
    bool success;
    double h0;              // 活冠基部高度
    double crown_depth;     // 冠幅深度 (CD = h_t - h0)
    int point_count;        // 用于计算h0的点数
    std::string error_message;
    
    CrownDepthResult() : success(false), h0(0.0), crown_depth(0.0), point_count(0) {}
};

// 冠幅深度计算类
class CrownDepth {
public:
    // 从筛选后的叶节点文件计算冠幅深度
    // input_xyz: 筛选后的叶节点文件路径 (*_filtered.xyz)
    // tree_height: 树高 h_t（由TreeHeight计算得出）
    // bottom_n: 使用最低的n个点计算h0（默认5个）
    static CrownDepthResult calculateFromFilteredNodes(
        const std::string& input_xyz,
        double tree_height,
        int bottom_n = 5,
        bool verbose = false
    );
    
    // 从点集合直接计算冠幅深度
    static CrownDepthResult calculateFromPoints(
        const std::vector<Point3D>& points,
        double tree_height,
        int bottom_n = 5,
        bool verbose = false
    );
    
    // 仅计算h0（活冠基部高度）
    static double calculateH0(
        const std::vector<Point3D>& points,
        int bottom_n = 5,
        bool verbose = false
    );
    
private:
    // 获取最低的n个点
    static std::vector<Point3D> getBottomNPoints(
        const std::vector<Point3D>& points,
        int n
    );
    
    // 计算点集的平均高度
    static double calculateAverageHeight(
        const std::vector<Point3D>& points
    );
};

} // namespace metric

#endif // METRIC_CD_H