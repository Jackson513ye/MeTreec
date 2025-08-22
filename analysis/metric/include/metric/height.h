#ifndef METRIC_HEIGHT_H
#define METRIC_HEIGHT_H

#include <string>
#include <vector>
#include <array>

namespace metric {

// 点结构（从XYZ文件读取）
struct Point3D {
    double x;
    double y;
    double z;
    double radius;  // 半径（可选）
    
    Point3D() : x(0), y(0), z(0), radius(1.0) {}
    Point3D(double x_, double y_, double z_, double r_ = 1.0) 
        : x(x_), y(y_), z(z_), radius(r_) {}
    
    // 按高度排序的比较运算符
    bool operator<(const Point3D& other) const {
        return z < other.z;  // 按z坐标升序
    }
    
    bool operator>(const Point3D& other) const {
        return z > other.z;  // 按z坐标降序
    }
};

// 高度计算结果
struct HeightResult {
    bool success;
    double tree_height;      // 树高 h_t
    int point_count;         // 用于计算的点数
    std::string error_message;
    
    HeightResult() : success(false), tree_height(0.0), point_count(0) {}
};

// 树木高度计算类
class TreeHeight {
public:
    // 从筛选后的叶节点文件计算树高
    // input_xyz: 筛选后的叶节点文件路径 (*_filtered.xyz)
    // top_n: 使用最高的n个点计算平均值（默认5个）
    static HeightResult calculateFromFilteredNodes(
        const std::string& input_xyz,
        int top_n = 5,
        bool verbose = false
    );
    
    // 从点集合直接计算树高
    static HeightResult calculateFromPoints(
        const std::vector<Point3D>& points,
        int top_n = 5,
        bool verbose = false
    );
    
    // 读取XYZ文件
    static bool readXYZFile(
        const std::string& filename,
        std::vector<Point3D>& points,
        bool verbose = false
    );
    
private:
    // 获取最高的n个点
    static std::vector<Point3D> getTopNPoints(
        const std::vector<Point3D>& points,
        int n
    );
    
    // 计算点集的平均高度
    static double calculateAverageHeight(
        const std::vector<Point3D>& points
    );
};

} // namespace metric

#endif // METRIC_HEIGHT_H