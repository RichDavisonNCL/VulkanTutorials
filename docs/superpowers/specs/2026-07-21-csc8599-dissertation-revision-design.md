# CSC8599 Dissertation 中文母稿重构设计

**日期：** 2026-07-21
**状态：** 已确认设计，等待作者文件级审核
**适用项目：** CSC8599 Academic Supervised Project
**论文工作标题：** *A Vulkan-based Evaluation of GPU-Driven Scene Management for PCG Modular Scenes*

## 1. 文档目的

本设计用于指导 `thesis.md` 的中文母稿重构，以及后续英文翻译、ACM 排版、软件整理和技术视频制作。当前阶段不受 20 页篇幅约束。工作顺序为：先建立内容完整且证据可靠的中文母稿，完成技术与数据核验，再翻译成英文，之后处理排版和压缩。

本设计只使用已有 benchmark、正式 CSV、独立复测和补充扫描。项目不新增 benchmark，不把缺失实验伪装成已完成内容。允许对已有数据进行重新汇总、计算、制图和一致性检查，也允许运行构建与单元测试来验证提交软件。

## 2. 项目背景与课程定位

CSC8599 的 Academic Supervised Project 由 dissertation 与 software 两部分构成，各占 50%。课程说明要求 dissertation 详细说明项目完成了什么、为何采用相应设计，并通过结果与评价证明目标达成。软件应能够被打开、理解和运行；技术视频用于展示项目实际工作状态。

导师批准的项目范围包括：

- 以 Wave Function Collapse 作为模块化场景生成方法和 case study；
- 在 Vulkan 中实现和比较三种 CPU/GPU 场景管理路径；
- 测量改变 pipeline usage 所产生的成本与收益；
- 分析场景规模、chunk 粒度、内容构成和局部更新的影响；
- 将 GPU-assisted WFC 视为 contingent extension。

因此，论文按工程评价型 dissertation 定位。贡献重点来自完整系统实现、受控比较、数据分析、失败区间识别和批判性反思。GPU-assisted WFC 未实施属于范围管理结果，放入 future work，不作为核心目标缺失处理。

## 3. 目标与非目标

### 3.1 中文母稿目标

1. 准确描述 WFC 场景生成、缓存、Split-SSBO、三种渲染路径和局部更新实现。
2. 用真实计时边界重新定义全部指标。
3. 保留四条具有辨识度且能由现有数据支持的工程观察。
4. 将观察、解释和待验证机制明确分层。
5. 公开说明数据 provenance、硬件范围和实现限制。
6. 展示个人在系统、工具、测试和分析方面的贡献。
7. 为英文稿、ACM 版本和技术视频提供单一内容基准。

### 3.2 非目标

- 不声称提出新的 WFC 算法。
- 不声称提出新的 Vulkan indirect drawing 原理。
- 不声称三个 pipeline stage 已被纯粹、独立地隔离。
- 不把局部 CPU interval 表述成端到端 frame time。
- 不把 combined GPU timestamp interval 归因到单一 GPU stage。
- 不把 weight tier 表述成实际 occupancy。
- 不声称结论能够直接推广到其他 GPU、相机、材质系统或生产引擎。
- 不新增 benchmark，不构造缺少来源的新数字。

## 4. 论文身份与核心研究问题

### 4.1 论文身份

论文是一项实现特定的 Vulkan 场景管理工程评价。WFC 负责生成具有规模、分块和内容构成变化的程序化 workload；三种 scene-management paths 提供 CPU/GPU 工作分配对照；自动化 benchmark 与分析工具负责观察这些实现路径的行为。

### 4.2 核心研究问题

建议英文版本：

> How do three implementation-specific CPU- and GPU-driven scene-management paths affect the measured CPU-side preparation and command-recording interval and the combined GPU timestamp interval when rendering modular scenes produced by a simplified WFC implementation, and how do these trade-offs vary with grid scale, chunk granularity, and observed geometry composition?

建议中文版本：

> 在渲染由简化 WFC 实现生成的模块化场景时，三种实现特定的 CPU/GPU 场景管理路径如何影响被测 CPU 侧准备与命令录制区间，以及组合 GPU 时间戳区间？这些权衡如何随网格规模、chunk 粒度和观测到的几何构成变化？

