#include "metric/CR.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_3.h>
#include <CGAL/Point_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Min_circle_2.h>
#include <CGAL/Min_circle_2_traits_2.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cmath>

// 如果有happly.h，包含它；否则使用简单的PLY读取
#ifdef HAS_HAPPLY
#include "happly.h"
#endif

namespace metric {

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point_3;
typedef Kernel::Point_2 Point_2;
typedef CGAL::Min_circle_2_traits_2<Kernel> Min_circle_traits;
typedef CGAL::Min_circle_2<Min_circle_traits> Min_circle;

// 内部数据结构
struct TreePoint {
    Point_3 position;
    double height;  // z坐标
};

// 读取XYZ文件
static std::vector<TreePoint> readXYZFile(const std::string& filename) {
    std::vector<TreePoint> points;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        return points;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        double x, y, z;
        if (iss >> x >> y >> z) {
            TreePoint pt;
            pt.position = Point_3(x, y, z);
            pt.height = z;
            points.push_back(pt);
        }
    }
    
    file.close();
    return points;
}

#ifdef HAS_HAPPLY
// 读取PLY文件（骨架）
static std::vector<TreePoint> readPLYSkeleton(const std::string& filename) {
    std::vector<TreePoint> leafNodes;
    
    try {
        happly::PLYData plyIn(filename);
        
        std::vector<float> x = plyIn.getElement("vertex").getProperty<float>("x");
        std::vector<float> y = plyIn.getElement("vertex").getProperty<float>("y");
        std::vector<float> z = plyIn.getElement("vertex").getProperty<float>("z");
        
        // 读取边信息以确定叶节点
        std::vector<int> degree(x.size(), 0);
        
        try {
            std::vector<std::vector<int>> edgeIndices =
                plyIn.getElement("edge").getListProperty<int>("vertex_indices");
            
            for (const auto& edge : edgeIndices) {
                if (edge.size() == 2) {
                    degree[edge[0]]++;
                    degree[edge[1]]++;
                }
            }
            
            // 度数为1的顶点是叶节点
            for (size_t i = 0; i < degree.size(); ++i) {
                if (degree[i] == 1) {
                    TreePoint node;
                    node.position = Point_3(x[i], y[i], z[i]);
                    node.height = z[i];
                    leafNodes.push_back(node);
                }
            }
        } catch (...) {
            // 如果没有边信息，使用所有点
            for (size_t i = 0; i < x.size(); ++i) {
                TreePoint node;
                node.position = Point_3(x[i], y[i], z[i]);
                node.height = z[i];
                leafNodes.push_back(node);
            }
        }
        
    } catch (const std::exception& e) {
        // 错误处理
    }
    
    return leafNodes;
}
#endif

