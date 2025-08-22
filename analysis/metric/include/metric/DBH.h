// analysis/metric/include/metric/DBH.h
#ifndef METRIC_DBH_H
#define METRIC_DBH_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "metric/height.h"  // 使用height.h中定义的Point3D

namespace metric {

// 树木模型类
class TreeModel {
private:
    std::vector<Point3D> vertices;
    std::string filename;
    
public:
    explicit TreeModel(const std::string& file);
    
    // 从OBJ文件加载模型
    bool loadFromOBJ(const std::string& filepath);
    
    // 获取指定高度范围内的点
    std::vector<Point3D> getPointsAtHeight(double height, double tolerance = 0.05) const;
    
    // 获取最低点高度
    double getMinHeight() const;
    
    // 获取顶点数量
    size_t getVertexCount() const { return vertices.size(); }
};

// DBH计算结果
struct DBHResult {
    bool success;           // 计算是否成功
    double dbh_cm;         // DBH值（厘米）
    std::string method_used; // 使用的方法
    std::string error_message; // 错误信息（如果失败）
};

// DBH计算器类
class DBHCalculator {
private:
    std::unique_ptr<TreeModel> model;
    double crownBaseHeight;
    
    // 从点云计算直径（简化版本）
    double calculateDiameterFromPoints(const std::vector<Point3D>& points) const;
    
    // 检测分叉数量（简化版本）
    int detectStemCount(const std::vector<Point3D>& points, double threshold = 0.3) const;
    
    // 方法1：合成胸径计算
    double calculateMethod1();
    
    // 方法2：锥度模型计算
    double calculateMethod2();
    
    // 查找OBJ文件路径
    std::string findOBJFile(const std::string& filename);
    
public:
    DBHCalculator();
    
    /**
     * 计算DBH
     * @param objFilePath OBJ文件路径（可以是文件名或完整路径）
     * @param crownBaseHeight 活冠基部高度（米）
     * @param verbose 是否输出详细信息
     * @return DBH计算结果
     */
    DBHResult calculate(const std::string& objFilePath, 
                       double crownBaseHeight, 
                       bool verbose = false);
    
    /**
     * 静态便捷方法：直接计算DBH
     * @param objFilePath OBJ文件路径
     * @param crownBaseHeight 活冠基部高度（米）
     * @param verbose 是否输出详细信息
     * @return DBH计算结果
     */
    static DBHResult calculateDBH(const std::string& objFilePath,
                                  double crownBaseHeight,
                                  bool verbose = false) {
        DBHCalculator calculator;
        return calculator.calculate(objFilePath, crownBaseHeight, verbose);
    }
};

} // namespace metric

#endif // METRIC_DBH_H