### 4.3 子问题

1. 从逐 chunk direct recording 切换到 CPU-written fixed-length indirect records，会怎样改变 CPU 侧准备与命令录制区间？
2. 将 visibility work 移入 compute path 后，被测 CPU 区间与组合 GPU 区间之间出现什么权衡？
3. WFC 输出的实际几何构成如何影响 GPU workload？
4. 在共享 scene representation 下，buffer update 的提交粒度如何影响 standalone upload cost？

## 5. 贡献模型

### 5.1 系统贡献

- 简化 WFC 模块场景生成与缓存；
- Split-SSBO 场景表示；
- 三种 Vulkan scene-management paths；
- CPU culling、compute culling、direct draws 和 indirect records；
- 局部 buffer update 路径；
- benchmark CLI、自动化 runner、CSV metadata、分析脚本与测试。

### 5.2 评价贡献

- 702 个 steady-state rendering configurations；
- 108 个 standalone update configurations；
- 每个正式配置包含 120 warmup frames 和 1200 within-run recorded frames；
- 75 份 selected-regime independent-process repeat validation；
- 独立的 geometry-composition supplementary sweeps。

### 5.3 工程观察

- indirect path 在中大规模测试配置下显著降低被测 CPU-side interval；
- fixed-length GPU-driven path 在极细 chunk 压力配置下产生很高的组合 GPU interval；
- 名义参数相近的程序化场景可能具有完全不同的几何 workload；
- batched buffer upload 相比逐 chunk submit-and-wait 具有显著优势。

## 6. 四条锁定主张

### 6.1 S1 到 S2 的 CPU-side interval

**数据来源：** `results/_aggregate.csv`。
**匹配范围：** 135 对 `grid >= 256`，相同 grid、chunk、weight tier 和 seed。
**计算：** `cpu_record_avg(S1) / cpu_record_avg(S2)`。
**范围：** 1.7366 到 9.9405。

允许表述：

> 在 135 对 grid≥256 的匹配配置中，从逐 chunk direct recording（S1）切换到 CPU culling 加主机写入 fixed-length indirect records（S2），使被测 CPU-side preparation and command-recording interval 下降 1.74–9.94×。

必须同时说明：S1 与 S2 共享 CPU frustum test；S2 还包含 indirect buffer map、十个字段的逐 chunk 重写、unmap、barrier recording 和两条 indirect API commands。因此，该比较反映整条实现路径差异，不能被描述为 isolated indirect-draw effect。

### 6.2 S3 的 236.2ms 压力点

**配置：** grid=4096、chunk=4、tier=50、seed=42。
**S1：** CPU-side interval 44,540.6µs；combined GPU interval 38,723.6µs。
**S2：** CPU-side interval 7,422.7µs；combined GPU interval 44,730.5µs。
**S3：** CPU-side interval 44.745µs；combined GPU interval 236,173µs。

允许表述：

> 在该压力配置中，S3 的报告 CPU-side interval 降至 44.7µs，而 combined GPU timestamp interval 达到 236.2ms。该结果展示了当前 fixed-length GPU-driven path 的明确 CPU/GPU 权衡。

必须同时说明：GPU timestamp 覆盖 buffer fill、barriers、compute、fixed-length indirect processing 和 graphics。现有 measurement 无法判定某个单独 stage 的责任。S3 的 O(N) visibility readback 位于 CPU measurement 之前，因此 44.7µs 不代表完整 CPU frame cost。

使用“观测到的压力点”，避免使用已定位的边界、饱和点或 compute dispatch cause。

### 6.3 WFC 几何构成与 11.36–11.37×

**配置：** grid=4096、chunk=16、tier=80、Scheme 3。
**seed42：** 52,879.3µs。
**seed1337：** 52,910.9µs。
**seed9999：** 600,999µs。
**比值：** 11.36–11.37×。

缓存解析：

- seed42：15,235,807 cube；500 sphere；
- seed1337：15,236,138 cube；510 sphere；
- seed9999：2,363 cube；14,703,079 sphere。

