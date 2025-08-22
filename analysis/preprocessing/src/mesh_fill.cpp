#include "preprocessing/mesh_fill.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/boost/graph/IO/polygon_mesh_io.h>

#include <iostream>
#include <fstream>

// CGAL类型定义
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Surface_mesh<Kernel::Point_3> Mesh;
typedef boost::graph_traits<Mesh>::halfedge_descriptor halfedge_descriptor;
typedef boost::graph_traits<Mesh>::face_descriptor face_descriptor;
typedef boost::graph_traits<Mesh>::vertex_descriptor vertex_descriptor;

namespace PMP = CGAL::Polygon_mesh_processing;

namespace preprocessing {

// PIMPL实现类
class MeshFillImpl {
public:
    Mesh mesh;
    std::vector<halfedge_descriptor> border_cycles;
    bool verbose;
    
    explicit MeshFillImpl(bool v) : verbose(v) {}
    
    void log(const std::string& message) const {
        if (verbose) {
            std::cout << message << std::endl;
        }
    }
};

// MeshFill类实现

MeshFill::MeshFill(bool verbose) 
    : pImpl(std::make_unique<MeshFillImpl>(verbose)) {
}

MeshFill::~MeshFill() = default;

MeshFill::MeshFill(MeshFill&&) noexcept = default;
MeshFill& MeshFill::operator=(MeshFill&&) noexcept = default;

bool MeshFill::loadMesh(const std::string& filepath) {
    pImpl->log("正在读取文件: " + filepath);
    
    if (!CGAL::IO::read_polygon_mesh(filepath, pImpl->mesh)) {
        pImpl->log("错误: 无法读取文件 " + filepath);
        return false;
    }
    
    if (pImpl->verbose) {
        auto stats = getMeshStats();
        pImpl->log("成功读取网格:");
        pImpl->log("  顶点数: " + std::to_string(stats.num_vertices));
        pImpl->log("  面数: " + std::to_string(stats.num_faces));
        pImpl->log("  边数: " + std::to_string(stats.num_edges));
    }
    
    return true;
}

MeshStats MeshFill::getMeshStats() const {
    MeshStats stats;
    stats.num_vertices = pImpl->mesh.number_of_vertices();
    stats.num_faces = pImpl->mesh.number_of_faces();
    stats.num_edges = pImpl->mesh.number_of_edges();
    stats.num_holes = pImpl->border_cycles.size();
    return stats;
}

size_t MeshFill::detectHoles() {
    pImpl->border_cycles.clear();
    PMP::extract_boundary_cycles(pImpl->mesh, std::back_inserter(pImpl->border_cycles));
    
    if (pImpl->verbose) {
        pImpl->log("\n检测到 " + std::to_string(pImpl->border_cycles.size()) + 
                   " 个边界循环（洞）");
    }
    
    return pImpl->border_cycles.size();
}

HoleInfo MeshFill::fillHole(size_t hole_index) {
    HoleInfo info;
    info.success = false;
    info.boundary_edges = 0;
    info.faces_added = 0;
    
    if (hole_index >= pImpl->border_cycles.size()) {
        return info;
    }
    
    halfedge_descriptor h = pImpl->border_cycles[hole_index];
    
    // 计算洞的大小
    halfedge_descriptor hc = h;
    do {
        info.boundary_edges++;
        hc = next(hc, pImpl->mesh);
    } while(hc != h);
    
    if (pImpl->verbose) {
        pImpl->log("  洞 " + std::to_string(hole_index + 1) + 
                   " 的边界边数: " + std::to_string(info.boundary_edges));
    }
    
    // 填补洞
    std::vector<face_descriptor> patch_faces;
    PMP::triangulate_hole(pImpl->mesh, h, std::back_inserter(patch_faces));
    
    info.faces_added = patch_faces.size();
    info.success = !patch_faces.empty();
    
    if (pImpl->verbose) {
        if (info.success) {
            pImpl->log("    成功填补! 新增面数: " + std::to_string(info.faces_added));
        } else {
            pImpl->log("    警告: 填补失败!");
        }
    }
    
    return info;
}

FillResult MeshFill::fillAllHoles(int max_hole_size) {
    FillResult result;
    result.success = true;
    result.initial_stats = getMeshStats();
    
    size_t num_holes = detectHoles();
    
    if (num_holes == 0) {
        pImpl->log("网格中没有检测到洞，无需填补。");
        result.final_stats = result.initial_stats;
        return result;
    }
    
    // 填补每个洞
    for (size_t i = 0; i < pImpl->border_cycles.size(); ++i) {
        if (pImpl->verbose) {
            pImpl->log("\n正在填补第 " + std::to_string(i + 1) + " 个洞...");
        }
        
        // 如果设置了最大洞尺寸，先检查
        if (max_hole_size > 0) {
            halfedge_descriptor h = pImpl->border_cycles[i];
            int hole_size = 0;
            halfedge_descriptor hc = h;
            do {
                hole_size++;
                hc = next(hc, pImpl->mesh);
            } while(hc != h);
            
            if (hole_size > max_hole_size) {
                if (pImpl->verbose) {
                    pImpl->log("  跳过（超过最大尺寸限制 " + 
                               std::to_string(max_hole_size) + "）");
                }
                HoleInfo info;
                info.boundary_edges = hole_size;
                info.faces_added = 0;
                info.success = false;
                result.holes.push_back(info);
                continue;
            }
        }
        
        HoleInfo info = fillHole(i);
        result.holes.push_back(info);
        
        if (!info.success) {
            result.success = false;
        }
    }
    
    // 更新最终统计
    result.final_stats = getMeshStats();
    
    // 再次检查是否还有洞
    size_t remaining_holes = detectHoles();
    result.final_stats.num_holes = remaining_holes;
    
    if (pImpl->verbose) {
        pImpl->log("\n填补后的网格统计:");
        pImpl->log("  顶点数: " + std::to_string(result.final_stats.num_vertices));
        pImpl->log("  面数: " + std::to_string(result.final_stats.num_faces));
        pImpl->log("  边数: " + std::to_string(result.final_stats.num_edges));
        pImpl->log("  剩余洞数: " + std::to_string(remaining_holes));
    }
    
    return result;
}

bool MeshFill::saveMesh(const std::string& filepath) const {
    pImpl->log("\n正在保存文件: " + filepath);
    
    if (!CGAL::IO::write_polygon_mesh(filepath, pImpl->mesh)) {
        pImpl->log("错误: 无法保存文件 " + filepath);
        return false;
    }
    
    pImpl->log("成功保存填补后的网格!");
    pImpl->log("输出文件位置: " + filepath);
    return true;
}

void MeshFill::clear() {
    pImpl->mesh.clear();
    pImpl->border_cycles.clear();
}

void MeshFill::setVerbose(bool verbose) {
    pImpl->verbose = verbose;
}

// 静态便利方法
FillResult MeshFill::processFile(const std::string& input_file,
                                  const std::string& output_file,
                                  int max_hole_size,
                                  bool verbose) {
    MeshFill filler(verbose);
    FillResult result;
    
    if (!filler.loadMesh(input_file)) {
        result.success = false;
        result.error_message = "Failed to load mesh from: " + input_file;
        return result;
    }
    
    result = filler.fillAllHoles(max_hole_size);
    
    if (result.success || result.final_stats.num_faces > result.initial_stats.num_faces) {
        if (!filler.saveMesh(output_file)) {
            result.success = false;
            result.error_message = "Failed to save mesh to: " + output_file;
        }
    }
    
    return result;
}

} // namespace preprocessing