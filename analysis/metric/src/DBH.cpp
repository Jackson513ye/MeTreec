// analysis/metric/src/DBH.cpp
#include "metric/DBH.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

namespace metric {

// TreeModel 实现
TreeModel::TreeModel(const std::string& file) : filename(file) {}

bool TreeModel::loadFromOBJ(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filepath << std::endl;
        return false;
    }
    
    vertices.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Point3D(x, y, z, 1.0));  // 添加默认radius参数
        }
    }
    
    file.close();
    std::cout << "成功加载 " << vertices.size() << " 个顶点" << std::endl;
    return !vertices.empty();
}

std::vector<Point3D> TreeModel::getPointsAtHeight(double height, double tolerance) const {
    std::vector<Point3D> points;
    for (const auto& v : vertices) {
        if (std::abs(v.z - height) <= tolerance) {
            points.push_back(v);
        }
    }
    return points;
}

double TreeModel::getMinHeight() const {
    if (vertices.empty()) return 0;
    return std::min_element(vertices.begin(), vertices.end())->z;  // 使用Point3D的operator<
}

// DBHCalculator 实现
DBHCalculator::DBHCalculator() : crownBaseHeight(0.5) {}

double DBHCalculator::calculateDiameterFromPoints(const std::vector<Point3D>& points) const {
    if (points.empty()) return 0;
    
    // 计算点云的最大距离（简化版本）
    double maxDist = 0;
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            double dist = std::sqrt(
                std::pow(points[i].x - points[j].x, 2) +
                std::pow(points[i].y - points[j].y, 2)
            );
            maxDist = std::max(maxDist, dist);
        }
    }
    return maxDist;
}

int DBHCalculator::detectStemCount(const std::vector<Point3D>& points, double threshold) const {
    if (points.empty()) return 0;
    
    // 使用简化的聚类算法检测分干数量
    std::vector<bool> visited(points.size(), false);
    int clusterCount = 0;
    
    for (size_t i = 0; i < points.size(); ++i) {
        if (!visited[i]) {
            clusterCount++;
            // 标记邻近点
            for (size_t j = i; j < points.size(); ++j) {
                double dist = std::sqrt(
                    std::pow(points[i].x - points[j].x, 2) +
                    std::pow(points[i].y - points[j].y, 2)
                );
                if (dist < threshold) {
                    visited[j] = true;
                }
            }
        }
    }
    
    return clusterCount;
}

double DBHCalculator::calculateMethod1() {
    std::cout << "\n使用方法1：合成胸径法" << std::endl;
    
    auto points = model->getPointsAtHeight(1.3);
    if (points.empty()) {
        std::cout << "警告：1.3米处无数据点" << std::endl;
        return 0;
    }
    
    int stemCount = detectStemCount(points);
    std::cout << "检测到 " << stemCount << " 个分干" << std::endl;
    
    if (stemCount == 1) {
        double dbh = calculateDiameterFromPoints(points);
        return dbh * 100; // 转换为厘米
    } else {
        // 简化实现：假设各分干直径相等
        double totalDiameter = calculateDiameterFromPoints(points);
        double individualDiam = totalDiameter / stemCount;
        double syntheticDBH = individualDiam * std::sqrt(stemCount);
        return syntheticDBH * 100; // 转换为厘米
    }
}

