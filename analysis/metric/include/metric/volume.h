#ifndef METRIC_VOLUME_H
#define METRIC_VOLUME_H

#include <string>
#include <vector>

namespace metric {

// 体积计算结果
struct VolumeResult {
    bool success = false;
    bool is_closed = false;          // 网格是否封闭
    int num_vertices = 0;            // 顶点数
    int num_faces = 0;               // 面数
    double volume = 0.0;             // 体积（立方米）
    double surface_area = 0.0;       // 表面积（平方米）
    double bbox_volume = 0.0;        // 边界框体积
    double volume_ratio = 0.0;       // 体积占比（%）
    double bbox_x_min = 0.0, bbox_x_max = 0.0;
    double bbox_y_min = 0.0, bbox_y_max = 0.0;
    double bbox_z_min = 0.0, bbox_z_max = 0.0;
    std::string error_message;
};

class TreeVolume {
public:
    // 从OBJ文件计算体积
    static VolumeResult calculateFromOBJ(
        const std::string& obj_file,
        bool verbose = false
    );
    
    // 批量处理目录中的所有OBJ文件
    static std::vector<VolumeResult> processBatch(
        const std::string& directory,
        bool verbose = false
    );
    
    // 获取体积统计信息
    struct VolumeStatistics {
        int total_files = 0;
        int successful = 0;
        int closed_meshes = 0;
        double total_volume = 0.0;
        double avg_volume = 0.0;
        double min_volume = 0.0;
        double max_volume = 0.0;
        std::string min_volume_file;
        std::string max_volume_file;
    };
    
    static VolumeStatistics calculateStatistics(
        const std::vector<VolumeResult>& results
    );
};

} // namespace metric

#endif // METRIC_VOLUME_H