// 计算冠幅半径的核心函数
static std::tuple<double, double, double, double> calculateCrownRadiusDetailed(
    const std::vector<TreePoint>& points) {
    
    if (points.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    
    // 将所有点投影到XY平面
    std::vector<Point_2> projected_points;
    for (const auto& pt : points) {
        projected_points.push_back(Point_2(pt.position.x(), pt.position.y()));
    }
    
    // 计算凸包
    std::vector<Point_2> convex_hull;
    CGAL::convex_hull_2(projected_points.begin(), projected_points.end(),
                         std::back_inserter(convex_hull));
    
    if (convex_hull.size() < 3) {
        // 使用简单边界框
        double min_x = projected_points[0].x(), max_x = min_x;
        double min_y = projected_points[0].y(), max_y = min_y;
        
        for (const auto& p : projected_points) {
            min_x = std::min(min_x, p.x());
            max_x = std::max(max_x, p.x());
            min_y = std::min(min_y, p.y());
            max_y = std::max(max_y, p.y());
        }
        
        double width_x = max_x - min_x;
        double width_y = max_y - min_y;
        double avg_diameter = (width_x + width_y) / 2.0;
        double crown_radius = avg_diameter / 2.0;
        
        double max_width = std::max(width_x, width_y);
        double min_width = std::min(width_x, width_y);
        double aspect_ratio = (min_width > 0) ? max_width / min_width : 1.0;
        
        return {crown_radius, max_width, min_width, aspect_ratio};
    }
    
    // 使用最小外接圆计算冠幅半径
    Min_circle min_circle(projected_points.begin(), projected_points.end(), true);
    Min_circle_traits::Circle circle = min_circle.circle();
    double radius = std::sqrt(circle.squared_radius());
    
    // 使用旋转卡尺算法计算最小外接矩形（获取更多信息）
    double min_area = std::numeric_limits<double>::max();
    double best_width = 0, best_height = 0;
    
    // 遍历凸包的每条边
    for (size_t i = 0; i < convex_hull.size(); ++i) {
        size_t j = (i + 1) % convex_hull.size();
        
        // 计算边的方向向量
        double dx = convex_hull[j].x() - convex_hull[i].x();
        double dy = convex_hull[j].y() - convex_hull[i].y();
        double edge_length = std::sqrt(dx * dx + dy * dy);
        
        if (edge_length < 1e-10) continue;
        
        // 归一化方向向量
        dx /= edge_length;
        dy /= edge_length;
        
        // 垂直方向向量
        double px = -dy;
        double py = dx;
        
        // 投影所有凸包顶点到这两个方向
        double min_proj_x = std::numeric_limits<double>::max();
        double max_proj_x = std::numeric_limits<double>::lowest();
        double min_proj_y = std::numeric_limits<double>::max();
        double max_proj_y = std::numeric_limits<double>::lowest();
        
        for (const auto& p : convex_hull) {
            double proj_x = p.x() * dx + p.y() * dy;
            double proj_y = p.x() * px + p.y() * py;
            
            min_proj_x = std::min(min_proj_x, proj_x);
            max_proj_x = std::max(max_proj_x, proj_x);
            min_proj_y = std::min(min_proj_y, proj_y);
            max_proj_y = std::max(max_proj_y, proj_y);
        }
        
        double width = max_proj_x - min_proj_x;
        double height = max_proj_y - min_proj_y;
        double area = width * height;
        
        if (area < min_area) {
            min_area = area;
            best_width = width;
            best_height = height;
        }
    }
    
    // 确定长边和短边
    double max_width = std::max(best_width, best_height);
    double min_width = std::min(best_width, best_height);
    double aspect_ratio = (min_width > 0) ? max_width / min_width : 1.0;
    
    return {radius, max_width, min_width, aspect_ratio};
}

// 从筛选后的XYZ文件计算冠幅半径
CrownRadiusResult CrownRadius::calculateFromFilteredNodes(
    const std::string& xyz_file,
    bool verbose) {
    
    CrownRadiusResult result;
    
    if (verbose) {
        std::cout << "读取XYZ文件: " << xyz_file << std::endl;
    }
    
    // 读取XYZ文件
    std::vector<TreePoint> points = readXYZFile(xyz_file);
    
    if (points.empty()) {
        result.error_message = "无法读取XYZ文件或文件为空";
        return result;
    }
    
    result.total_points = points.size();
    result.leaf_nodes = points.size();  // XYZ文件中都是叶节点
    
    if (verbose) {
        std::cout << "  读取到 " << points.size() << " 个点" << std::endl;
    }
    
    // 计算冠幅半径
    auto [crown_radius, max_width, min_width, aspect_ratio] = 
        calculateCrownRadiusDetailed(points);
    
    result.crown_radius = crown_radius;
    result.max_width = max_width;
    result.min_width = min_width;
    result.aspect_ratio = aspect_ratio;
    result.success = true;
    
    if (verbose) {
        std::cout << "  冠幅半径: " << result.crown_radius << " 米" << std::endl;
        std::cout << "  最大冠幅: " << result.max_width << " 米" << std::endl;
        std::cout << "  最小冠幅: " << result.min_width << " 米" << std::endl;
        std::cout << "  长宽比: " << result.aspect_ratio << std::endl;
    }
    
    return result;
}

// 从PLY骨架文件计算冠幅半径
CrownRadiusResult CrownRadius::calculateFromSkeleton(
    const std::string& ply_file,
    bool verbose) {
    
    CrownRadiusResult result;
    
#ifdef HAS_HAPPLY
    if (verbose) {
        std::cout << "读取PLY骨架文件: " << ply_file << std::endl;
    }
    
    // 读取PLY文件
    std::vector<TreePoint> leafNodes = readPLYSkeleton(ply_file);
    
    if (leafNodes.empty()) {
        result.error_message = "无法读取PLY文件或没有找到叶节点";
        return result;
    }
    
    result.total_points = leafNodes.size();
    result.leaf_nodes = leafNodes.size();
    
    if (verbose) {
        std::cout << "  找到 " << leafNodes.size() << " 个叶节点" << std::endl;
    }
    
    // 计算冠幅半径
    auto [crown_radius, max_width, min_width, aspect_ratio] = 
        calculateCrownRadiusDetailed(leafNodes);
    
    result.crown_radius = crown_radius;
    result.max_width = max_width;
    result.min_width = min_width;
    result.aspect_ratio = aspect_ratio;
    result.success = true;
    
    if (verbose) {
        std::cout << "  冠幅半径: " << result.crown_radius << " 米" << std::endl;
        std::cout << "  最大冠幅: " << result.max_width << " 米" << std::endl;
        std::cout << "  最小冠幅: " << result.min_width << " 米" << std::endl;
        std::cout << "  长宽比: " << result.aspect_ratio << std::endl;
    }
#else
    result.error_message = "PLY支持未编译（需要happly.h）";
#endif
    
    return result;
}

}  // namespace metric