允许表述：

> 在 grid=4096、chunk=16、tier=80 的 Scheme 3 数据中，seed9999 的 combined GPU interval 比 seed42/1337 高 11.36–11.37×；缓存解析同时显示场景构成由近乎全 cube 转为近乎全 sphere。两项观测高度一致，补充扫描进一步支持几何构成与 GPU 时间之间的关联。

seed 只作为生成路径变量。几何构成是更直接的 workload descriptor。补充 sweep 使用另一 binary，必须被报告为独立 supplementary dataset。

### 6.4 Batched update 的约 21.8×

**数据来源：** `results/_aggregate_update.csv`。
**配置：** 32-chunk standalone buffer-upload microbenchmark。
**全部 seeds 与 schemes 汇总：** 87.07µs 对 1,898.44µs，21.80×。
**seed42 下三方案平均：** 86.47µs 对 1,896.53µs，21.93×。
**各方案范围：** 20.93–23.24×。

允许表述：

> 在 32-chunk standalone buffer-upload microbenchmark 中，单次 batched submit-and-wait 相比逐 chunk submit-and-wait 的朴素参考路径快约 21.8×。

必须同时说明：timer 覆盖 staging allocation、memcpy、copy command recording、submit 和 wait；CPU 数据修改位于计时起点之前。该结果属于提交粒度案例，不代表完整动态帧成本。

## 7. WFC 实现定位与不变量

### 7.1 Tile catalogue

当前 catalogue 包含：

- EMPTY；
- LOW、MID、HIGH 三类 cube；
- SPHERE_S、SPHERE_L 两类 sphere。

正文不得列出 cylinder。

### 7.2 Weight tier

`emptyWeight` 作用于一个 EMPTY tile；`otherWeight` 分别作用于五个非空 tile。输入权重组合 8:2、5:5、2:10 不等于 20%、50%、80% occupancy。中文母稿统一使用 low、medium、high weight tier 或 tier 20、50、80，并另行报告实测 occupancy。

4096² cache 的观测 occupancy 约为：

- tier 20：39.1%；
- tier 50：67.3%；
- tier 80：87.7–90.8%。

### 7.3 Singleton materialisation

输出 `grid` 以 EMPTY tile ID 0 初始化。adjacency matrix 中 EMPTY 与全部 tile 兼容，因此传播不会从任何 cell 的 possibility set 中移除 EMPTY。由传播产生的 singleton 必然为 `{EMPTY}`；输出 grid 的默认值已经表示该状态，无需额外写回。

这一行为属于当前 tile catalogue 的实现不变量。中文母稿可以简短解释，但不将它宣传为性能贡献。软件整理阶段增加注释和 invariant test，以防未来 adjacency 变化破坏该前提。

### 7.4 WFC 贡献边界

正文称其为 simplified WFC implementation，用于 deterministic modular workload generation。项目不评价 WFC 解质量，也不宣称对 WFC 算法作出创新。GPU-assisted WFC 仍放入 future work。

## 8. 数据来源与 provenance

### 8.1 Formal matrix

- 702 steady-state configurations；
- 108 update configurations；
- commit 记录为 `407efde`；
- `dirty=true`；
- executable hash 为 `e415726b9b02c3e5`；
- GPU 为 RTX 4080 SUPER；
- driver 为 610.47.0.0；
- Release build。

允许声称：正式矩阵内部共享相同 executable hash 和硬件环境。
禁止声称：当前 checkout 能够逐位重建历史 executable。

### 8.2 Repeat validation

- 五类 selected regimes；
- 三种 schemes；
- 每组五次 independent-process runs；
- 共 75 份 CSV。

用途是报告 selected regimes 的 run-level variation，不能代替整个正式矩阵的完整重复。

### 8.3 Supplementary sweeps

percolation/weight sweep 使用另一 commit、dirty state 和 executable hash。它们用于补充分析几何构成关联，不与 formal matrix 混成同一实验批次。

### 8.4 图表 metadata

每幅结果图和每张汇总表均应注明：

- dataset name；
- configuration range；
- metric；
- sample unit；
- commit；
- dirty state；
- executable hash。

## 9. 指标定义

