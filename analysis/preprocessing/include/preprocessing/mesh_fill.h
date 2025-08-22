#ifndef PREPROCESSING_MESH_FILL_H
#define PREPROCESSING_MESH_FILL_H

#include <string>
#include <vector>
#include <memory>

namespace preprocessing {

// 前向声明，避免在头文件中暴露CGAL类型
class MeshFillImpl;

// 填洞统计信息
struct HoleInfo {
    int boundary_edges;      // 洞的边界边数
    int faces_added;         // 填补时新增的面数
    bool success;           // 是否成功填补
};

// 网格统计信息
struct MeshStats {
    size_t num_vertices;
    size_t num_faces;
    size_t num_edges;
    size_t num_holes;
};

// 填洞结果
struct FillResult {
    bool success;                    // 整体是否成功
    MeshStats initial_stats;         // 填洞前统计
    MeshStats final_stats;           // 填洞后统计
    std::vector<HoleInfo> holes;     // 每个洞的信息
    std::string error_message;       // 错误信息（如果失败）
};

/**
 * @brief 网格填洞处理器类
 * 
 * 使用CGAL库检测并填补网格中的洞。
 * 采用PIMPL模式隐藏CGAL实现细节。
 */
class MeshFill {
public:
    /**
     * @brief 构造函数
     * @param verbose 是否输出详细信息到标准输出
     */
    explicit MeshFill(bool verbose = true);
    
    /**
     * @brief 析构函数
     */
    ~MeshFill();
    
    // 禁用拷贝构造和赋值
    MeshFill(const MeshFill&) = delete;
    MeshFill& operator=(const MeshFill&) = delete;
    
    // 允许移动构造和赋值
    MeshFill(MeshFill&&) noexcept;
    MeshFill& operator=(MeshFill&&) noexcept;
    
    /**
     * @brief 加载网格文件
     * @param filepath 输入文件路径（支持OBJ、OFF、PLY等格式）
     * @return 是否成功加载
     */
    bool loadMesh(const std::string& filepath);
    
    /**
     * @brief 获取当前网格统计信息
     * @return 网格统计信息
     */
    MeshStats getMeshStats() const;
    
    /**
     * @brief 检测网格中的洞
     * @return 检测到的洞的数量
     */
    size_t detectHoles();
    
    /**
     * @brief 填补所有检测到的洞
     * @param max_hole_size 最大洞尺寸限制（边界边数），-1表示不限制
     * @return 填洞结果
     */
    FillResult fillAllHoles(int max_hole_size = -1);
    
    /**
     * @brief 填补指定索引的洞
     * @param hole_index 洞的索引（0开始）
     * @return 填洞信息
     */
    HoleInfo fillHole(size_t hole_index);
    
    /**
     * @brief 保存网格到文件
     * @param filepath 输出文件路径
     * @return 是否成功保存
     */
    bool saveMesh(const std::string& filepath) const;
    
    /**
     * @brief 清空当前网格数据
     */
    void clear();
    
    /**
     * @brief 设置是否输出详细信息
     * @param verbose true表示输出详细信息
     */
    void setVerbose(bool verbose);
    
    /**
     * @brief 静态方法：直接处理文件
     * @param input_file 输入文件路径
     * @param output_file 输出文件路径
     * @param max_hole_size 最大洞尺寸限制
     * @param verbose 是否输出详细信息
     * @return 填洞结果
     */
    static FillResult processFile(const std::string& input_file,
                                   const std::string& output_file,
                                   int max_hole_size = -1,
                                   bool verbose = true);

private:
    std::unique_ptr<MeshFillImpl> pImpl;  // PIMPL模式实现
};

} // namespace preprocessing

#endif // PREPROCESSING_MESH_FILL_H