#include "metric/volume.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/OBJ.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <limits>

namespace metric {

namespace fs = std::filesystem;

// 定义内核和多面体类型
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef Kernel::Point_3 Point_3;

namespace PMP = CGAL::Polygon_mesh_processing;

// 从OBJ文件计算体积
VolumeResult TreeVolume::calculateFromOBJ(
    const std::string& obj_file,
    bool verbose) {
    
    VolumeResult result;
    result.success = false;
    result.is_closed = false;
    
    // 获取文件名（用于存储在结果中）
    fs::path filepath(obj_file);
    std::string filename = filepath.filename().string();
    
    if (verbose) {
        std::cout << "处理OBJ文件: " << filename << std::endl;
    }
    
    try {
        // 打开文件
        std::ifstream input(obj_file);
        if (!input) {
            result.error_message = "无法打开文件";
            return result;
        }
        
        // 读取点和面
        std::vector<Point_3> points;
        std::vector<std::vector<std::size_t>> faces;
        
        if (!CGAL::IO::read_OBJ(input, points, faces)) {
            result.error_message = "无法读取OBJ格式";
            return result;
        }
        
        input.close();
        
        if (verbose) {
            std::cout << "  读取到 " << points.size() << " 个顶点, " 
                      << faces.size() << " 个面" << std::endl;
        }
        
        // 构建网格
        Polyhedron mesh;
        PMP::orient_polygon_soup(points, faces);
        PMP::polygon_soup_to_polygon_mesh(points, faces, mesh);
        
        if (mesh.empty()) {
            result.error_message = "生成的网格为空";
            return result;
        }
        
        // 基本信息
        result.num_vertices = mesh.size_of_vertices();
        result.num_faces = mesh.size_of_facets();
        
        // 修复网格
        PMP::remove_isolated_vertices(mesh);
        if (!PMP::is_outward_oriented(mesh)) {
            PMP::reverse_face_orientations(mesh);
            if (verbose) {
                std::cout << "  已修正面片方向" << std::endl;
            }
        }
        
        // 检查封闭性
        result.is_closed = CGAL::is_closed(mesh);
        
        if (verbose) {
            std::cout << "  网格状态: " << (result.is_closed ? "封闭" : "开放") << std::endl;
        }
        
        // 计算体积和表面积
        if (result.is_closed) {
            result.volume = std::abs(CGAL::to_double(PMP::volume(mesh)));
        } else {
            // 对于非封闭网格，尝试估算体积
            result.volume = std::abs(CGAL::to_double(PMP::volume(mesh)));
            if (verbose) {
                std::cout << "  警告: 网格非封闭，体积可能不准确" << std::endl;
            }
        }
        
        result.surface_area = CGAL::to_double(PMP::area(mesh));
        
        // 计算边界框
        auto bbox = CGAL::bbox_3(mesh.points_begin(), mesh.points_end());
        result.bbox_x_min = bbox.xmin();
        result.bbox_x_max = bbox.xmax();
        result.bbox_y_min = bbox.ymin();
        result.bbox_y_max = bbox.ymax();
        result.bbox_z_min = bbox.zmin();
        result.bbox_z_max = bbox.zmax();
        
        result.bbox_volume = (bbox.xmax() - bbox.xmin()) *
                            (bbox.ymax() - bbox.ymin()) *
                            (bbox.zmax() - bbox.zmin());
        
        result.volume_ratio = (result.bbox_volume > 0) ?
                             (result.volume / result.bbox_volume * 100) : 0;
        
        result.success = true;
        
        if (verbose) {
            std::cout << "  体积: " << result.volume << " 立方米" << std::endl;
            std::cout << "  表面积: " << result.surface_area << " 平方米" << std::endl;
            std::cout << "  体积占比: " << result.volume_ratio << "%" << std::endl;
        }
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
        if (verbose) {
            std::cerr << "  错误: " << e.what() << std::endl;
        }
    }
    
    return result;
}

// 批量处理目录中的所有OBJ文件
std::vector<VolumeResult> TreeVolume::processBatch(
    const std::string& directory,
    bool verbose) {
    
    std::vector<VolumeResult> results;
    
    if (!fs::exists(directory)) {
        if (verbose) {
            std::cerr << "错误: 目录不存在 - " << directory << std::endl;
        }
        return results;
    }
    
    // 收集所有OBJ文件
    std::vector<std::string> obj_files;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".obj") {
            obj_files.push_back(entry.path().string());
        }
    }
    
    // 按文件名排序
    std::sort(obj_files.begin(), obj_files.end());
    
    if (verbose) {
        std::cout << "找到 " << obj_files.size() << " 个OBJ文件" << std::endl;
    }
    
    // 处理每个文件
    for (size_t i = 0; i < obj_files.size(); ++i) {
        if (verbose) {
            std::cout << "[" << (i+1) << "/" << obj_files.size() << "] ";
        }
        
        VolumeResult result = calculateFromOBJ(obj_files[i], verbose);
        results.push_back(result);
    }
    
    return results;
}

// 计算统计信息
TreeVolume::VolumeStatistics TreeVolume::calculateStatistics(
    const std::vector<VolumeResult>& results) {
    
    VolumeStatistics stats;
    stats.total_files = results.size();
    stats.min_volume = std::numeric_limits<double>::max();
    stats.max_volume = 0;
    
    for (const auto& r : results) {
        if (r.success) {
            stats.successful++;
            if (r.is_closed) {
                stats.closed_meshes++;
            }
            
            stats.total_volume += r.volume;
            
            if (r.volume < stats.min_volume) {
                stats.min_volume = r.volume;
                fs::path p(r.error_message);  // 这里应该存储文件名
                stats.min_volume_file = p.filename().string();
            }
            if (r.volume > stats.max_volume) {
                stats.max_volume = r.volume;
                fs::path p(r.error_message);  // 这里应该存储文件名
                stats.max_volume_file = p.filename().string();
            }
        }
    }
    
    if (stats.successful > 0) {
        stats.avg_volume = stats.total_volume / stats.successful;
    }
    
    return stats;
}

} // namespace metric