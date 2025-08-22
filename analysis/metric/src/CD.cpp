#include "metric/CD.h"
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace metric {

CrownDepthResult CrownDepth::calculateFromFilteredNodes(
    const std::string& input_xyz,
    double tree_height,
    int bottom_n,
    bool verbose) {
    
    CrownDepthResult result;
    
    // 检查输入文件
    if (!fs::exists(input_xyz)) {
        result.error_message = "Input file does not exist: " + input_xyz;
        return result;
    }
    
    // 检查树高有效性
    if (tree_height <= 0) {
        result.error_message = "Invalid tree height: " + std::to_string(tree_height);
        return result;
    }
    
    // 读取XYZ文件
    std::vector<Point3D> points;
    if (!TreeHeight::readXYZFile(input_xyz, points, verbose)) {
        result.error_message = "Failed to read XYZ file: " + input_xyz;
        return result;
    }
    
    if (verbose) {
        std::cout << "Read " << points.size() << " points from file" << std::endl;
    }
    
    // 计算冠幅深度
    return calculateFromPoints(points, tree_height, bottom_n, verbose);
}

CrownDepthResult CrownDepth::calculateFromPoints(
    const std::vector<Point3D>& points,
    double tree_height,
    int bottom_n,
    bool verbose) {
    
    CrownDepthResult result;
    
    if (points.empty()) {
        result.error_message = "No points provided";
        return result;
    }
    
    if (tree_height <= 0) {
        result.error_message = "Invalid tree height: " + std::to_string(tree_height);
        return result;
    }
    
    if (bottom_n <= 0) {
        result.error_message = "Invalid bottom_n value: " + std::to_string(bottom_n);
        return result;
    }
    
    // 计算h0
    result.h0 = calculateH0(points, bottom_n, verbose);
    
    // 计算冠幅深度
    result.crown_depth = tree_height - result.h0;
    
    // 获取实际使用的点数
    std::vector<Point3D> bottomPoints = getBottomNPoints(points, bottom_n);
    result.point_count = static_cast<int>(bottomPoints.size());
    
    result.success = true;
    
    if (verbose) {
        std::cout << "Crown depth calculation:" << std::endl;
        std::cout << "  Total points: " << points.size() << std::endl;
        std::cout << "  Using bottom " << result.point_count << " points" << std::endl;
        std::cout << "  h0 (crown base height): " << result.h0 << " m" << std::endl;
        std::cout << "  Tree height (h_t): " << tree_height << " m" << std::endl;
        std::cout << "  Crown depth (CD): " << result.crown_depth << " m" << std::endl;
        
        // 显示用于计算的点的高度
        std::cout << "  Heights of bottom points: ";
        for (const auto& p : bottomPoints) {
            std::cout << p.z << " ";
        }
        std::cout << std::endl;
    }
    
    return result;
}

double CrownDepth::calculateH0(
    const std::vector<Point3D>& points,
    int bottom_n,
    bool verbose) {
    
    if (points.empty() || bottom_n <= 0) {
        return 0.0;
    }
    
    // 获取最低的n个点
    std::vector<Point3D> bottomPoints = getBottomNPoints(points, bottom_n);
    
    if (bottomPoints.empty()) {
        return 0.0;
    }
    
    // 计算平均高度
    double h0 = calculateAverageHeight(bottomPoints);
    
    if (verbose) {
        std::cout << "h0 calculation: using " << bottomPoints.size() 
                  << " lowest points, h0 = " << h0 << " m" << std::endl;
    }
    
    return h0;
}

std::vector<Point3D> CrownDepth::getBottomNPoints(
    const std::vector<Point3D>& points,
    int n) {
    
    if (points.empty() || n <= 0) {
        return {};
    }
    
    // 复制点集
    std::vector<Point3D> sortedPoints = points;
    
    // 按高度升序排序（最低的在前）
    std::sort(sortedPoints.begin(), sortedPoints.end(), 
        [](const Point3D& a, const Point3D& b) { return a.z < b.z; });
    
    // 取前n个点
    int count = std::min(n, static_cast<int>(sortedPoints.size()));
    std::vector<Point3D> bottomPoints(sortedPoints.begin(), sortedPoints.begin() + count);
    
    return bottomPoints;
}

double CrownDepth::calculateAverageHeight(const std::vector<Point3D>& points) {
    if (points.empty()) return 0.0;
    
    double sumHeight = 0.0;
    for (const auto& point : points) {
        sumHeight += point.z;
    }
    
    return sumHeight / points.size();
}

} // namespace metric