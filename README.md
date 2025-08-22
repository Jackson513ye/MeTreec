# MeTreec v1.0 <img src="MeTreec_logo.jpg" alt="logo" height="30" />

<p align="right">
    <b><img src="https://img.shields.io/badge/Supported%20Platforms-Windows%20%7C%20macOS%20%7C%20Linux-green" /></b><br>
</p>

**MeTreec** 是一个面向单木点云的“一站式”处理流水线：  
接收已分割的单木点云（`.xyz`，Z 轴朝上），调用 **AdTree**（Du et al., 2019）完成 3D 重建，随后基于 CGAL 进行网格修复（填洞），并从骨架或筛选叶节点中计算树木关键指标——**树高**、**胸径（DBH）**、**冠幅半径（CR）**、**冠幅深度（CD）**、**体积/表面积** 等，同时输出单棵 JSON 报告与批量 CSV 汇总。

## ✨ 特性一览

- **完整流水线**：.xyz → AdTree 建模 → 网格填洞 → 骨架/叶节点筛选 → 指标计算 → 报告导出
- **稳健依赖**：C++17 / CMake，核心几何依赖 CGAL，并使用 Boost、Eigen3
- **可控模块**：可通过 CMake 选项启用/禁用 preprocessing、metric、pipeline 等模块
- **AdTree 灵活**：可使用本仓内置 reconstruction/AdTree/ 构建，也可指定外部可执行文件
- **多种指标**：h_t、DBH、CR、CD、体积/表面积（含网格闭合性检查）等
- **完善的落地产物**：模型 .obj/.ply、筛选节点 .xyz/.ply、逐木 JSON 与批量 CSV 报告

---

## 📦 仓库结构

```text
MeTreec/
├── CMakeLists.txt               # 顶层工程：模块开关、安装与导出
├── README.md
├── analysis/
│   ├── preprocessing/           # 预处理：网格填洞、骨架叶节点筛选
│   └── metric/                  # 指标计算
├── pipeline/                    # 流水线编排（命令行入口）
├── reconstruction/AdTree/       # 内置 AdTree
└── data/input/                  # 测试点云
```

> 输入前提：确保输入 .xyz 是单木且 Z 轴向上（无地面/围栏等背景）。

---

## 🔧 依赖与环境

- CMake ≥ 3.14
- C++ 编译器支持 C++17
- CGAL（建议 ≥ 5.x）
- Boost
- Eigen3

示例（Ubuntu/Debian）：

```bash
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev libcgal-dev libeigen3-dev
```

---

## 🏗️ 构建

```bash
# 构建 MeTreec
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release          -DBUILD_PREPROCESSING=ON          -DBUILD_METRIC=ON          -DBUILD_PIPELINE=ON          -DBUILD_ADTREE=OFF
cmake --build . -j

# 构建内置 AdTree
cd reconstruction/AdTree
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

---

## ▶️ 运行

方式 A：默认路径模式（零参数）

```bash
./bin/TreePipeline
```

默认目录：
- 输入：data/input/
- AdTree 原始输出：data/output/models/
- 报告：data/output/report/
- 中间件：data/temp/

方式 B：显式指定输入输出

```bash
./bin/TreePipeline <input_path> <output_dir> [options]
```

常用选项：
```
--adtree-exe <path>   指定 AdTree 可执行路径
--no-fill             不进行网格填洞
--max-hole-size <n>   最大填洞尺寸
--no-skeleton         不处理骨架
--no-volume           不计算体积
--no-crown            不计算冠幅
--filter-ratio <n>    叶节点筛选比例
--verbose             输出详细日志
```

---

## 📤 输出与命名

以 03998.xyz 为例：

- AdTree 原始输出
  - 03998_branches.obj
  - 03998_leaves.obj
  - 03998_skeleton.ply

- 管线产物
  - 03998_branches_filled.obj
  - 03998_skeleton_filtered_leaves.xyz/.ply

- 报告
  - 03998_xxxx.json
  - summary_xxxx.csv

JSON 示例：

```json
{
  "metrics": {
    "height": 18.53,
    "crown_depth": 9.58,
    "dbh": { "value_cm": 28.41, "method": "合成胸径法" },
    "crown": { "radius": 3.25, "aspect_ratio": 1.22 },
    "volume": { "value_m3": 0.84, "surface_area_m2": 12.73 }
  }
}
```

---

## 🧠 指标计算方法

- 高度：取最高 N 个叶节点估计
- 冠幅深度：CD = h_t - h0，h0 取最低 N 个叶节点
- 冠幅半径：投影至 XY 平面，取凸包与最小包围圆
- 胸径：在 1.3/1.0/0.7 m 切片点云估算，含分干检测
- 体积/表面积：CGAL 修复网格并测量

---

## ⚙️ 故障排查

- 找不到 AdTree → 显式指定路径或构建内置 AdTree
- 体积为 0 → 检查填洞，必要时调整 --max-hole-size
- DBH 失败 → 分干/数据不足时会中止
- 高度异常 → 确认 Z 轴朝上

---

## 数据

`data/input/` 提供测试点云，来自 FOR-species20K 数据集。

---

## 参考文献

Bauwens, S., Ploton, P., Fayolle, A., et al. (2021). A 3D approach to model the taper of irregular tree stems: Making plots biomass estimates comparable in tropical forests. Ecological Applications, 31(8), e02451.

Du, S., Lindenbergh, R., Ledoux, H., Stoter, J., & Nan, L. (2019). AdTree: Accurate, Detailed, and Automatic Modelling of Laser-Scanned Trees. *Remote Sensing*, 11(18), 2074.

Fan, G., Nan, L., Chen, F., Dong, Y., Wang, Z., Li, H., & Chen, D. (2020). A New Quantitative Approach to Tree Attributes Estimation Based on LiDAR Point Clouds. *Remote Sensing*, 12(11), 1779.

Puliti, S., Lines, E. R., Müllerová, J., et al. (2025). Benchmarking tree species classification from proximally sensed laser scanning data: Introducing the FOR-species20K dataset. *Methods in Ecology and Evolution*, 16(4), 801–818.