### 9.1 CPU-side preparation and command-recording interval

该指标对应现有 `cpu_record`，可能包含：

- CPU frustum culling；
- host buffer mapping/writes；
- push constants；
- draw/dispatch/barrier command recording。

不同 scheme 包含的工作并不完全相同。正文使用完整名称，图表可保留字段名 `cpu_record`，图注必须定义边界。

### 9.2 CPU wait interval

`cpu_wait` 与 acquire/fence 或 swapchain 相关行为需按实际源码描述。它不能与 `cpu_record` 简单相加后自动成为端到端 frame time，除非边界得到源码证明。

### 9.3 Combined GPU timestamp interval

现有 `gpu_exec` 使用 top-to-bottom timestamp，覆盖方案命令区间内的组合 GPU 工作。该指标适合跨方案观察整体 GPU interval，缺少 stage-level attribution 能力。

### 9.4 Frame wall interval

现有 `frame_wall` 排除了若干发生在 measurement 起点之前或终点之后的工作，不能称为完整墙钟帧时间，也不能用于声明完整 throughput。

### 9.5 Standalone update interval

该指标只描述 isolated upload path。正文避免将它换算成“若干正常帧”；若需要提供 60Hz 语境，可以用 16.67ms budget 计算，并明确它仍未包含与 rendering integration 相关的调度效应。

## 10. 七章中文母稿设计

### 10.1 Introduction

承担任务：

1. 解释大型程序化场景给 CPU scene management 和 draw submission 带来的压力；
2. 说明 GPU-driven paths 可能减少 CPU work，同时增加 compute、synchronisation 和 indirect processing；
3. 引入 WFC 作为可控且内容敏感的 workload generator；
4. 给出收窄后的 RQ 和子问题；
5. 概括完成的软件系统、评价矩阵和四条工程观察；
6. 说明 GPU-assisted WFC 属于 contingent extension。

贡献按系统贡献、评价贡献和工程观察分组。删除领域唯一性、纯阶段分解和普遍优越性语言。

### 10.2 Background & Related Work

建议结构：

1. Vulkan command recording 与 direct/indirect drawing；
2. CPU/GPU visibility culling；
3. GPU-driven scene management 与 fixed/compacted command lists；
4. PCG/WFC 场景生成与 rendering workload；
5. GPU benchmark methodology；
6. Xylem 等相邻系统；
7. 本项目的实现特定定位。

定位句：

> 现有研究已经展示多种 GPU-driven rendering 技术。本项目在统一的程序化 workload 与共享渲染框架中，比较三条具体 Vulkan scene-management paths，并记录这些实现随规模、分块和几何构成变化时的工程权衡。

需要修正 Xylem、Gonakhchyan、Aokana 和 Unterguggenberger 的描述与元数据。删除缺少检索协议的“唯一”“尚无”“没有工作”等排他性措辞。

### 10.3 Design & Implementation

建议结构：

1. System Requirements and Scope；
2. Simplified WFC Scene Generator and Cache；
3. Split-SSBO Scene Representation；
4. Three Scene-Management Paths；
5. Synchronisation and Memory Flow；
6. Local Update Path；
7. Automation, Tests and Personal Contribution。

需要按源码写清：

- S1：CPU culling 加 per-chunk direct API commands；
- S2：CPU culling 加 host-written fixed-length indirect records；
- S2 每帧重写全部十个 indirect fields；
- S3：buffer clear、barrier、compute culling、barrier、fixed-length indirect processing；
- S3 shader 使用普通 stores；
- local size 为 64，每个 invocation 处理一个 chunk；
- grid4096/chunk4 对应 16,384 workgroups 和 1,048,576 invocations；
- 两条 indirect API commands 各处理 N 条 indirect records；
- visibility readback 的调用位置和作用；
- indirect buffer 的 host-visible/host-coherent 要求；
- local update timer 的起止边界。

本章应提供 personal contribution map，区分课程 Vulkan framework 与个人新增模块、工具、测试和数据 pipeline。

### 10.4 Evaluation Method

建议结构：