double DBHCalculator::calculateMethod2() {
    std::cout << "\n使用方法2：锥度模型法（m2模型）" << std::endl;
    
    double measureHeight;
    if (crownBaseHeight >= 1.0) {
        measureHeight = 1.0;
    } else {
        measureHeight = 0.7;
    }
    
    std::cout << "测量高度：" << measureHeight << "米" << std::endl;
    
    auto points = model->getPointsAtHeight(measureHeight);
    if (points.empty()) {
        std::cout << "警告：" << measureHeight << "米处无数据点" << std::endl;
        return 0;
    }
    
    int stemCount = detectStemCount(points);
    std::cout << "检测到 " << stemCount << " 个分干" << std::endl;
    
    if (stemCount != 1) {
        if (measureHeight == 0.7) {
            std::cout << "错误：0.7米处仍有分叉，无法计算" << std::endl;
            return -1;
        } else {
            std::cout << "1.0米处有分叉，尝试0.7米..." << std::endl;
            measureHeight = 0.7;
            points = model->getPointsAtHeight(measureHeight);
            stemCount = detectStemCount(points);
            
            if (stemCount != 1) {
                std::cout << "错误：0.7米处仍有分叉，无法计算" << std::endl;
                return -1;
            }
        }
    }
    
    double D_POM = calculateDiameterFromPoints(points) * 100; // 转换为厘米
    std::cout << "D_POM = " << D_POM << " cm" << std::endl;
    
    // 应用m2模型：DBH' = D_POM * (h_target/H_POM)^a
    // 其中 a = -0.156 + 0.048 * D_POM
    double a = -0.156 + 0.048 * D_POM;
    double h_target = 1.3;
    double H_POM = measureHeight;
    
    double DBH_prime = D_POM * std::pow(h_target / H_POM, a);
    
    std::cout << "锥度参数 a = " << a << std::endl;
    std::cout << "计算公式：DBH' = " << D_POM << " * (" << h_target 
              << "/" << H_POM << ")^" << a << std::endl;
    
    return DBH_prime;
}

DBHResult DBHCalculator::calculate(const std::string& objFilePath, double crownBaseHeight, bool verbose) {
    DBHResult result;
    result.success = false;
    result.dbh_cm = 0.0;
    
    this->crownBaseHeight = crownBaseHeight;
    
    // 查找并加载OBJ文件
    std::string filepath = findOBJFile(objFilePath);
    if (filepath.empty()) {
        result.error_message = "未找到OBJ文件: " + objFilePath;
        return result;
    }
    
    model = std::make_unique<TreeModel>(objFilePath);
    if (!model->loadFromOBJ(filepath)) {
        result.error_message = "加载OBJ文件失败";
        return result;
    }
    
    if (verbose) {
        std::cout << "\n========== DBH计算开始 ==========" << std::endl;
        std::cout << "活冠基部高度：" << crownBaseHeight << "米" << std::endl;
    }
    
    // 新增规则：活冠基部高度 < 0.7 m 时，直接不计算
    if (crownBaseHeight < 0.7) {
        result.error_message = "活冠基部高度小于 0.7 米，不满足计算条件";
        if (verbose) {
            std::cout << result.error_message << std::endl;
        }
        return result;
    }
    
    double dbh;
    if (crownBaseHeight > 1.3) {
        dbh = calculateMethod1();
    } else {
        dbh = calculateMethod2();
    }
    
    if (dbh > 0) {
        result.success = true;
        result.dbh_cm = dbh;
        result.method_used = (crownBaseHeight > 1.3) ? "合成胸径法" : "锥度模型法";
        if (verbose) {
            std::cout << "\n========== 计算结果 ==========" << std::endl;
            std::cout << "DBH = " << dbh << " cm" << std::endl;
        }
    } else if (dbh == 0) {
        result.error_message = "计算失败：数据不足";
    } else {
        result.error_message = "计算失败：不满足计算条件";
    }
    
    return result;
}

std::string DBHCalculator::findOBJFile(const std::string& filename) {
    // 尝试多个可能的路径
    std::vector<std::string> possiblePaths = {
        filename,  // 直接路径
        "../data/temp/" + filename,
        "../../data/temp/" + filename,
        "data/temp/" + filename,
        "./data/temp/" + filename
    };
    
    // 如果filename已经包含路径，只尝试直接路径
    if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
        if (fs::exists(filename)) {
            return filename;
        }
        return "";
    }
    
    // 否则尝试所有可能的路径
    for (const auto& path : possiblePaths) {
        if (fs::exists(path)) {
            std::cout << "找到文件：" << fs::absolute(path) << std::endl;
            return path;
        }
    }
    
    std::cerr << "未找到文件，尝试过以下路径：" << std::endl;
    for (const auto& path : possiblePaths) {
        std::cerr << "  - " << fs::absolute(path) << std::endl;
    }
    
    return "";
}

} // namespace metric