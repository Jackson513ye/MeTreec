#include "metric/height.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace metric {

HeightResult TreeHeight::calculateFromFilteredNodes(
    const std::string& input_xyz,
    int top_n,
    bool verbose) {
    
    HeightResult result;
    
    // 检查输入文件
    if (!fs::exists(input_xyz)) {
        result.error_message = "Input file does not exist: " + input_xyz;
        return result;
    }
    
    // 读取XYZ文件
    std::vector<Point3D> points;
    if (!readXYZFile(input_xyz, points, verbose)) {
        result.error_message = "Failed to read XYZ file: " + input_xyz;
        return result;
    }
    
    if (verbose) {
        std::cout << "Read " << points.size() << " points from file" << std::endl;
    }
    
    // 计算树高
    return calculateFromPoints(points, top_n, verbose);
}

HeightResult TreeHeight::calculateFromPoints(
    const std::vector<Point3D>& points,
    int top_n,
    bool verbose) {
    
    HeightResult result;
    
    if (points.empty()) {
        result.error_message = "No points provided";
        return result;
    }
    
    if (top_n <= 0) {
        result.error_message = "Invalid top_n value: " + std::to_string(top_n);
        return result;
    }
    
    // 获取最高的n个点
    std::vector<Point3D> topPoints = getTopNPoints(points, top_n);
    
    if (topPoints.empty()) {
        result.error_message = "Failed to get top points";
        return result;
    }
    
    // 计算平均高度
    result.tree_height = calculateAverageHeight(topPoints);
    result.point_count = static_cast<int>(topPoints.size());
    result.success = true;
    
    if (verbose) {
        std::cout << "Tree height calculation:" << std::endl;
        std::cout << "  Total points: " << points.size() << std::endl;
        std::cout << "  Using top " << result.point_count << " points" << std::endl;
        std::cout << "  Tree height (h_t): " << result.tree_height << " m" << std::endl;
        
        // 显示用于计算的点的高度
        std::cout << "  Heights of top points: ";
        for (const auto& p : topPoints) {
            std::cout << p.z << " ";
        }
        std::cout << std::endl;
    }
    
    return result;
}

bool TreeHeight::readXYZFile(
    const std::string& filename,
    std::vector<Point3D>& points,
    bool verbose) {
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        if (verbose) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
        }
        return false;
    }
    
    points.clear();
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // 跳过空行
        if (line.empty()) continue;
        
        // 跳过注释行（如果有）
        if (line[0] == '#' || line[0] == '/') continue;
        
        std::istringstream iss(line);
        Point3D point;
        
        // 读取 x y z [radius]
        if (!(iss >> point.x >> point.y >> point.z)) {
            if (verbose) {
                std::cerr << "Warning: Invalid data at line " << line_number << std::endl;
            }
            continue;
        }
        
        // 尝试读取半径（可选）
        if (!(iss >> point.radius)) {
            point.radius = 1.0;  // 默认半径
        }
        
        points.push_back(point);
    }
    
    file.close();
    
    if (verbose) {
        std::cout << "Successfully read " << points.size() << " points from " << filename << std::endl;
    }
    
    return !points.empty();
}

std::vector<Point3D> TreeHeight::getTopNPoints(
    const std::vector<Point3D>& points,
    int n) {
    
    if (points.empty() || n <= 0) {
        return {};
    }
    
    // 复制点集
    std::vector<Point3D> sortedPoints = points;
    
    // 按高度降序排序（最高的在前）
    std::sort(sortedPoints.begin(), sortedPoints.end(), 
        [](const Point3D& a, const Point3D& b) { return a.z > b.z; });
    
    // 取前n个点
    int count = std::min(n, static_cast<int>(sortedPoints.size()));
    std::vector<Point3D> topPoints(sortedPoints.begin(), sortedPoints.begin() + count);
    
    return topPoints;
}

double TreeHeight::calculateAverageHeight(const std::vector<Point3D>& points) {
    if (points.empty()) return 0.0;
    
    double sumHeight = 0.0;
    for (const auto& point : points) {
        sumHeight += point.z;
    }
    
    return sumHeight / points.size();
}

} // namespace metric