1. Parameter Matrix；
2. Hardware and Build；
3. Scene Generation and Cache Protocol；
4. Metric Boundaries；
5. Sampling and Within-Run Statistics；
6. Independent-Process Repeat Validation；
7. Dataset Provenance；
8. Threats to Validity。

Threats to validity 集中说明：

- 单 GPU；
- top-down 全可见相机；
- 单分辨率；
- 简单 meshes 和材质；
- naïve CPU culling；
- host-visible indirect buffer；
- fixed-length command list；
- S3 readback 位于 CPU interval 外；
- GPU interval 缺少 stage breakdown；
- 1200 frames 属于 within-run samples；
- 正式矩阵没有完整 independent-process repetitions；
- dirty artifact provenance；
- weight tier 与 occupancy 的区别。

### 10.5 Results & Evaluation

每节采用统一结构：

1. Evaluation question；
2. Data and configuration；
3. Direct observation；
4. Supported interpretation；
5. What the result cannot answer。

建议小节：

- Indirect Path and CPU-side Interval；
- CPU–GPU Trade-offs and the 236.2ms Stress Case；
- PCG Geometry Composition and GPU Workload；
- Standalone Buffer-Upload Granularity；
- Selected-Regime Repeat Validation。

不得使用 `max(cpu_record, gpu_exec)` 作为完整 frame bottleneck。可以并列展示两个 interval，讨论工作从 CPU 区间转向 GPU 区间的现象。

### 10.6 Discussion

围绕导师提出的 smart pipeline usage 展开：

- 减少逐 chunk API recording 对 CPU-side interval 有明确价值；
- 将工作移入 GPU path 需要同步观察组合 GPU interval；
- 极细 partition 会扩大 fixed-length workloads；
- PCG 系统应记录实际 geometry workload，而只记录 generator labels 会掩盖成本差异；
- batched submit-and-wait 优于频繁逐 chunk submit-and-wait；
- pipeline choice 依赖 workload、粒度、memory placement 和目标 metric。

讨论中将 236.2ms 作为有价值的负面工程观察。避免用未测量机制补足故事。

### 10.7 Conclusion, Reflection & Future Work

包含：

- 一段直接回答 RQ；
- 四条受限结论；
- 对系统设计成功部分的评价；
- 对 metric boundary、artifact provenance 和验证安排的反思；
- 说明若重新规划项目，会更早建立 clean artifact、run-level repeats 和 stage-level GPU timestamps；
- 将 visibility-ratio evaluation、compacted draws、多 GPU、device-local indirect buffers 和 GPU-assisted WFC 列入 future work。

结论不得引入 Results 中没有出现的新数字或新因果解释。

## 11. 术语表

全文统一：

| 当前易误用术语 | 中文母稿统一术语 |
|---|---|
| density 20/50/80% | weight tier 20/50/80 |
| CPU frame time | CPU-side preparation and command-recording interval |
| GPU execution time | combined GPU timestamp interval |
| two draw calls | two recorded indirect API commands |
| one workgroup per chunk | one invocation per chunk; 64 invocations per workgroup |
| atomic write | ordinary store to an exclusive indirect record |
| pipeline bottleneck | observed CPU/GPU interval trade-off |
| GPU culling saturation boundary | observed fine-granularity stress point |
| standard/full WFC | simplified WFC implementation |
| scene representation comparison | shared representation and standalone update-path evaluation |

## 12. 主张语言等级

### 12.1 数据直接支持

可使用：

- measured；
- observed；
- recorded；
- differed by；
- reduced the reported interval by。

### 12.2 数据支持的解释

可使用：

- indicates；
- is consistent with；
- suggests；
- coincided with；
- supports the interpretation that。

### 12.3 尚未验证的机制

使用：

- may；
- could；
- cannot be separated by the current measurement；
- requires stage-level profiling；
- remains future work。

任何带有“导致”“证明”“精确归因”的句子，都要检查是否存在对应消融或分段测量。缺少时降级为 observation 或 supported interpretation。

## 13. 引文与资料审计

### 13.1 必修项

- 更正 Xylem 对 compute culling 与 indirect rendering 的描述；
- 更正 Gonakhchyan 路线定位；
- 将 Aokana venue 改为 *Proceedings of the ACM on Computer Graphics and Interactive Techniques*, 8(1), I3D 2025；
- 澄清 Unterguggenberger 的 cullable condition 与实际 culled proportion；
- 区分 Vulkan 1.2 与 1.3 中 promoted-to-core 的特性；
- 区分 Vulkan 1.0 已有的 indirect draw/dispatch 与实验使用 Vulkan 1.3 的事实。

### 13.2 文献定位策略

Related Work 只需要证明：

- 项目理解已有 GPU-driven techniques；
- 三条 path 的设计有来源；
- 本项目选择统一 workload 做工程比较；
- 贡献来自实现、评价和受限观察。

不需要证明全领域没有相似工作。

## 14. AI 使用与人工核验

中文母稿中保留一份披露草案，英文定稿时根据课程和 ACM 要求调整。披露应包括：

- 使用的工具与版本；
- 使用范围，例如结构讨论、语言修改、代码或引文检查；
- 作者对所有源码、数据、数字、引用和结论进行人工核验；
- AI 工具不列为作者；
- 作者对作品内容承担责任。

AI 文风检测器不作为证据。质量控制以源码、数据和原始文献核验为准。

## 15. Software 交付设计

### 15.1 Project-specific README

README 至少覆盖：

- 项目目标；
- 基础 framework 来源；
- 个人贡献；
- hardware/software prerequisites；
- CMake/Visual Studio build；
- unit tests；
- interactive run；
- scheme selection；
- benchmark CLI；
- results directories；
- analysis scripts；
- known limitations；
- video link。

### 15.2 Contribution map

按模块列出：

- 课程提供或既有 Vulkan framework；
- WFC generator；
- GPUSceneManagement；
- compute shader；
- benchmark panel/CLI；
- runner scripts；
- analysis scripts；
- tests；
- figures/data artifacts。

每项说明作者完成的设计和代码范围。

### 15.3 Artifact manifest

建立清单：

- source snapshot；
- formal results；
- repeat validation；
- supplementary sweeps；
- aggregate CSV；
- analysis scripts；
- generated figures；
- executable hash metadata；
- known provenance limitations。

### 15.4 软件验证

不新增 benchmark。允许执行：

- clean build；
- existing unit tests；
- CLI help/smoke run；
- analysis script dry run；
- artifact path validation；
- README command verification。

任何源代码修正都需要测试证明，并明确说明正式结果仍来自历史 executable。

## 16. 技术视频设计

视频承担“项目确实工作”和“个人理解系统”的证明功能。建议结构：

1. 项目问题和三条 scene-management paths；
2. WFC scene generation 与 cache；
3. interactive scene 与 scheme switching；
4. CPU/GPU overlay 和 chunk count；
5. normal-scale comparison；
6. fine-granularity stress case；
7. geometry-family difference；
8. local update modes；
9. benchmark CLI、CSV 和 analysis scripts；
10. limitations 与 future work。

视频避免只展示静态截图。需要同时展示运行画面、关键源码或命令、指标定义和一条负面结果。

## 17. 工作阶段

### 17.1 阶段 A：中文母稿

- 建立术语表和 claim ledger；
- 修正技术事实；
- 重写 Design & Implementation；
- 重写 Evaluation Method；
- 组织 Results；
- 重写 Discussion；
- 重写 Introduction 和 contributions；
- 重写 Conclusion、reflection 与 future work；
- 中文全文稳定后编写 Abstract。

### 17.2 阶段 B：证据锁定

- 数字到 CSV 的追踪；
- 机制到源码的追踪；
- 文献到原始来源的追踪；
- 图表 metadata 检查；
- claim strength 检查；
- formal 与 supplementary 数据隔离检查。

### 17.3 阶段 C：英文翻译与语言重写

- 建立中英术语对照；
- 按学术英语重写；
- 保持数字、限制和主张强度不变；
- 删除重复自我认证语言；
- 完成标题、摘要、caption、AI disclosure 和 video link。

### 17.4 阶段 D：ACM 排版与压缩

- 使用 ACM template；
- 处理双栏图表；
- 合并重复内容；
- 压缩 related work；
- 将可复现细节移入 repository documentation；
- 逐页核对图注、引用、分页和 20 页上限。

## 18. 风险登记

| 风险 | 影响 | 应对 |
|---|---|---|
| dirty historical executable 无对应 patch | 限制精确源码复现 | 公开 commit/hash/dirty 状态，限定复现声明 |
| S3 readback 不在 CPU interval 内 | 不能声明完整 CPU frame cost | 使用准确指标名，在方法与讨论中披露 |
| GPU timestamp 无 stage breakdown | 不能解释 236.2ms 具体来源 | 保留整体压力观察，机制列入 future work |
| 单 GPU 与全可见相机 | 限制外推 | 将结论限定到测试平台和 workload |
| weight tier 与 occupancy 不同 | 可能误导场景密度解释 | 统一术语并报告实测 occupancy |
| supplementary sweep 使用另一 binary | 可能造成数据混写 | 独立 subsection、独立 metadata |
| 当前 README 面向教程集合 | 个人贡献和运行方式不清楚 | 编写 project-specific README |
| 中文到英文翻译扩大结论 | 证据强度漂移 | 中文母稿作为内容基准，翻译后逐条对照 |
| AI 辅助造成模板化重复或事实漂移 | 影响可信度 | 人工源码、数据、文献三重核验 |

## 19. 验收标准

### 19.1 中文母稿验收

- RQ、objectives、results 和 conclusion 一一对应；
- 四条亮点均使用锁定口径；
- 所有数字能追溯到数据文件；
- 所有机制能追溯到源码；
- 所有引文判断能追溯到原始来源；
- observation、interpretation、hypothesis 语言分级一致；
- 没有把局部 interval 写成完整 frame time；
- 没有把 weight tier 写成 occupancy；
- formal、repeat 和 supplementary 数据没有混写；
- WFC tile catalogue 与源码一致；
- S2/S3 实现描述与源码一致；
- limitation 覆盖计时、硬件、相机、artifact 和统计层级；
- personal reflection 说明项目决策与学习结果；
- GPU-assisted WFC 明确为未实施 contingent extension。

### 19.2 软件验收

- clean submission snapshot 能够构建；
- existing tests 通过；
- README 命令可执行；
- contribution map 清楚；
- artifact manifest 完整；
- 原始数据保持只读；
- analysis scripts 能找到所需输入；
- 已知限制清单与 dissertation 一致；
- 技术视频能够展示项目运行和核心取舍。

### 19.3 英文与 ACM 版本验收

- 英文术语与中文母稿一致；
- 数字和结论强度没有变化；
- video link 位于标题下方；
- ACM template 使用正确；
- 页数符合课程要求；
- 图表在双栏尺寸下可读；
- 引文格式和 metadata 正确；
- AI disclosure 符合课程与目标格式要求。

## 20. 分数策略

工作按 90 分交付标准执行，现实目标区间为 82–86。高分来源应包括：

- 较强的 Vulkan 工程难度；
- 完整的软件系统和自动化工具；
- 大规模现有数据；
- 对 measurement boundaries 的主动审计；
- 对负面结果的诚实分析；
- 清晰的个人贡献；
- 专业软件文档和技术视频；
- 成熟的 critical reflection。

无法通过文字完全消除的限制包括 dirty historical artifact、单 GPU、全可见相机、S3 readback 边界和缺少 GPU stage breakdown。设计目标是准确限定这些限制，并让 marker 看到作者能够识别证据边界、解释工程取舍并交付可运行系统。

## 21. 设计决策摘要

1. 采用工程评价型 dissertation 主线。
2. 保留标题和三条 scene-management paths。
3. 使用收窄后的 implementation-specific RQ。
4. 锁定四条亮点及其数据口径。
5. WFC 定位为 simplified implementation 和 workload generator。
6. 当前 singleton 行为由 EMPTY invariant 保证，不视为结果破坏。
7. formal、repeat 和 supplementary 数据分层报告。
8. 不新增 benchmark，充分利用已有数据与测试。
9. 先完成完整中文母稿，再翻译、排版和压缩。
10. software documentation、artifact manifest 和技术视频与论文并行推进。
