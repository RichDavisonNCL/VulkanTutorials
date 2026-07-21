# A Vulkan-based Evaluation of GPU-Driven Rendering for PCG Modular Scenes

**Author:** [Your Name] · [Your University] · [Date]

---

## Abstract

程序化内容生成(PCG)技术的进步使得实时渲染场景的规模迅速增长,但 GPU 驱动渲染管线的各部分从 GPU 执行中受益的程度并不均匀。本文在 Vulkan 1.3 下构建了一个统一的 WFC 场景实验框架,通过对比三种渲染方案——CPU 实例化(基准)、CPU 剔除+间接绘制、GPU 计算剔除+间接绘制——定量分解了场景表示更新、可见性剔除和绘制提交三个管线阶段的 GPU 化收益。实验在 Release 构建下覆盖 9 个网格规模(16² 到 4096²)、3 个块大小、3 种场景密度和 3 个随机种子,共 810 个配置,每个配置采集 1200 帧数据。结果表明:**(1)** 绘制提交是 GPU 驱动收益最直接的来源——仅将绘制机制由直接绘制改为间接绘制(方案 1→2),CPU 录制成本在中大规模场景(grid≥256)下下降 1.7--9.9×,典型约 5--8×,低密度场景偏高;**(2)** 将剔除进一步迁移到 GPU(方案 2→3)带来叠加收益,且该收益随块数增加而扩大(3.2--32×),两阶段合计使方案 3 的 CPU 录制成本恒定于 7--45µs,相比方案 1 的差距达 21--995×(seed42/dens50 参考配置,跨 chunk 与 grid);**(3)** WFC 场景内容对 GPU 负载有显著影响,同一密度参数下不同随机种子可产生 11.4× 的 GPU 执行时间差异(该比值随网格增大而单调递减,从 256² 时的 18.0× 降至 4096² 时的 11.4×,提示为有限样本下的单场景观察而非普适规律);**(4)** 局部更新成本受方案影响很小(批处理提交下最大差异 <12%),提示场景表示层面的更新路径主要由提交策略而非渲染方案决定。这些发现为程序化生成场景的实时渲染管线设计提供了定量参考。

**Keywords:** GPU-driven rendering, Vulkan, procedural content generation, Wave Function Collapse, indirect draw, scene management

---

# 1. Introduction

## 1.1 Motivation

程序化内容生成(PCG)技术的进步使得实时渲染场景的规模迅速增长。以 Wave Function Collapse (WFC) [Gumin 2016] 为例,一个参数化的 2D 瓦片网格可以在几秒到几分钟内生成包含数百万个三维实例的场景。在我们的测试中,一个 4096×4096 的 WFC 网格在 80% 密度配置下产生了超过 1500 万个实例(立方体和球体),分布在 65536 个空间块中。这种规模的场景对渲染管线提出了严峻的挑战:CPU 需要为每一帧遍历数万个块,决定哪些块可见,并为每个可见块提交绘制命令。

Vulkan 1.3 引入了多个关键特性,使得将渲染管线的大部分工作从 CPU 卸载到 GPU 成为可能。间接绘制(`vkCmdDrawIndexedIndirect`)允许 GPU 从内存中读取绘制参数,而不是依赖 CPU 逐对象提交;计算着色器可以执行剔除、LOD 选择等通常由 CPU 处理的工作;动态渲染(`VK_KHR_dynamic_rendering`)简化了渲染通道的管理。这些特性共同构成了"GPU 驱动渲染"的技术基础 [Haar & Aaltonen 2015]。

然而,并非管线的所有阶段都同样受益于 GPU 执行。将剔除从 CPU 迁移到 GPU 需要引入计算着色器的调度开销和管线屏障同步;使用间接绘制虽然减少 CPU 的逐个绘制调用,但不会减少 GPU 需要处理的几何体总量;场景数据的表示和更新方式(如 SSBO 的上传路径)可能完全不受渲染策略的影响。简而言之,**GPU 驱动的收益在管线的不同阶段是不均匀的,有些阶段可能根本不受益**。

此外,程序化场景引入了一个额外的维度:相同参数配置下,不同随机种子生成的场景虽然统计属性相似(如总瓦片数、密度),但空间布局可能差异显著。如果这种差异导致 GPU 渲染性能的显著波动,那么 PCG 管线就需要考虑渲染性能反馈——而这是一个在文献中几乎未被探讨的问题。

因此,在统一的实验框架下,对 GPU 驱动管线各阶段进行定量的收益分解,不仅具有工程实践价值,也对理解 PCG 场景在现代 GPU 硬件上的行为具有理论意义。

## 1.2 Research Question

本文旨在回答以下核心研究问题:

> **在处理密集的程序化生成环境(PCG modular scenes)时,Vulkan 管线的哪些部分——场景表示(scene representation)、可见性剔除(visibility culling)、绘制提交(draw submission)——从 GPU 驱动执行中受益最大?**

具体而言,本文通过构建三种渲染方案——CPU 实例化(基准)、CPU 剔除+间接绘制、GPU 计算剔除+间接绘制——在 9 个网格规模(16² 到 4096²)、3 个块大小、3 种场景密度和 3 个随机种子上进行系统对比,定量分解每个阶段的 GPU 化收益。

## 1.3 Contributions

本文的主要贡献包括:

本文贡献可分为两类:两项独立的定量发现,以及三项对既有工程经验的定量验证。

**独立发现:**

1. **三阶段的统一定量分解框架。** 在 Vulkan 1.3 下,使用共享的 WFC 场景和 SSBO 数据布局,对三个管线阶段分别进行公平的定量对照。实验在 Release 构建下覆盖 702 个渲染配置和 108 个局部更新配置,每个配置采集 1200 帧数据。

2. **绘制提交与剔除迁移的收益是叠加且各自可分离测量的。** 将直接绘制改为间接绘制(方案 1→方案 2,剔除仍在 CPU)直接降低 CPU 录制成本;在此基础上将剔除迁移到 GPU(方案 2→方案 3,间接绘制不变)带来进一步的叠加收益,且该收益随块数增加而扩大。两阶段叠加后,方案 3 的 CPU 录制成本在中大规模场景下相比方案 1 低一到三个数量级。在本文的全可见相机条件下,CPU 端视锥体剔除在中大规模场景中并未表现为负收益。具体数值、收益分布及其适用范围见 §5.1--§5.2;构建配置效度说明见 §4.3.4。

**对既有工程经验的定量验证(而非新颖发现):**

3. **WFC 场景方差对 GPU 负载有显著影响,成因已定位到网格构成的家族级翻转。** 同一密度参数(80%)下,种子 9999 相对种子 42/1337 的 GPU 执行时间差异在 4096² 网格下为 11.4×。直接解析 WFC 缓存二进制文件证实:种子 42/1337 坍缩出的场景中 99.997% 的非空瓦片属于立方体家族,而种子 9999 坍缩出的场景中 99.98% 属于球体家族——这是网格构成的近乎完全翻转,而非"更多大型 tile"。球体网格(768 个三角形)相对立方体网格(12 个三角形)有 64× 的三角形数差异,是 GPU 执行时间差异的主要驱动来源(见 §5.3.1、§6.3)。对生成器代码和大量额外种子的独立验证进一步确认:这种家族级锁定并非罕见的单点异常,而是该邻接规则在 density=80% 下的普遍行为(见 §5.3.1)。

4. **局部更新成本对渲染方案的敏感度较低。** 在三种渲染方案下,基于 staging buffer + `vkCmdCopyBuffer` 的批处理更新路径产生的成本差异不超过 12%。由于三个方案在本文实现中共享同一条更新代码路径(见 §3.2),这一结果在很大程度上是实现结构的必然产物,此处作为对该设计前提的定量验证而非独立发现报告。

5. **批处理提交优于逐块同步提交,幅度符合预期。** 批处理单次提交相比逐块同步提交在 32 个块时实现 21.9× 加速(三方案均值)。这一结果符合"避免多次同步 fence 等待"这一常见 Vulkan 工程实践的预期,此处提供的是具体场景下的量化数据,而非新的工程原理。

**研究范围说明:** 本文不贡献 WFC 算法本身,也不声称 WFC 优于其他程序化生成方法。WFC 在本工作中作为场景生成工具,为 GPU 驱动渲染评测提供可控且可复现的测试平台。

## 1.4 Thesis Structure

本文的其余部分组织如下。第二章回顾 Vulkan 1.3 管线、GPU 驱动渲染技术(按场景表示、可见性剔除、绘制提交三个阶段组织)和 WFC 场景生成,并定位现有工作的空白。第三章描述系统设计,包括 WFC 场景生成器与缓存机制、Split-SSBO 场景表示架构、三种渲染方案的详细设计和测量基础设施。第四章给出实验设计,包括自变量和参数空间、场景生成协议和测量协议。第五章呈现结果与分析,按稳态渲染成本、CPU/GPU 瓶颈交叉、场景内容敏感性和局部更新成本分为四节。第六章讨论发现的意义,直接回答研究问题。第七章总结全文,列出局限性和未来工作方向。

---

# 2. Background & Related Work

## 2.1 Modern GPU Rendering Pipeline (Vulkan 1.3)

### 2.1.1 Key Vulkan 1.3 Features

Vulkan 1.3 引入了几项与 GPU 驱动渲染直接相关的核心特性。**动态渲染**(`VK_KHR_dynamic_rendering`)消除了对显式渲染通道对象的需求,允许应用程序在命令缓冲区中直接开始和结束渲染,减少了样板代码和对象管理开销。**同步 2**(`VK_KHR_synchronization2`)简化了管线屏障的语义,将屏障操作从基于访问标志的模型统一为基于管线阶段的模型。**标量块布局**(`VK_EXT_scalar_block_layout`)允许类似 C 的结构在 SSBO 中直接映射,无需手动填充对齐。**描述符索引**(`VK_EXT_descriptor_indexing`)允许着色器动态索引描述符数组,是实现无绑定渲染(bindless rendering)的基础构件。

在这些特性中,对本文三个渲染方案设计最关键的是**间接绘制**和**计算着色器**。

### 2.1.2 Indirect Drawing

间接绘制(`vkCmdDrawIndexedIndirect` 和 `vkCmdDrawIndirect`)是现代 GPU 驱动渲染的核心机制。与传统的直接绘制不同——后者要求 CPU 在每次绘制调用前通过 Vulkan API 显式指定顶点数、实例数和起始偏移——间接绘制允许 GPU 从设备本地或主机可见的缓冲区中读取这些参数。这意味着绘制调用的参数可以由 GPU 自己生成(例如通过计算着色器写入),而 CPU 只需发出一次命令即可绘制整个实例集合。

在本文的方案 2 和方案 3 中,间接绘制使得所有块的 cube 和 sphere 分别仅需**两次**绘制调用——不论块的数量是 256 还是 10⁶。方案 1 则没有使用间接绘制,每个块每种网格需要一个单独的 `vkCmdDrawIndexed` 调用,因此绘制调用数与块数量线性相关。

### 2.1.3 Compute Shaders for GPU Work Generation

计算着色器是方案 3 的 GPU 剔除核心。计算着色器运行在 GPU 的计算队列或图形队列上,能够直接访问 SSBO 中的数据。在方案 3 中,每个线程组处理一个块:从 ChunkBuffer(详见 §3.2)读取块的 AABB,使用推送到计算管线的视锥体平面进行相交测试,如果块可见则通过原子操作将其实例数写入间接绘制缓冲区。计算着色器调度完成后,需要一个**管线屏障**将写入的间接缓冲区从计算阶段过渡到间接绘制阶段,确保 GPU 在开始读取间接绘制命令前所有写入已完成。

### 2.1.4 GPU Timestamp Queries

本文使用 GPU 时间戳查询(`vkCmdWriteTimestamp2`)来测量每个方案的 GPU 执行时间,与基于 CPU 的 QPC 计时并行。每帧在命令缓冲区的顶部(`eTopOfPipe`)和底部(`eBottomOfPipe`)写入时间戳。所有时间戳在录制的帧数结束时通过 `vkGetQueryPoolResults` 批量读回,从而将 GPU 执行时间与 CPU 录制时间分离。GPU 时间戳周期(在 RTX 4080 SUPER 上通常为几十分之一纳秒)通过 `VkPhysicalDeviceProperties::limits::timestampPeriod` 获取。由于方案 3 的 CPU 录制完成后 GPU 异步执行,本文的 `frame_wall` 指标对方案 3 而言不是端到端延迟指标——它只反映 CPU 录制耗时。`gpu_exec`(从时间戳推导的 GPU 执行时间,定义见 §3.4.3)才是正确的跨方案 GPU 对比指标。

## 2.2 GPU-Driven Rendering Techniques

本节按三个维度组织对 GPU 驱动渲染技术的文献回顾:场景表示与数据管理、GPU 端可见性剔除、绘制提交与间接渲染。每个小节最后标注现有工作与本文贡献之间的空白。

### 2.2.1 Scene Representation & Data Management

GPU 驱动渲染需要在 GPU 端维护场景数据。在现代 API 中,着色器存储缓冲对象(SSBO)是实现这一目标的自然选择——它们提供着色器的读写访问,支持任意大小的数据,并可在不重新绑定描述符的情况下更新。

Aokana [Fang et al. 2025] 是近期一个将场景表示作为独立管线层处理的代表性工作。Fang 等提出了"全 GPU 驱动体素渲染框架",其中场景被划分为轴对齐的立方体块,每个块使用稀疏体素有向无环图(SVDAG)进行压缩,并附带独立的颜色数组。场景表示层(包括 LOD 层级和流式系统)被明确地独立于渲染管线层进行管理,渲染管线只是场景表示层的消费者。该框架在一台 RTX 3060 Ti 上以 Vulkan 运行,在 64K³ 体素分辨率下达到约 6ms 每帧。

Haar 与 Aaltonen [2015] 在 GPU 驱动渲染的开创性 SIGGRAPH 演示中,将"实例数据更新"作为 GPU 端剔除和绘制前的独立 CPU 前阶段。在此工作中,每个实例的变换矩阵和包围体数据在 CPU 端更新后上传,随后 GPU 端管线阶段(实例剔除、簇块扩展、簇剔除、索引压缩、多重间接绘制)全部操作于这些数据。

尽管这些工作识别出了场景表示作为管线中的一个独立关注点,但它们都**未**定量测量其对整体渲染成本的独立贡献。具体而言,在本文的三方案对照中,场景表示层的更新成本(SSBO 填充和上传)在三个方案之间是否一致,以及该成本在整体帧预算中的占比,是未被现有工作检验的问题。

### 2.2.2 Visibility Culling on GPU

GPU 端剔除是现代 GPU 驱动管线中研究最深入的阶段。Haar 与 Aaltonen [2015] 提供了权威的框架:实例剔除(视锥体+遮挡)处理粗粒度的每对象可见性,随后簇块剔除以更细的粒度(64 顶点簇)应用视锥体、遮挡和三角形背面检查。在 Xbox One 上以 1080p 分辨率测试 250,000 个独立移动物体时,GPU 剔除开销约为 0.37ms(实例剔除 0.28ms + 簇块剔除 0.09ms),总 GPU 帧时间为 2.3ms。

Unterguggenberger 等 [2021] 提供了剔除开销与剔除收益之间权衡的**唯一定量分析**。他们展示了在 0% 可剔除网格片的最坏情况下,后向面剔除代码在任务着色器中增加了 3.4% 的开销(帧时间从 7.80ms 增加到 8.07ms)。当 100% 网格片可剔除时,同样的代码获得了 11.4% 的加速(从 7.79ms 减少到 6.90ms)。收支平衡点大约在 20% 可剔除率处附近。该工作在 Vulkan 上使用 `VK_NV_mesh_shader` 实现,在 RTX 2060 和 RTX 3070 上测试了四个带骨骼动画的模型。

Li 等 [2023] 提出了一个 GPU-only 混合遮挡剔除管线,在 Vulkan 上使用一次间接多重绘制命令。他们的方法结合了迭代层次 Z 缓冲(IHZB)粗粒度计算剔除和光栅化细粒度剔除,无任何 CPU 回读。在 RTX 2060 上测试了一个包含 1,083,437 栋建筑(35,711,720 个三角形)的纽约市场景,相比于 HROC 实现了更高的剔除率和吞吐量。四个管线阶段被描述为:前向变形、并行提取(从 BVH 提取咬合体组)、混合剔除(IHZB + 细粒度)、渲染。

这些工作共同证明了 GPU 端剔除的工程可行性和性能优势。然而,就我们所知(to the best of our knowledge),它们都**未**提供"相同剔除逻辑在 CPU 上执行 vs 在 GPU 上执行"的公平定量对照。Li 等与 Unterguggenberger 等都在比较不同的 GPU 剔除算法,而非比较 GPU 执行与同等的 CPU 执行。本文通过方案 2 和方案 3 使用相同的视锥体测试逻辑(六个视锥体平面 vs AABB)和相同输入数据(相同的 ChunkInfo SSBO),只是将执行位置从 CPU 迁移到 GPU——从而直接填补了这一空白。

### 2.2.3 Draw Submission & Indirect Rendering

将绘制提交从 CPU 端逐一调用迁移到 GPU 端批量间接绘制,是 GPU 驱动管线的核心工程动机。Haar 与 Aaltonen [2015] 在 250,000 个物体的场景中展示了 CPU 侧单核绘制提交时间仅需 0.2ms,而之前迭代的《刺客信条》在同代硬件上大量时间消耗在 CPU 绘制调用上。间接绘制将绘制调用数减少了 1--2 个数量级,使其成为一个不随块数和网格类型变化的常数。

Galajda [2020] 提供了 Vulkan 中常规绘制调用 CPU 开销的全面定量基准,比较了跨三个硬件平台(搭载 GT 650M 的笔记本电脑、RX 580 台式机、RTX 2080 台式机)和三个 API(Vulkan-Hpp、OpenGL、他自己的 Tephra 抽象层)。在 1,000,000 个物体的绘制调用限制场景下,OpenGL 所需时间是 Vulkan 的 3.1--4.9 倍。Gonakhchyan [2018] 分析了 Vulkan 中三种命令缓冲区录制策略,展示了 300,000 个立方体场景下内联录制为 55.5ms 而每 1000 个对象辅助命令缓冲区为 0.03ms。**重要的是,Galajda 和 Gonakhchyan 的工作都不涉及间接绘制**——他们测量的是 CPU 端命令录制策略,而非 GPU 端命令生成的收益。

因此,现有工作分为两类:要么测量 API 端的常规绘制调用开销但不涉及间接绘制(Galajda、Gonakhchyan),要么展示单个 GPU 驱动技术的基准但不进行多方案分解(Li、Unterguggenberger、Haar & Aaltonen)。**没有一个工作在三方案对照实验中将间接绘制与同一场景同一数据布局下的两个 CPU 替代方案进行定量对比**。本文通过方案 1(直接绘制)与方案 2/3(间接绘制)的直接比较,在控制场景表示和剔除逻辑的前提下,隔离并量化绘制提交阶段的 GPU 化收益。

## 2.3 WFC for Procedural Scene Generation

Wave Function Collapse (WFC) 是一种基于约束的程序化生成算法,最初由 Gumin [2016] 引入。给定一组瓦片邻接规则,WFC 通过迭代选择最小熵的单元格并将其坍缩到一个满足所有局部约束的特定瓦片值,生成符合所有邻接规则的二维网格输出。

本文使用 6 种瓦片(空地、小立方体、大立方体、小球体、大球体、圆柱体),可配置权重控制场景密度和组成。WFC 配置由三个参数控制:网格边长(gridSize:16--4096)、确定性随机种子(seed:42、1337、9999)和密度(非空瓦片在权重中的比例:20%、50%、80%,通过 emptinessWeight 与 otherWeight 比率实现)。生成的瓦片网格通过 `TileGridToInstances` 转换为三维实例,该函数将每个非空瓦片映射到一个程序化网格(立方体或球体),并赋予位置、缩放和颜色属性。

WFC 尤其适合 GPU 驱动渲染评测,原因有三:**(1)** 瓦片网格结构自然地映射到渲染管线的空间块划分——同一块内的实例在空间上相邻,适合块级剔除;**(2)** 参数空间干净利落——网格大小、种子和密度是三个独立可复现的维度,与其他程序化方法(如依赖数十个参数的 L-system)形成对比;**(3)** 密度范围覆盖从稀疏(20% 非空)到极密(80% 非空),提供了跨越两个数量级的实例数量。尽管 WFC 瓦片网格在 GPU 内存中的最终产出与 Xylem [Sundararaman 2026] 使用的 L-system 树木场景具有可比性,但 WFC 模块化的、类似建筑的几何结构对 GPU 驱动管线的挑战不同——每个瓦片产生块内的少量实例(立方体或球体),使得实例级剔除变得不那么关键,但块级可见性主导了 GPU 工作负载。

重要的是,本文并不贡献 WFC 算法本身,也不声称 WFC 优于其他程序化方法。WFC 作为场景生成工具,为评测提供了一个可控且可复现的测试平台。

## 2.4 Existing Benchmarks & Gaps

### 2.4.1 GPU-Driven Rendering Benchmarks

现有的 GPU 渲染基准套件聚焦于微架构特征而非管线阶段分解。RayBench [Wang & Yu 2023a] 汇编了 100 个渲染程序的 160 个 GPU 微架构无关特征(指令级、线程级、内存级),并将其用于跨 P100、A100、T4 和 2080Ti 的机器学习驱动性能预测。RenderBench [Wang & Yu 2023b] 将渲染工作负载的 CPU 端微架构特征与 MiBench 和 NAS 并行基准套件进行比较,显示渲染程序在 256 宽度指令级并行度上平均为 5.70。尽管这些基准为低级 GPU 行为提供了洞察,但它们不将高级别管线阶段(如剔除 vs 绘制 vs 光栅化)作为特征进行区分。

在工业方面,Haar 与 Aaltonen [2015] 和《GPU Zen 3: Advanced Rendering Techniques》[Engel 2024] 第一部分"GPU-Driven Rendering"(收录六篇独立技术文章)提供了以生产者为中心的管线阶段框架和终点性能数据,但每篇文章仅报告一个或两个场景配置,没有提供跨场景规模和剔除比率的系统对照测量。

### 2.4.2 PCG Scene Rendering Evaluation

Xylem(本文完成时几乎同时出现)由 Sundararaman [2026] 完成,是目前最接近本文的工作。该文比较了三种渲染策略——传统 CPU 端渲染通道、计算驱动的间接渲染通道和网格着色器渲染通道——使用了跨三个场景(Orchard、Forest、Valley)的 L-system 生成的程序化树木在 fBM 置换地形上的场景。在密集的 Forest 场景(约 10⁵--10⁶ 树木实例)中,传统通道的 CPU 时间达到 201ms(而 GPU 时间为 60ms),而 GPU 驱动通道则将 CPU 时间降至约 11ms。一个与本文方案 2 退化行为类似的发现是:在结构化低实例场景(Orchard)上,计算通道实际上比传统通道更慢(20.75ms vs 16.07ms),这是因为 GPU 驱动的间接调度开销在没有足够剔除节省的情况下无法摊销。

尽管 Xylem 在概念上非常接近,但存在几个关键区别:**(1)** 该工作使用 Direct3D 12 配合 NVRHI 抽象层,而本文使用 Vulkan 1.3 进行更直接的 GPU 编程;**(2)** 场景使用 L-system 生成——这是一种不同属性的程序化方法——并以 1280×720 分辨率渲染;**(3)** "计算通道"在概念上融合了本文的方案 2 和方案 3(CPU 填充常量并调度计算着色器,但单个实例剔除仍在 GPU 上运行),使得剔除阶段难以隔离;**(4)** 无局部更新维度。最关键的是,该工作未包含纯 GPU 计算剔除方案(本文的方案 3),因为其第三个通道使用了具有不同几何处理管线的网格着色器,而非计算着色器。

最后,就我们所知,**WFC 生成场景与 GPU 驱动渲染评测的组合在已发表的文献中尚未出现**。在 arXiv、ACM 数字图书馆、Elsevier、IADIS 数字图书馆和多个学术搜索引擎上的多次搜索,均未发现任何将 WFC 生成的场景用于 GPU 驱动渲染评估的已发表论文。

### 2.4.3 Our Positioning

本文通过以下方式填补上述若干空白:

1. 在 Vulkan 1.3 下用统一 WFC 场景框架和共享 SSBO 数据布局,对照三个渲染方案,分解 GPU 驱动在场景表示、可见性剔除和绘制提交阶段的定量收益;
2. 提供方案 2(CPU 剔除 + 间接绘制)与方案 1(CPU 实例化)以及方案 2 与方案 3(GPU 计算剔除 + 间接绘制)之间的公平比较,隔离每个阶段的贡献;
3. 将场景数据管理的更新维度纳入评测,测量局部更新的成本并展示其在三个方案间的一致性。

---

# 3. System Design

本章描述实验框架的设计。核心原则是**公平性**:三种渲染方案共享相同的场景数据、相同的 SSBO 布局和相同的测量基础设施,它们之间的性能差异仅来自渲染策略的不同,而非数据组织或测量方式的差异。

## 3.1 WFC Scene Generator & Cache

场景生成管线由三个组件构成:

**WFCGenerator** 是 WFC 算法的核心实现。接受一个 `WFCConfig` 结构体——包含网格大小(`gridSize`)、随机种子(`seed`)和密度权重(`emptyWeight`/`otherWeight`)。密度到权重的映射为:20% 密度 → (emptyWeight=8, otherWeight=2);50% 密度 → (5, 5);80% 密度 → (2, 10)。生成器使用最小堆优化的约束传播算法,输出一个 `std::vector<uint32_t>` 类型的瓦片网格,其中每个元素编码一个 6 种瓦片类型(空地、小/大立方体、小/大球体、圆柱体)之一的 ID。

**TileGridToInstances** 将瓦片网格转换为三维实例数组。每个非空瓦片根据其类型被映射到一个 `WFCInstance` 结构体:立方体瓦片产生立方体网格实例,球体瓦片产生球体网格实例。每个实例包含位置(从网格坐标和 `cellSize` 推导)、缩放(从瓦片类型的 `scaleYMin`/`scaleYMax` 和 `scaleXZ` 随机采样)和颜色(RGB 值来自瓦片类型的预定义颜色)。

**按空间块分区**是场景生成的最后一步。每个实例根据其空间位置被分配到对应的块中:`cx = posX / (chunkSize × cellSize)`,`cy = posZ / (chunkSize × cellSize)`。每个块内的实例按 mesh 类型排序(cube 在前,sphere 在后),使得同一块内同一 mesh 的实例在 SSBO 中连续存储。每个块生成一个 `ChunkInfo` 结构体,记录其在网格中的位置(`gridX/gridY`)、在实例数组中的偏移(`instanceOffset`)和计数(`instanceCount`)、按 mesh 类型分别的计数(`cubeCount/sphereCount`)以及所有实例的轴对齐包围盒(`aabbMin/aabbMax`)。

**WFCCache** 确保场景的确定性和可复现性。同一组 `(gridSize, seed, density)` 参数生成的瓦片网格被序列化为二进制文件(`wfc_{grid}_{seed}_{density}.bin`)并缓存到磁盘。首次生成后,后续运行——包括所有三个方案和所有块大小的配置——直接从缓存中加载相同的瓦片网格,消除重复的 WFC 生成时间(对于 4096² 网格,单次 WFC 生成可能需要数分钟)。

> **公平性保证 1:** 三个方案运行于完全相同的 WFC 场景实例。任何性能差异仅来源于渲染策略,而非场景内容的差异。

## 3.2 Split-SSBO Scene Representation

GPU 端的场景数据组织采用了分离式着色器存储缓冲对象(Split-SSBO)架构,将实例的几何/剔除数据和外观数据分离为两个独立的设备本地缓冲区。

**CullingData SSBO**(每个实例 24 字节)包含 GPU 端剔除所需的信息:中心位置(`centerX/centerY/centerZ`,三个 float)和半尺寸(`halfX/halfY/halfZ`,三个 float)。这六个浮点值描述了一个与轴对齐的包围盒,用于视锥体剔除测试。该缓冲区在方案 3 中被计算着色器读取,在方案 1 和方案 2 中被 CPU 端剔除遍历使用(通过主机端映射的副本)。两个用途都使用相同的 `CullingDatum` 结构体布局。

**RenderData SSBO**(每个实例 16 字节)包含 RGBA 颜色值(`r/g/b/a`,四个 float)。这些数据仅被片段着色器消耗,不受剔除逻辑的影响。

**ChunkBuffer**(每个块 48 字节)将所有块的元数据以 `ChunkInfo` 结构体数组的形式存储在设备本地 SSBO 中。每个 `ChunkInfo` 包含块的网格坐标、实例偏移和计数、按 mesh 类型分别的计数以及 AABB。在方案 3 中,该缓冲区被计算着色器读取以执行 GPU 端剔除;在方案 2 中,其内容在 CPU 端遍历时被用作主机端副本。

**Indirect Buffer**(主机可见,方案 2 和方案 3)存储 GPU 间接绘制命令的参数。每个块需要两个间接绘制命令(一个用于 cube,一个用于 sphere),每个命令由 5 个 `uint32_t` 值组成(`indexCount`、`instanceCount`、`firstIndex`、`vertexOffset`、`firstInstance`)。因此总大小为 `N_chunks × 2 meshes × 5 × sizeof(uint32_t)`。该缓冲区被创建为**主机可见**,使方案 2 的 CPU 能够直接写入 `instanceCount` 字段,也使方案 3 能够在每帧后通过映射读回 `instanceCount` 值(用于填充 `m_chunkVisible`)。

数据传输路径对所有三个方案都是相同的:数据首先被组织到主机端向量(`m_cullingData`、`m_renderData`、m_chunks`)中,然后在初始化时通过 staging buffer 上传到设备本地 SSBO。上传使用 `vkCmdCopyBuffer`,将 staging 缓冲区(主机可见、主机一致)的内容复制到最终的设备本地缓冲区。该路径是**方案无关的**——所有三个方案使用相同的 `WriteInstanceData()` 函数和相同的 staging → copyBuffer → device-local 流程。数据仅在场景初始化或局部更新时上传;在正常渲染帧期间,SSBO 内容保持不变。

> **公平性保证 2:** 三个方案共享相同的 SSBO 布局、相同的数据内容和相同的数据上传路径。方案间的任何性能差异都不是由数据布局或传输差异造成的。

## 3.3 Three Rendering Schemes

三种渲染方案在 CPU/GPU 边界的位置、绘制调用的类型和剔除的执行位置上有所不同。图 3.1--3.3 展示了每种方案的架构。表 3.1 总结了它们差异的维度。

| 设计对比 | 隔离的管线阶段 |
|---|---|
| Scheme 1 vs Scheme 2 | Draw submission(直接绘制 vs 间接绘制) |
| Scheme 2 vs Scheme 3 | Visibility culling(CPU 端 vs GPU 端) |
| 局部更新(三个方案) | Scene representation(更新路径独立于渲染策略) |

**[图 3.1: Scheme 1 架构图——CPU 实例化]**
*(Mermaid 源码见 Figures/scheme1_architecture.mmd)*

**Scheme 1(CPU Instanced)** 是最简单的实现,作为基准方案。CPU 端遍历所有块:对于每个块,调用 `ExtractFrustumPlanes()` 获取六个视锥体平面,然后使用 Gribb-Hartmann 方法测试块的 AABB 与视锥体的相交关系。如果块可见,CPU 为 cube 网格发出一次 `vkCmdDrawIndexed` 调用,为 sphere 网格发出另一次调用。`m_drawCallCount` 等于可见块数 × 2,随场景块数线性增长。CPU 端的每帧遍历——包括视锥体测试和绘制调用录制——通过 `CpuSegBegin/CpuSegEnd` 分段计时,作为 `cpu_record` 的一部分捕获。

**[图 3.2: Scheme 2 架构图——CPU 剔除 + 间接绘制]**
*(Mermaid 源码见 Figures/scheme2_architecture.mmd)*

**Scheme 2(CPU Cull+Indirect)** 保留了 Scheme 1 的 CPU 端视锥体剔除遍历,但将绘制提交机制改为间接绘制。CPU 遍历所有块,执行视锥体测试,对于每个可见块,直接写入间接绘制缓冲区中对应块入口的 `instanceCount` 字段(该缓冲区为**主机可见**,因此 CPU 可以直接映射写入)。每个块每个 mesh 的其他所有间接绘制字段(`indexCount`、`firstIndex`、`vertexOffset`、`firstInstance`)在初始化时预填充,每帧保持不变——只有 `instanceCount`(可以是 0 表示被剔除)需要每帧更新。然后发出正好两次 `vkCmdDrawIndexedIndirect` 调用:一次用于 cube,一次用于 sphere,不论块的数量是多少。GPU 读取间接缓冲区的 `instanceCount` 值,并仅为该值指示的实例数量提交绘制工作。`m_drawCallCount` 恒为 2。

**[图 3.3: Scheme 3 架构图——GPU 计算剔除 + 间接绘制]**
*(Mermaid 源码见 Figures/scheme3_architecture.mmd)*

**Scheme 3(GPU Cull+Indirect)** 将剔除和绘制提交都迁移到 GPU。帧以填充间接缓冲区为零(`vkCmdFillBuffer`)开始,将所有 `instanceCount` 值清零。一个管线屏障将缓冲区从传输阶段过渡到计算着色器阶段。然后调度一个计算着色器:每个线程组处理一个块(`dispatch((N_chunks + 63) / 64, 1, 1)`)。计算着色器从 ChunkBuffer 读取块的 AABB,使用通过 push constants 传递的六个视锥体平面进行相交测试,如果块可见,则**原子性地**将间接缓冲区中的 `instanceCount` 值设置为每个 mesh 的实际实例数。计算调度后,第二个管线屏障将间接缓冲区从计算阶段过渡到间接绘制阶段。与 Scheme 2 相同,正好发出两次 `vkCmdDrawIndexedIndirect` 调用。`m_drawCallCount` 恒为 2。

**可见性读回**是方案 3 的额外步骤。由于 CPU 不执行剔除,它无法直接知道哪些块可见。在 `EndFrame()` 和 `SwapBuffers()` 之后——当 GPU 已完成间接绘制调用时——CPU 将间接缓冲区映射为 `uint32_t*`,并读取每个块的 `instanceCount` 字段。每个 mesh 入口中任何非零值表示该块被 GPU 判定为可见。该信息填充 `m_chunkVisible` 向量,用于每帧统计(录制阶段记录的 `visible_instances`)和交互式调试面板。

**设计意图。** 三种方案形成了一个受控的比较链。Scheme 1 → Scheme 2 从 CPU 端逐对象直接绘制切换到 GPU 端间接绘制,同时保持剔除在 CPU 端。这种隔离——相同的场景数据,相同的 CPU 端剔除逻辑,不同的绘制提交机制——使得间接绘制的 CPU 端加速被精确归因。Scheme 2 → Scheme 3 将剔除从 CPU 迁移到 GPU,同时保持间接绘制机制不变。这种隔离——相同的间接绘制调用,相同的 SSBO 数据,不同的剔除执行位置——将 CPU 端剔除的遍历成本与 GPU 端计算着色器的剔除开销区分开来。局部更新路径(3.4 节描述)在所有三个方案中完全相同,为场景表示层的更新成本提供了一个方案无关的基准。

## 3.4 Measurement Infrastructure

测量基础设施采用双轨设计:基于 CPU 的 QPC 计时测量主机端的执行情况,基于 GPU 的时间戳查询测量设备端的执行情况。

### 3.4.1 CPU Timing

CPU 计时使用 `QueryPerformanceCounter`,在 `Initialise()` 期间获取 QPC 频率并通过 `1e9 / frequency` 转换为纳秒精度(与 `EndMeasurement()` 使用的相同转换方式)。每帧经过四个阶段:

1. **BeginMeasurement** 记录 `m_frameStartQpc` 作为帧的起始时间戳。在命令缓冲区中写入一个 GPU 时间戳(`eTopOfPipe`),将 CPU 帧开始与 GPU 管线顶部对齐。

2. **CpuSegBegin / CpuSegEnd** 将 CPU 工作划分为独立的可测量段。`CpuSegBegin()` 记录段起始 QPC;`CpuSegEnd()` 记录结束时间并将差值累加到 `m_cpuRecordAccumUs` 中。多个段可以在单帧内串联(例如,方案 3 的 compute dispatch 录制段和 indirect draw 录制段),其计时会正确累加。所得的 `cpu_record` 指标代表所有管线段(剔除、命令录制、屏障)的 CPU 端总工作,但**不包括** fence 等待时间。

3. **MarkFenceWait** 测量 `BeginRenderToScreen()` 调用前后的时间差值,该调用包含了交换链 image 获取的 fence 等待。此持续时间单独计入 `m_cpuWaitUs`,并在每帧报告为 `cpu_wait`。

4. **EndMeasurement** 记录结束 QPC,从起始 QPC 减去得到 `frame_wall`。在命令缓冲区中写入第二个 GPU 时间戳(`eBottomOfPipe`)。当前帧的 `cpu_record`、`cpu_wait` 和 `draw_calls` 被捕获到一个 `FrameStats` 结构体中,并推入 `m_frameStats` 向量。

此流程的硬件开销为零:QPC 本身在 `Initialise()` 中仅被查询一次以获取频率,而段级 QPC 调用(`CpuSegBegin/CpuSegEnd`)是廉价的内联函数,仅读取 CPU 时间戳计数器。

### 3.4.2 GPU Timing

GPU 计时使用 `vkCmdWriteTimestamp2` 配合一个在 `CreateQueryPool()` 中创建的专用时间戳查询池,大小为 `2 × (recordFrames + 1)` 个查询。每帧写入两个时间戳:一个在管线顶部(`eTopOfPipe`),一个在底部(`eBottomOfPipe`)。两个时间戳之差即为该帧的 GPU 执行时间。使用 `eTopOfPipe` 和 `eBottomOfPipe` 而不是渲染通道特定的阶段,是出于必要:方案 1 和方案 2 在渲染通道内发出多个绘制调用,而方案 3 在渲染通道之外还有额外的计算调度。通过将时间戳锚定在管线的极端端点,GPU 执行测量在所有三个方案中具有可比性。

时间戳在所有录制帧的末尾通过 `vkGetQueryPoolResults` 进行批量读回——每个帧缓冲区索引读取 2 个 `uint64_t` 值——使用 `VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT` 标志,以确保 GPU 在 CPU 读取结果前已完成所有时间戳的写入。每个时间戳对通过 `(ts[2i+1] - ts[2i]) × m_gpuTimestampPeriod / 1000.0` 转换为微秒,其中 `m_gpuTimestampPeriod` 来自 `VkPhysicalDeviceProperties::limits::timestampPeriod`。

### 3.4.3 Metric Definitions

四个每帧指标定义如下:

- **`gpu_exec`**(µs):从时间戳查询推导出的 GPU 执行时间。这是表示 GPU 侧工作的最准确指标,且不受 CPU 调度或驱动程序开销的影响。对于方案 3,由于 GPU 在 CPU 发出绘制调用后异步执行,这尤为重要。

- **`cpu_record`**(µs):CPU 在帧内所有分段中花费的累计录制时间(剔除、变换、绘制调用录制、管线屏障)。不包括 fence 等待时间。在方案 1 中,此指标包括每块每 mesh CPU 端视锥体剔除和每块 `vkCmdDrawIndexed` 调用;在方案 2 中,包括每块 CPU 端 culling 和仅两次间接绘制调用;在方案 3 中,仅为设置 push constants、调度计算着色器和发出两次间接绘制调用的 CPU 端成本。

- **`cpu_wait`**(µs):在 `BeginRenderToScreen()` 中发生的交换链 image 获取 fence 等待。此指标反映了呈现管线的背压:当 GPU 完成前一帧的时间比 CPU 开始下一帧的时间更长时,`cpu_wait` 会相应地反映这种失配。

- **`frame_wall`**(µs):从 `BeginMeasurement` 到 `EndMeasurement` 的墙钟时间。对于方案 1 和方案 2,该指标近似 `cpu_record + cpu_wait`(差值 < 2\%)。对于方案 3,该指标明显小于端到端帧时间,因为 GPU 在 `EndMeasurement` 之外的 `EndFrame`/`SwapBuffers` 期间继续执行(**方案 3 中 `frame_wall` 不代表端到端帧延迟**——当比较方案间的墙钟性能时,应使用 `gpu_exec`)。

### 3.4.4 Local Update Standalone Measurement

局部更新成本在渲染帧循环之外以独立模式(`standalone`)进行测量。当向可执行文件传递 `-UpdateSize N` 标志时,程序跳过正常的 warmup+record 帧循环,改为运行一个专用测量序列:对 `RegenerateChunks(N)` 进行 10 次预热调用,然后进行 100 次计时调用,每次调用捕获 `GetLastUpdateUs()` 返回的 CPU 墙钟时间(由 QPC 在更新函数内的 staging 分配 + memcpy + copyBuffer + submit/wait 段周围测量)。per-chunk 和 batched 变体通过 `-updatebatched 0|1` 标志进行选择(默认 batched):两种模式执行完全相同的数据随机化(相同的 chunk 选择和相同的 scale/color 修改),只在提交策略上不同——per-chunk 为每个 chunk 使用一个独立的 `CmdBufferEndSubmitWait`,batched 将所有 chunk 拷贝合并进一个 command buffer 后进行单次 `CmdBufferEndSubmitWait`。

选择 standalone 模式而非将更新集成到录制帧循环中,原因有二。首先,standalone 测量将更新成本与渲染帧时间的噪声(呈现节流、驱动程序调度抖动、GPU 频率变化)隔离开来,以最小的标准偏差提供更精确的成本读数。其次,更新成本可以随后通过除以在实验设计中给出的相同 128² / chunk8 / density50 配置的稳态 `frame_wall` 值,转化为帧预算占用率——这种推导提供了"实际影响"的答案,而无需引入更新频率作为一个新的自变量(in-frame 触发必须在"每 N 帧更新一次"的参数下评估,会引入一个与方案选择无关的维度)。

### 3.4.5 Benchmark Automation

所有配置的编排由 `run_benchmarks.py` 处理,该脚本枚举完整的参数空间(9 个网格 × 3 个块 × 3 种密度 × 3 种方案 × 3 个种子),生成适当的命令行调用,并通过 `subprocess.run()` 执行每个配置。每个配置的超时时间由 `est_timeout(grid, chunk, update_size)` 计算,该函数根据块数(CPU 成本)和网格面积(GPU 成本,特别是在 seed9999 × dens80 等病态高负载配置下观测到的成本)这两个驱动因素进行缩放,并将上限设置在 5400 秒。完成的配置会生成一个 CSV 文件;断点续跑检查现有文件的存在性并跳过它们。失败(超时或非零退出码)记录为 `.fail` 文件并包含诊断信息,以便识别系统性的超时模式。所有成功完成的渲染和更新配置分别由 `aggregate()` 合并为 `_aggregate.csv` 和 `_aggregate_update.csv`。

---

# 4. Experimental Design

本章描述实验的独立变量、场景生成协议和测量协议。实验采用全因子设计(full factorial design),在所有独立变量组合上系统扫描,以刻画 GPU 驱动收益随场景规模、管理粒度和场景内容变化的完整特征曲线。

## 4.1 Independent Variables \& Parameter Space

实验包含两套独立的配置集合:稳态渲染评估和局部更新成本评估。

### 4.1.1 Steady-State Rendering Matrix

稳态渲染评估的独立变量有五个,形成全因子交叉:

| 变量 | 取值 | 计数 |
|---|---|---|
| Grid size ($N \times N$) | 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 | 9 |
| Chunk size (cells) | 4, 8, 16 | 3 |
| Density (non-empty ratio) | 20\%, 50\%, 80\% | 3 |
| Rendering scheme | 1 (CPU instanced), 2 (CPU cull+indirect), 3 (GPU cull+indirect) | 3 |
| WFC seed | 42, 1337, 9999 | 3 |

理论全组合数为 $9 \times 3 \times 3 \times 3 \times 3 = 729$。但由于块大小必须小于或等于网格大小的一半(chunk $\le$ grid/2),否则每个轴仅包含一个块,块级划分退化为整体渲染,失去评估意义,gridSize=16 且 chunkSize=16 的组合被排除。因此稳态渲染配置实际为 **702 个**。

每个配置在一组固定的 WFC tile 上运行——同一(gridSize, seed, density)组合在所有三个方案和三个块大小间共享相同的场景实例。WFC 场景仅生成一次(首次遇到该参数组合时),保存到磁盘缓存,后续配置直接从缓存加载。

五个独立变量覆盖了三个数量级的实例数量(从 16² × 20\% density ≈ 50 个实例到 4096² × 80\% density ≈ 15,000,000 个实例),使 GPU 负载从纯 CPU 瓶颈(小网格)跨越到纯 GPU 瓶颈(大网格),完整刻画了瓶颈转移曲线。

**与现有文献的参数空间对比。** Haar 与 Aaltonen [2015] 报告了单个场景配置(250,000 个物体,Xbox One,1080p)。Unterguggenberger 等 [2021] 使用两个场景配置(400K 和 849K meshlet 变体)在两块 GPU 上,未系统性地改变剔除率。Li 等 [2023] 在一个场景中以五种相机视角测试七种剔除方法(~35 个数据点)。Gonakhchyan [2018] 在三种命令缓冲区策略下测试了四种物体数量(~12 个数据点)。Xylem [Sundararaman 2026] 在三个场景中比较三种方案,D3D12,720p。就笔者检索范围而言,尚未见到将场景密度作为独立变量系统变化、使用多个随机种子测试场景内容敏感性、或包含局部更新维度的已发表工作。本文的 702 个渲染配置(加 108 个局部更新配置)覆盖五个独立维度的交叉组合,在参数空间的绝对覆盖数量上超过上述工作,但这一差异主要来自参数维度数和取值密度的选择,而非方法学上的突破——上述工作各自的场景规模、渲染管线复杂度或应用场景可能使更密集的参数扫描并非其研究目标所需。

### 4.1.2 Local Update Matrix

局部更新的独立变量,在固定的 128² / chunkSize=8 / density=50\% 场景上进行评估:

| 变量 | 取值 | 计数 |
|---|---|---|
| Update size (chunks affected) | 1, 2, 4, 8, 16, 32 | 6 |
| Submission mode | batched(单次提交), per-chunk(逐块提交) | 2 |
| Rendering scheme | 1, 2, 3 | 3 |
| WFC seed | 42, 1337, 9999 | 3 |

全组合: $6 \times 2 \times 3 \times 3 = 108$ 个配置。局部更新始终在隔离模式(standalone)下测量——脱离渲染帧循环——详细理由见 4.3.3 节。

## 4.2 Scene Generation Protocol

### 4.2.1 WFC Configuration

每种(density, seed, gridSize)组合的场景通过一次 WFC 生成产生。密度参数映射到 WFC 权重如下:20\% 密度 → (emptyWeight=8, otherWeight=2);50\% → (5, 5);80\% → (2, 10)。权重比通过影响坍缩阶段中非空瓦片的采样概率来控制输出的瓦片组成。种子保证每个(gridSize, density)组合的确定性可复现输出。

6 种瓦片类型的缩放和颜色属性在 WFC 配置中是固定的:立方体瓦片的 scaleY 在 1.0--11.0 之间随机采样(均匀分布),scaleXZ 固定为 1.0;球体瓦片的 scale 统一为 1.0--3.0。每种瓦片类型的颜色是预定义的 RGB 三元组,在生成时引入微小的随机扰动(±0.7 范围),使场景内的实例具有视觉多样性但仍然可辨识其瓦片类型。

### 4.2.2 Caching Strategy

缓存策略是实验在实践上可行的关键——WFC 生成对于大网格(2048² 和 4096²)可能需要几分钟的 CPU 时间,每次运行重新生成会使得 702 个配置的 overnight 运行变为 infeasible。每个唯一的(gridSize, seed, density)三元组生成一次,序列化为二进制缓存文件(`wfc_{grid}_{seed}_{density}.bin`)。缓存文件的大小等于 $\text{gridSize}^2 \times 4$ 字节(每个瓦片一个 uint32_t ID),对于 4096² 网格约为 67 MB。首次生成后,所有共享该(gridSize, seed, density)的配置从缓存加载相同的瓦片网格,仅在首次遇到时发生 WFC 生成。

由于块大小(chunkSize)不改变场景内容——只改变实例如何分组为块——同一个(gridSize, seed, density)缓存服务于该组合的所有三个块大小变体。三个渲染方案也共享相同的场景实例。因此缓存命中率非常高:在 810 个配置的完整运行中,为了覆盖 9 × 3 × 3 = 81 个唯一的(gridSize, seed, density)组合,最多进行 81 次 WFC 生成。

### 4.2.3 Camera Configuration

所有基准测试使用相同的静态 top-down(俯瞰)相机配置:相机放置在场景中心上方,高度 $\text{camY} = \text{sceneSize} \times 1.4$,俯仰角 pitch = $-89.9^\circ$,偏航角 yaw = $180^\circ$。这一相机位姿确保整个场景——跨越 $\text{sceneSize} \times \text{sceneSize}$ 区域的网格——完全包含在 45° 视场角的视锥体内。远裁剪面设置为 100000,以防止远距离实例被裁剪。

选择 top-down 全可见视角是出于实验设计上的考虑,而非对实际应用场景的模拟。在全可见条件下,**每个块要么完全可见(在视锥体内),要么被其自身 AABB 剔除(当 AABB 足够大以至于深度测试失败时)**——这种简化使得剔除效率的分析更可控。如果在部分遮挡的相机路径下评估,可见性比会随相机位置和时间剧烈变化,增加一个与方案选择无关的混乱变量。全可见条件代表**最有利于 CPU 剔除的场景**(因为大多数块是通过的,CPU 剔除没有节省绘制调用),因此 Scheme 2 在任何测试条件下都不会比这更差——如果 CPU 剔除在此条件下退化,那么该退化是该方法的一个保守下界。

### 4.2.4 Frame Timing Control

基准模式(`-Benchmark` 标志)下的帧循环传递固定的 $\Delta t = 1/60$ 秒,与实际的墙钟时间无关。这使得所有配置的每帧逻辑更新保持一致,消除了交互模式下帧时间抖动对相机运动(在基准模式中已禁用)和 CPU 调度的影响。

## 4.3 Measurement Protocol

### 4.3.1 Warmup and Recording

每个稳态渲染配置采集 **120 个预热帧 + 1200 个录制帧**。预热阶段消除冷启动效应:GPU 频率提升至稳态工作点,着色器和管线对象已编译并缓存,驱动内部数据结构已达到稳态大小。录制阶段提供每指标 1200 个样本,足以支撑百分位数统计——一个优于大多数现有文献的选择。Unterguggenberger 等 [2021] 使用 100 个预热帧和 1000 个录制帧,仅报告平均值;Li 等 [2023] 和 Gonakhchyan [2018] 完全没有指定预热帧数。本文每指标报告六个统计量:avg、min、max、P1、P99、stddev,使得读者能够评估帧间方差和尾部行为。

预热帧期间不进行任何测量。录制阶段开始时(`m_isRecording` 在一个 `BeginMeasurement` 调用中设置为 true,当 `m_currentFrame >= warmupFrames` 时触发),后续 1200 个帧中的每一个帧都贡献一个包含 `cpu_record`、`cpu_wait`、`gpu_exec`、`frame_wall`、`draw_calls` 和 `visible_chunks` 的 `FrameStats` 结构体。

### 4.3.2 Timing Infrastructure

测量基础设施在第 3.4 节中已详述,此处仅总结关键参数:

- **CPU 计时:** QPC 频率在 `Initialise()` 期间通过 `QueryPerformanceFrequency` 获取。每帧 `cpu_record` 通过 `CpuSegBegin/CpuSegEnd` 对累积,`cpu_wait` 通过 `MarkFenceWait` 单独捕获。转换因子: $\text{ticks} \times (1\times 10^9 / \text{QPC frequency}) / 1000$ → 微秒。
- **GPU 计时:** 时间戳查询池大小为 $2 \times (\text{recordFrames} + 1)$。每帧通过 `vkCmdWriteTimestamp2` 在 `eTopOfPipe` 和 `eBottomOfPipe` 写入两个时间戳。时间戳在所有录制帧结束时通过 `vkGetQueryPoolResults` 进行批量读回(`VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT`)。GPU 时间戳周期来自 `VkPhysicalDeviceProperties::limits::timestampPeriod`。
- **聚合:** 每个配置的 1200 个每帧样本通过 `computeStats()` 汇总为六个统计量,计算方式为标准排序百分位数:P1 = `sorted[max(1, n/100) - 1]`,P99 = `sorted[min(n, n*99/100) - 1]`(对 n=1200 即索引 11 和 1187)。每个配置写出一个 CSV 文件(每帧一行 + 末尾统计摘要行),所有配置的摘要行合并为 `_aggregate.csv`。

### 4.3.3 Local Update Standalone Measurement

局部更新成本在隔离的 standalone 测量序列中采集(脱离渲染帧循环),对 per-chunk 和 batched 两种提交策略分别计时。该测量模式的实现机制、standalone 而非 in-frame 触发的两点理由(精度隔离与免于引入更新频率维度),以及 per-chunk/batched 的提交差异,已在 §3.4.4 中详述,此处不再重复。本节仅说明其在实验设计中的角色:局部更新构成一个独立于三种渲染方案的配置子集(见 §4.1.2),用于隔离 scene representation 层的更新成本。

### 4.3.4 Hardware and Software Configuration

所有实验在以下配置的单一机器上运行:

| 组件 | 规格 |
|---|---|
| GPU | NVIDIA GeForce RTX 4080 SUPER (16 GB GDDR6X) |
| CPU | AMD Ryzen 9 9900X |
| 系统内存 | 64 GB DDR5 |
| 操作系统 | Windows 11 Pro (Build 10.0.26200) |
| 图形 API | Vulkan 1.3 |
| 呈现模式 | VK\_PRESENT\_MODE\_MAILBOX\_KHR(无垂直同步,允许 GPU 以无上限帧率运行) |
| 基准窗口 | 256 × 256 像素 |
| 编译器 | Visual Studio 2022, Release 配置(/O2,LTCG) |

基准窗口分辨率为 256×256 而非典型桌面分辨率(如 1920×1080),这是因为实验的目的是测量**场景管理**开销(culling、draw submission、buffer update)而非像素着色器吞吐量。较小的分辨率减少了片段着色器工作在总 GPU 执行时间中的占比,使实验对块数量、剔除效率和绘制提交成本的敏感度更高。此外,所有实验使用 top-down 相机实现全场景可见,这意味着可见实例的数量和 GPU 负载主要由场景规模决定,而非视锥体裁剪——该选择在第 4.2.3 节中已给出理由。

GPU 型号在运行时通过 `VkPhysicalDeviceProperties::deviceName` 自动捕获,并嵌入每个 CSV 文件的元数据头中。CPU 和系统内存规格为手动记录(未通过编程方式捕获)。所有基准测试运行在**无验证层**的情况下,因为在录制数千帧时验证层的 CPU 开销会显著污染 `cpu_record` 指标。

**关于构建配置的说明。** 全部 810 个配置最初在 Debug 构建下完成过一轮测量。由于 Debug 构建关闭内联优化并启用迭代器调试和运行时检查,`cpu_record` 的绝对值被系统性放大,且放大倍数因方案的代码路径结构不同而不均匀(如 §5.1.3 所述)。为排除这一因素,本文以 Release 构建对全部 810 个配置重新采集数据,正文和第五章报告的全部数值均来自 Release 构建;Debug 构建的原始数据已归档,仅在 §5.1.3 中用于说明构建配置对方案间相对排序的影响。`gpu_exec` 一项例外——由于该指标完全由 GPU 时间戳测得,不经过 CPU 编译路径,Debug 与 Release 构建下的差异在两轮测量中均小于 2%。

---

# 5. Results \& Analysis

本章呈现实验测量的定量结果,按四个维度组织:稳态渲染成本(5.1),CPU/GPU 瓶颈交叉(5.2),场景内容敏感性(5.3),和局部更新成本(5.4)。除非特别说明,所有结果均来自每个配置 1200 帧录制的汇总统计。

## 5.1 Steady-State Rendering Cost by Scheme

### 5.1.1 GPU Execution Time

表 5.1 展示了三个方案在固定参数(chunk=16, density=50\%, seed=42)下的 `gpu_exec` 随 grid size 的变化。`gpu_exec` 从 grid=64 时的约 14us 增长到 grid=4096 时的约 45646us,跨越三个数量级。

**表 5.1:** GPU 执行时间(µs)随网格规模变化(chunk=16, density=50%, seed=42)

| Grid | Scheme 1 | Scheme 2 | Scheme 3 | S3/S1 |
|---|---|---|---|---|
| 64 | 14.1 | 18.3 | 20.8 | +48.0% |
| 256 | 142.8 | 171.4 | 173.8 | +21.7% |
| 512 | 551.4 | 666.0 | 669.2 | +21.4% |
| 1024 | 2459.9 | 2860.9 | 2863.2 | +16.4% |
| 2048 | 9125.8 | 11219.1 | 11095.3 | +21.6% |
| 4096 | 35486.7 | 41403.2 | 45645.8 | +28.6% |

三个方案的 GPU 执行时间在所有 grid 下都较为接近。S3 的 `gpu_exec` 一致性地高于 S1,超出幅度约为 +16\% 到 +48\%。这是 GPU compute culling 的固定开销——compute shader dispatch、atomic indirect buffer 写入和两次管线屏障。在 grid=64 时这一开销占 S1 gpu\_exec 的 48\%(+6.7us);在 grid=4096 时增长到 +29\%(+10159us),但在大 grid 下片段着色成为 GPU 工作的主导,S3 的相对额外成本对整体帧时间的影响有所稀释。

S1 和 S2 的 `gpu_exec` 差异在 16\% 到 31\% 之间(如 grid=64 时 +30.5\%,grid=1024 时 +16.1\%),S2 略高于 S1。这一差异不随 grid 单调变化,幅度也小于 S1--S3 在 `cpu_record` 上的数量级差距,提示两者的 GPU 端工作量本身接近,`gpu_exec` 的波动更多来自帧间噪声或呈现管线的间接影响,而非绘制机制本身的系统性区别——绘制机制(直接 vs 间接)改变的是 CPU 端命令准备的方式,不改变 GPU 需要栅格化的几何体总量。

### 5.1.2 CPU Recording Cost

表 5.2 展示了 `cpu_record` 随 chunk 数的变化(grid=1024, density=50\%, seed=42)。

**表 5.2:** CPU 录制成本(µs)随块大小变化(grid=1024, density=50%, seed=42)

| Chunk | Chunks² | Scheme 1 | Scheme 2 | Scheme 3 | S1/S3 |
|---|---|---|---|---|---|
| 4 | 65536 | 2601.4 | 496.3 | 15.7 | 166× |
| 8 | 16384 | 801.8 | 140.9 | 13.5 | 60× |
| 16 | 4096 | 263.4 | 40.7 | 12.8 | 21× |

S3 的 `cpu_record` 在所有配置下基本保持恒定(约 7--45us,且不随 chunk 数量呈量级增长,上端的 45us 出现在 grid=4096/chunk=4 这一 compute dispatch 线程组数最多的极端配置),因为 S3 的 CPU 工作仅为设置 push constants、发出一次 compute dispatch 和两次 indirect draw 调用——这些都是常数时间操作。S1 和 S2 的 `cpu_record` 随 chunk 数量增长,因为两者都必须逐 chunk 遍历所有块;但增长速度不同,S2 比 S1 慢得多,因为间接绘制把每帧的绘制调用数固定为 2 次,只剩 CPU 端 frustum 测试随 chunk 数增长。

在 grid=1024 下:chunk=4(65536 blocks)时 S1=2601us, S2=496us, S3=15.7us——S1 与 S3 差距 **166×**,S1 与 S2 差距 **5.2×**;chunk=16(4096 blocks)时 S1=263us, S2=41us, S3=12.8us——S1 与 S3 差距 **21×**。在 grid=4096/chunk=4(1,048,576 blocks)下差距被推向本文测得的最大值:S1=44541us 对 S3=44.7us,**995× 差距**。

### 5.1.3 间接绘制机制对 CPU 录制成本的净收益(Release 构建)

本节报告 Release 构建下 S1→S2 的 `cpu_record` 净收益;构建配置对本节结论的影响(以及据此重新采集全部数据的说明)已在 §4.3.4 中给出。在参考配置 grid=1024/chunk=4/density=50\%/seed=42 下,S1=2601us、S2=496us,**S2 比 S1 快 5.2×**;这一优势在 grid≥256 的全部测试配置下一致成立,且随网格增大而扩大(在 seed42/dens50 参考配置下,chunk4 时 4.9--6.0×,chunk16 时随 grid 从 6.5× 增至 7.3×)。就全部测试配置而言,S1/S2 的 `cpu_record` 比值最高达 9.9×;在 S2 稳定胜出的 grid≥256 范围内为 1.7--9.9×,按密度中位数约 5--8×(低密度场景偏高:dens20 约 8×、dens50 约 5.5×、dens80 约 5.1×)。在更小的场景规模下(grid≤64),间接绘制机制自身的固定开销(缓冲区写入、间接命令读取)超过了它节省的绘制调用成本,使 S1 在 grid=16 的全部密度和种子组合下反而快 0.1--1.2us(比值 <1);这一差距随网格增大而收窄,在 grid=64 时已反转为 S2 领先 5.8--12.2us。

**图 5.1:** S1/S2/S3 的 `cpu_record` 对比(grid=1024, chunk=4, density=50%, seed=42)

![](Figures/fig5_1_cpu_record_bar.png)

此处补充说明为何必须以 Release 构建为准(§4.3.4 已交代该修订的整体背景)。Debug 构建关闭内联优化并启用迭代器调试和运行时检查,这会系统性放大 CPU 端代码的执行时间,但放大倍数并非跨方案均匀——它取决于具体代码路径的结构(逐块的绘制调用录制、还是逐块的浮点视锥体测试与内存写入)。对同一配置(grid=1024/chunk16/dens50/seed42)比较 Debug 与 Release 的 `cpu_record`:S1 从 1285us 降至 263us(4.9×),而这一放大倍数在 S1 和 S2 之间并不相等,足以使两者的相对排序反转。因此,若以 Debug 数据为准会得出"CPU 剔除退化为纯损耗"的结论,而该结论只是不均匀优化倍数造成的测量假象,并非 CPU 端视锥体剔除本身的固有属性。

在 Release 构建下,S1→S2 的收益单纯来自绘制机制——CPU 端仍对全部 65536 个 chunk 执行 frustum 测试(该测试在全可见相机条件下确实不产生任何剔除收益,§4.2.3 所述的相机设计仍然成立),但将绘制调用从逐 chunk 的 `vkCmdDrawIndexed`(71133 次)改为恒定 2 次的 `vkCmdDrawIndexedIndirect`,足以抵消 frustum 测试本身的成本并带来净收益。这表明:在本文的测量条件下,间接绘制机制本身的 CPU 端收益超过了"无效剔除"的额外开销,§6.2 将进一步讨论这一点对 Unterguggenberger 等 [2021] 的类比是否仍然成立。

## 5.2 CPU/GPU Bottleneck Crossover

### 5.2.1 Bottleneck Identification

有效帧瓶颈是每方案 `max(cpu_record, gpu_exec)`。在 chunk=16 的所有 grid 下,三个方案均为 GPU-bound——CPU 录制成本始终小于 GPU 执行时间。当 chunk 缩小到 4 时,S1 的瓶颈发生转移,但 S2/S3 并未跟随。在 grid=4096/chunk=4(1,048,576 个块)下:

- S1: `cpu_record`=44541us, `gpu_exec`=38724us → **CPU-bound**(CPU 是 GPU 的 1.15 倍)
- S2: `cpu_record`=7423us, `gpu_exec`=44730us → **GPU-bound**(GPU 是 CPU 的 6.0 倍)
- S3: `cpu_record`=45us, `gpu_exec`=236173us → **强烈 GPU-bound**(GPU 是 CPU 的 5278 倍)

只有 S1 在极端小 chunk 下转为 CPU-bound;S2 凭借间接绘制将绘制提交固定为常数成本,即使仍在 CPU 端执行 frustum 测试,也足以避免 CPU 成为瓶颈。S3 的 `gpu_exec` 在 chunk4 时(236173us)相比 chunk16 时(45646us)大幅上升——这是 1,048,576 次 compute shader 线程组调度的固有开销,提示 GPU 端剔除并非在任何 chunk 粒度下都是免费的:当块数极端细粒度化时,compute dispatch 本身的调度开销可以超过它所替代的 CPU 端遍历成本的量级增益。

### 5.2.2 Draw Submission Cost Decomposition

在 grid=4096/chunk=4 下,S1 发出约 114 万个 `vkCmdDrawIndexed` 调用(实测 `draw_calls`=1,137,383;理论上限为 2×1,048,576,但空块和仅含单一网格家族的块使实际调用数低于上限,约合每块 1.08 次)(cpu\_record=44541us),S2 仅发出 2 个 indirect draw 调用(cpu\_record=7423us)——绘制调用数减少约 57 万倍,对应 CPU 成本下降 6.0×。这与 §5.1.3 报告的反转一致:间接绘制本身的收益足以让 S2 快于 S1,即使 S2 仍需在 CPU 端遍历全部 chunk 做 frustum 测试。

S3 在此基础上进一步移除了 CPU 端的逐 chunk 遍历:S3 的 `cpu_record`=45us——比 S1 快 **995×**,比 S2 快 **165×**。但这一 CPU 端收益的代价体现在 GPU 端:如 §5.2.1 所述,S3 的 `gpu_exec` 在同一配置下达到 236173us,远高于 S1(38724us)和 S2(44730us)——compute dispatch 的调度开销在 100 万级别的 chunk 数下变得显著,S3 把瓶颈从 CPU 移到了 GPU,但没有消除瓶颈本身。

## 5.3 Scene Content Sensitivity

### 5.3.1 The seed9999--dens80 Outlier

在 density=20\% 和 50\% 下,三个种子的 `gpu_exec` 几乎相同(差异 < 1\%):约 110,600us(20\%)和 45,550us(50\%)。但在 density=80\% 下,seed=42 和 seed=1337 分别为 52,879us 和 52,911us,而 seed=9999 达到 **600,999us**——是其他两个种子均值的 **11.4 倍**。这一现象在三个渲染方案下一致出现(S1 为 12.29×,S2 为 12.10×,S3 为 11.36×),排除了它是特定方案的测量伪影。

**图 5.2:** `gpu_exec` 按种子 × 密度拆分(grid=4096, chunk=16, scheme=3)

![](Figures/fig5_2_content_sensitivity_bar.png)

三个种子的可见 chunk 数完全相同(65536,对应 256×256 的 chunk 网格),绘制调用数也完全相同(2,S3 为间接绘制)。为定位差异的真实来源,本文直接解析了 4096²/density=80\% 下三个种子对应的 WFC 缓存二进制文件,统计各瓦片类型的精确数量:

| 种子 | 立方体家族(LOW+MID+HIGH) | 球体家族(SPHERE\_S+SPHERE\_L) | 球体占比 |
|---|---|---|---|
| 42 | 15,235,807 | 500 | 0.003\% |
| 1337 | 15,236,138 | 510 | 0.003\% |
| 9999 | 2,363 | 14,703,079 | 99.98\% |

差异不是"更多大型 tile",而是网格构成在立方体家族和球体家族之间的近乎完全翻转。原因可追溯到 WFC 邻接规则的结构:立方体家族(LOW/MID/HIGH)与球体家族(SPHERE\_S/SPHERE\_L)之间没有直接的邻接兼容性,两个家族只能通过 EMPTY 瓦片连接;而 density=80\% 对应的权重配置(emptyWeight=2, otherWeight=10)使 EMPTY 瓦片的采样概率很低。当 EMPTY 稀缺时,坍缩早期哪个家族的局部连通区域先扩张,就会通过约束传播扫过几乎整个网格——这一过程从第一个坍缩的格点(算法固定从网格原点开始,而非随机遍历顺序)就已经决定,因此对随机种子高度敏感,但与网格规模无关(这解释了该现象在 grid=16 到 grid=4096 的全部测试规模下表现一致)。对生成器代码的独立复核和覆盖 95 个未在正式实验中使用的种子的补充测试证实:在 density=80\% 下,家族级锁定是**普遍**结果而非罕见异常——全部测试种子无一例外锁定到某一家族(无中间/混合结果),其中约 60\% 锁定为立方体家族、40\% 锁定为球体家族,这一 60/40 的偏向可追溯到瓦片目录中立方体家族有 3 种细分类型而球体家族只有 2 种,在权重按瓦片种类而非按家族均分的情况下构成了 3:2 的结构性倾向。种子 9999 落入球体家族并非小概率的极端个例,而是这一机制下常见的两种结果之一。

球体网格的三角形数(768)是立方体网格的三角形数(12)的 **64 倍**。为验证三角形数是否比"种子身份"本身更能解释 `gpu_exec`,本文将比较范围从单一密度(80\%)扩展到全部 9 个种子 × 密度组合(20\%/50\%/80\% × 42/1337/9999),对每个组合直接解析缓存文件、按实际瓦片混合计算总三角形数,与对应的 `gpu_exec` 一并绘制:

**图 5.3:** `gpu_exec` 相对总三角形数的散点图(grid=4096, chunk=16, scheme=3, 全部 9 个种子×密度组合)

![](Figures/fig5_3_triangle_count_scatter.png)

这 9 个组合虽提示 `gpu_exec` 随三角形数增长,但它们在三角形轴上仅聚成两簇:density=20\%/50\% 的 8 个点落在低三角形区,而 seed9999/dens80 的 1 个点孤立于高三角形区。在这种两簇分布上,即便单变量线性回归也能达到 R²=0.998,但这更多是两簇结构的产物,而非对线性关系的严格检验;若再加入总实例数做双变量回归,9 个数据点上使用 3 个参数虽可将 R² 推到接近 1,却存在明显的过拟合风险。更根本的是,总实例数本身随密度单调变化(density=20\% 约 656 万、density=80\% 约 1524 万),与三角形数混淆,无法在这组数据上区分二者各自的贡献。要严格检验"三角形数直接决定 `gpu_exec`"这一命题,需要一条在三角形轴上连续、等间距、且与实例数解耦的受控曲线。

为此,本文设计了一组受控扫描:固定 grid=1024、chunk=16,通过提高 emptyWeight(从 density=50\% 对应的 5 提高到 20)打破立方体/球体家族锁定(机制见本节前文),使场景构成可连续调节且**与随机种子无关**——11 个采样点的总三角形数在三个种子间的离散度均 ≤2.8\%,种子作为混淆变量被移除。调节 cube:sphere 权重比(两者之和固定为 20),使总三角形数在 8.47M→487M(58×)区间内**等间距分布为 11 个十分位点**(0\%, 10\%, …, 100\%),每点在 3 个种子 × 3 个渲染方案下各测一次,共 99 次运行。

**图 5.3b:** `gpu_exec` 相对总三角形数的受控扫描(grid=1024, chunk=16, emptyWeight=20, 11 个十分位点 × 3 种子 × 3 方案)

![](Figures/fig5_3b_percolation_triangle_sweep.png)

结果(图 5.3b)显示 `gpu_exec` 对三角形数呈高度线性。以方案 3 为例,单变量拟合为 `gpu_exec ≈ 58102 × triangles(十亿) + 1963`(单位 µs);**11 个点的残差全部落在 ±1.5\% 以内,三个种子间的离散度仅 1.09\%**(R²>0.999)。需说明的是,本次扫描的 11 个采样点是按立方体∶球体比例等间距设计的,在这种大动态范围(58×)的等距设计下,任何单调关系都会得到接近 1 的 R²,因此此处的线性证据主要由残差幅度(±1.5\%)而非 R² 数值承担。关键在于:本次扫描中总实例数并非单调变化,而是随构成呈 U 形(pt0 约 70.6 万 → pt4 约 52.6 万 → pt10 约 63.4 万,总幅度 25.4\%)。若实例数是 `gpu_exec` 的主要驱动来源,曲线应在中段出现相应的 U 形偏离——但实测曲线保持严格线性,表明在本文测试规模下,三角形栅格化开销主导,每实例的固定开销(变换、SSBO 读取)贡献落在噪声内、不可分辨。两个间接方案的每三角形斜率几乎相同(S2=58464, S3=58102 µs/十亿,相差 <1\%),方案 1 略高(60072,约高 3\%,原因见下段),三者整体一致,证实该几何负载成本与绘制提交方式和剔除位置基本无关——这与图 5.2 中"该现象在三个渲染方案下一致出现"的结论互相印证。

值得注意的是,方案 1 的拟合质量略低于两个间接方案(R²=0.9986 对 ≥0.99994)。这可由一个间接方案不存在的协变量解释:方案 1 的 draw_calls 数量随场景构成变化——单一家族场景(纯立方体或纯球体)每 chunk 只需 1 组实例化绘制,共 4096 次;而混合场景两个家族并存,每 chunk 需 2 组,升至约 8190 次。这一 draw_calls 数随构成呈驼峰形(两端纯家族低、中段混合高),并不随三角形数单调,因而给方案 1 的 `gpu_exec` 引入了一项与几何负载无关的绘制开销,在三角形数最少的纯立方体端最为显著(残差 +34\%,该处绘制开销占总成本比例最大)。方案 2 和方案 3 无论场景内容如何,draw_calls 恒为 2,从而消除了这一协变量,这也是二者的 `gpu_exec` 是三角形数更纯粹线性函数的原因。这构成了间接绘制优势的一个补充侧面:除 §5.2 所述的 CPU 端加速外,间接绘制还使 GPU 执行时间成为几何负载更干净的函数——不过需注意这一差异主要在低三角形负载下显著,高负载下栅格化开销占主导,三个方案趋于一致。

综上,在本受控扫描覆盖的三角形数区间内(grid=1024,总三角形数约 8.5M–487M),`gpu_exec` 与场景总三角形数呈高度线性关系,且与实例数的非单调变化无明显关联;种子和密度只是通过决定 WFC 坍缩结果间接影响三角形数这一几何负载指标的中间变量,所谓"种子敏感性"的本质是"坍缩结果对几何负载的敏感性",在本文的邻接规则下,这种敏感性在 density=80\% 时因家族级锁定而被放大到数量级级别。

关于这一结论需说明两点作用边界。其一,本扫描仅通过改变立方体/球体两种网格类型的混合比例来变化三角形数,并未单独控制每种网格类型的逐三角形着色成本(球体与立方体在顶点拓扑、缓存访问模式上存在差异);严格地说,回归刻画的是"几何负载(以三角形数度量)与 `gpu_exec` 的强相关",不能完全排除网格类型特有的渲染开销对回归斜率的贡献——将三角形数作为主导驱动因子,是在本文两类网格构成下、排除了实例数这一竞争解释后得到的最简解释(见 §7.2)。其二,§5.3.1 开头 seed9999/dens80 的异常出现在 grid=4096(总三角形数约 113 亿,超出本扫描上限约 23×),其量级与构成翻转后的三角形数差异(64× 三角形比经家族占比与实例数差异调制后)一致,可视为上述线性关系的外推;但本文未在该规模上单独验证线性关系是否严格成立,且 §5.3.2 显示每 chunk 的 GPU 成本本身随 grid 规模变化,故对 grid=4096 异常的解释应理解为量级一致的外推,而非在该规模上的严格验证。

这一比值随 grid size 单调递减:grid=256 时 18.0×,grid=512 时 17.4×,grid=1024 时 13.3×,grid=2048 时 11.5×,grid=4096 时 11.4×。这一趋势值得注意——它意味着该现象的相对幅度依赖于测试的网格规模,本文报告的 11.4× 只是 4096² 网格下的观测值,不应外推为该种子在任意规模下的固定倍数。尽管家族级锁定机制本身已经确认具有普遍性(见上文),但本文的正式实验矩阵仍只覆盖 3 个种子,量化"密度 × 种子"交互效应对 GPU 负载的完整分布仍需要更大范围的种子扫描(见 §7.3)。

### 5.3.2 per-Chunk GPU Cost Stability

在正常场景(排除 dens80/seed9999)下,每 chunk 的 GPU 成本随 grid 增大而下降:seed=42/dens20/ch16/scheme1 下,grid=64→2268 ns/chunk,grid=1024→1786 ns/chunk,grid=4096→1339 ns/chunk。这是 GPU 的**幅度经济**效应——更大的 grid 意味着更充分地利用 GPU 计算单元和内存子系统。

seed9999 在 density=20\% 下也遵循同一规律(1944→1792→1334 ns/chunk),证实 seed9999 本身不是"病态种子"——它仅在 density=80\% 时表现出异常。这表明性能方差是 density 和 seed 的**交互效应**,而非 seed 单独引起的。

## 5.4 Local Update Cost

### 5.4.1 Batched vs Per-Chunk Submission

表 5.3 展示了两种提交模式下更新成本随 update size 的变化(seed=42 均值)。

**表 5.3:** 局部更新成本(µs),batched vs per-chunk 提交(seed=42,三个方案均值)

| Update Size | Batched (avg) | Per-Chunk (avg) | Speedup |
|---|---|---|---|
| 1 | 55.3 | 61.4 | 1.1× |
| 2 | 59.1 | 119.5 | 2.0× |
| 4 | 61.4 | 241.2 | 3.9× |
| 8 | 67.5 | 466.8 | 6.9× |
| 16 | 69.6 | 941.7 | 13.5× |
| 32 | 86.5 | 1896.5 | 21.9× |

Per-chunk 模式的更新成本随 update size 近似线性增长:1 chunk=61us, 16 chunks=942us, 32 chunks=1897us(约 59us/chunk)。这一增长由每次同步提交的 fence 等待主导。Batched 模式随 update size 增长要缓慢得多:1 chunk=55us, 16 chunks=70us, 32 chunks=87us。32 chunks 处 batched 模式加速 **21.9×**(87us vs 1897us)。

两条曲线随 update size 持续分叉——"剪刀"形态。对于单 chunk 更新,两种模式接近(都仅有一次 submit/wait);但随着 update size 增大,per-chunk 的 N 次独立 fence 循环导致成本近似线性增长,而 batched 增长幅度小得多。

### 5.4.2 Scheme-Independence Confirmation

Batched 更新在三个方案间的成本最大差异(三种子均值)为 5.3\%(size=1)、4.4\%(size=16) 和 9.7\%(size=32),跨全部 update size 的最大值为 11.5\%(size=2)。这一差异高于最初设计预期,但仍显著小于 per-chunk 模式随 update size 的增长幅度,说明**提交策略(batched vs per-chunk)对更新成本的影响远大于渲染方案的选择**。这一结果的解释力受限于一个已知的实现事实(见 §3.2):三个方案在本文的实现中共享同一条更新代码路径(`RegenerateChunks`),因此"方案间差异较小"在很大程度上是实现结构的必然产物,而非跨越独立实现路径验证出的普适规律。

就研究问题而言,这一结果提示:在本文的实现约束下,scene representation 的更新成本主要由**提交策略**而非**渲染方案**决定。若要验证这一结论是否可推广到方案间使用独立更新路径的实现,需要进一步的对照实验。

### 5.4.3 Impact on Frame Budget

在 128^2/chunk=8/density=50\% 场景下,实测稳态 `frame_wall` 为 S1=21.5us,S3=14.0us。Batched 16-chunk 更新平均成本随方案略有差异(S1=70.2us,S2=71.9us,S3=66.8us);为便于横向比较,下文各方案分别采用其自身的更新成本换算帧预算占用。

在 S3 下,一次更新消耗的 CPU 录制预算相当于 **4.8 个正常帧**(66.8/14.0)。在 S1 下,同样换算相当于 3.3 个帧(70.2/21.5)。这一比例关系在两个方案下较为接近(4.8 对 3.3),不像 Debug 构建下测得的数据那样悬殊——这是因为 Release 构建下 S1 本身的稳态 `frame_wall` 已经很低(21.5us),使得更新成本相对帧预算的放大效应不再像 S3 那样悬殊。这提示"更新成本占比因方案而异"这一效应在 Release 构建下依然存在但幅度有限,不宜过度解读为方案选择对更新预算占比有决定性影响。

---

# 6. Discussion

本章将前述实证结果综合为对研究问题的直接回答,并探讨其对 GPU 驱动渲染设计和 PCG 管线实践的含义。

## 6.1 Which Stages Benefit from GPU-Driven Execution?

本文的核心研究问题是:在处理密集的程序化生成环境时,Vulkan 管线的哪些部分——scene representation、visibility culling、draw submission——从 GPU 驱动执行中受益最大?基于第 5 章的系统测量,三个阶段的收益并非互相独立,而是叠加且部分依赖于彼此。

绘制提交是收益最直接可归因的部分。S1→S2 的唯一变量是绘制机制(直接绘制改为间接绘制,culling 仍在 CPU 端):在 S2 稳定胜出的中大规模场景(grid≥256)下,CPU 录制成本下降 1.7--9.9×,典型约 5--8×,低密度场景偏高(完整分布见 §5.1.3)。在 grid=4096/chunk=4 的配置下,S1 需要发出约 114 万次 `vkCmdDrawIndexed` 调用,S2 无论场景规模都只发出 2 次 `vkCmdDrawIndexedIndirect`。将 culling 进一步迁移到 GPU(S2→S3)带来叠加收益,且该收益随 chunk 数增加而扩大——chunk16 时 3.2×,chunk4 时 32×——这一收益幅度的变化本身有意义:chunk 数越多,CPU 端逐块遍历的相对成本越高,GPU 并行 compute dispatch 的优势就越明显。两阶段叠加后,S3 相对 S1 的 CPU 录制成本差距在 grid1024 下为 21--166×,在 grid4096 下达 158--995×(均为 seed42/dens50 参考配置,跨 chunk)。

这一收益并非没有代价。S3 的 GPU 端多付出 compute dispatch、atomic 写入和两次管线屏障的成本,在 chunk16 粒度下体现为 `gpu_exec` +16\% 到 +48\% 的额外开销,在这一档位下是可接受的固定税费。但在 chunk4 的极端细粒度下(grid4096,104 万个 chunk),这一开销急剧放大:S3 的 `gpu_exec` 达到 236173us,远高于 S1 的 38724us,S3 把瓶颈从 CPU 移到了 GPU,但并未消除瓶颈,反而在这一极端配置下让 GPU 端总耗时变得更长。这提示 GPU 端剔除的收益是有 chunk 粒度上界的——当块数过多、每块工作过少时,compute dispatch 本身的调度开销会超过它替代的 CPU 端遍历所节省的成本。

对于 scene representation 的更新,本文观测到的是提交策略(batched vs per-chunk)对更新成本的影响远大于渲染方案的选择,但由于三个方案在本文实现中共享同一条更新代码路径(见 §3.2),这一结果更接近对实现前提的确认,而非独立验证的普适规律(见 §5.4.2 的讨论)。

## 6.2 绘制提交与剔除迁移收益的分离与适用范围

S2 相对 S1 的收益,以及它是否受可见性比影响,是两个需要分开讨论的问题(Release 构建下的测量依据见 §5.1.3;构建配置对测量的影响见 §4.3.4)。

第一个问题——S2 是否比 S1 更快——答案是肯定的,且这一收益直接来自间接绘制机制本身,与可见性比无关:即使 CPU 端仍对全部 chunk 执行 frustum 测试(在 §4.2.3 描述的全可见相机条件下,这一测试确实不产生任何剔除收益),将绘制调用从逐 chunk 改为恒定 2 次也足以带来净收益。第二个问题——CPU 端剔除本身在低可见性比场景下是否更有效——本文的实验设计(§4.2.3 所述的全可见相机)恰恰无法回答:全可见条件下,frustum 测试对提交给 GPU 的实例数没有任何影响,因此本文无法测得"有效剔除"与"无效剔除"之间的成本对比。Unterguggenberger 等 [2021] 在 meshlet 层面报告的"剔除开销随可剔除比例变化"曲线(0\% 可剔除时 +3.4\% 开销,100\% 可剔除时 -11.4\%)描述的正是这一权衡的两端,而本文的相机设计只测到了其中一端。将本文数据与该权衡曲线类比,需要补充非全可见的相机条件才能成立——这是 §7.2 和 §7.3 讨论的局限之一,而非本文已经验证的跨粒度印证。

对渲染管线设计者的可靠建议因此收窄为:在本文验证的范围内,将绘制提交迁移到间接绘制机制本身就有收益,不需要以可见性比为前提;但"是否需要在 CPU 端保留剔除逻辑"这一问题,依赖于目标场景的实际可见性分布,本文的全可见测试条件不能给出该问题的答案。

## 6.3 Implications for PCG Scene Management

seed9999/dens80 组合下观察到的现象——GPU 执行时间相差 11.4 倍——对 PCG 管线和 GPU 驱动渲染的关系有明确的机制含义,而非停留在推测层面。

如 §5.3.1 所述,直接解析 WFC 缓存文件证实这一差异的成因是网格构成在立方体家族和球体家族之间的近乎完全翻转(球体占比从 <0.01\% 跃变到 99.98\%),根源在于本文所用邻接规则下两个网格家族只能通过 EMPTY 瓦片连接,而 density=80\% 的权重配置使 EMPTY 稀缺,导致坍缩结果对随机种子高度敏感。这意味着 WFC 参数空间到 GPU 性能空间的映射确实不是等距的:两个具有相同 gridSize、chunkSize、density 参数形式的场景,仅因随机种子不同,就可能在 GPU 上相差一个数量级——不是因为"更大的 tile",而是因为不同网格类型(mesh type)可能有截然不同的几何复杂度(本文测得球体网格的三角形数是立方体网格的 64 倍)。对生成器代码和 95 个额外种子的独立验证进一步表明,这种家族级锁定在高密度设置下并非罕见——它是这一邻接规则结构的普遍行为,而 seed9999 只是恰好落入了其中一种(约 40\% 概率出现的)结果。

这一发现对 PCG 管线设计有具体含义:一个在 WFC 指标意义上"成功"的场景(满足邻接约束、达到目标密度、视觉连贯)可能恰好是 GPU 上性能最差的场景,因为 PCG 的优化指标和渲染性能指标是解耦的——尤其是当邻接规则允许网格类型发生这种家族级的、对渲染成本影响巨大的切换时。对于需要处理程序化生成内容的 GPU 驱动渲染管线,一个可能的实践方向是将 GPU 性能反馈接入 PCG 生成循环——例如在坍缩完成后按各网格类型的实际三角形数估计场景的渲染成本,超出预算时重新选择种子或调整邻接规则以避免家族级锁定。这一方向目前仍停留在设计层面,其必要性和具体实现需要在更大范围的种子和邻接规则设计上验证后才能成立,本文将其列为未来工作(§7.3)而非已证成的设计建议。

---

# 7. Conclusion

## 7.1 Summary of Findings

本文在 Vulkan 1.3 下构建了一个统一的 WFC 场景实验框架,通过三种渲染方案的对照实验,以 Release 构建定量分解了 GPU 驱动渲染在 scene representation、visibility culling 和 draw submission 三个管线阶段的收益分布。与 §1.3 一致,主要发现分为两项独立发现和三项对既有工程经验的定量验证:

**独立发现:**

**(1) 绘制提交与剔除迁移的收益是叠加的,且各自可分离测量,但需要足够的场景规模才能显现。** S1→S2(仅将直接绘制改为间接绘制)使 CPU 录制成本在中大规模场景(grid≥256)下下降 1.7--9.9×(典型约 5--8×,低密度偏高);S2→S3(在此基础上将剔除迁移到 GPU)带来进一步收益,且随 chunk 数增加而扩大(chunk16 时 3.2×,chunk4 时 32×)。两阶段叠加后,S3 相对 S1 的差距在 grid1024 下为 21--166×,在 grid4096 下达 158--995×(seed42/dens50 参考配置,跨 chunk)。S2 在 grid≥256 的测试配置下均快于 S1;在 grid=16 的更小场景下,间接绘制机制自身的固定开销超过其节省的绘制调用成本,S1 反而快 0.1--1.2us,该差距随网格增大而收窄并在 grid=64 时反转为 S2 领先。(构建配置效度说明及本结论的测量依据见 §4.3.4、§5.1.3。)

**(2) 在极端细粒度配置下观察到 GPU 端剔除收益的饱和迹象。** S2 与 S3 的对比隔离了 culling 从 CPU 迁移到 GPU 的收益:在 chunk16 粒度下,`gpu_exec` 的额外开销为 +16\% 到 +48\%,是可接受的固定税费;但在 chunk4 且 grid=4096(约 104 万个 chunk)的极端配置下,compute dispatch 的调度开销使 S3 的 `gpu_exec` 达到 236173us,远超 S1 的 38724us——GPU 端剔除把瓶颈从 CPU 移到了 GPU,在这一极端粒度下并未缩短总耗时。需强调的是,这一饱和现象目前仅在本文测试的最细 chunk 粒度(chunk=4)这一单一极端配置上观察到,尚不能据此断定收益上界的确切位置;确切的饱和边界需要在 chunk=4 与 chunk=8 之间补充采样点才能刻画(见 §7.3),因此此处报告为一项单点观察而非已确立的普适上界。

**对既有工程经验的定量验证:**

**(3) WFC 场景内容对 GPU 性能有显著影响,成因已定位到网格家族级构成翻转。** 同密度参数(80\%)下,种子 9999 相对种子 42/1337 的 GPU 执行时间差异在 4096² 网格下为 11.4×,且该比值随网格增大单调递减(256² 时 18.0× 降至 4096² 时 11.4×)。这一现象在三个渲染方案下一致出现。通过直接解析 WFC 缓存文件确认,成因是场景构成在立方体家族(约 15.2M 个实例)和球体家族(约 14.7M 个实例)之间的近乎完全翻转——球体网格的三角形数是立方体网格的 64 倍。这一家族级锁定并非罕见异常,而是本文邻接规则在高密度设置下的普遍行为(见 §5.3.1、§6.3)。

**(4) Scene representation 的更新成本对渲染方案的敏感度较低。** 局部更新成本在三个方案间的最大差异为 11.5\%(三种子均值,跨全部 update size),但由于三个方案在本文实现中共享同一条更新代码路径,这一结果更接近对实现前提的确认,而非独立于实现结构验证出的普适规律。

**(5) 局部更新成本对提交策略比对方案选择更敏感。** Per-chunk 同步提交在 32 chunk 时产生约 1897us 的更新成本(三方案均值),而 batched 单次提交仅需约 87us(21.9× 加速)。这一差距远大于方案间差异(<12%),表明在本文的实现约束下,提交策略是决定更新成本的主要因素。

## 7.2 Limitations

本文的工作存在以下局限,需要在解读结果时予以考虑:

**(1) 单一 GPU 平台。** 全部 810 个配置以 Release 构建在单一硬件配置(RTX 4080 SUPER)上完成(构建配置效度说明见 §4.3.4)。本文的 Debug→Release 修订经历本身也提示了平台敏感性:在不同 GPU 架构(AMD RDNA、集成 GPU、移动 GPU)上,CPU/GPU 通信带宽和计算着色器调度延迟的差异可能同样影响三个方案的相对排序,S3 的 GPU compute overhead 占 `gpu_exec` 的具体比例、以及 S1--S3 的相对排序,均需要在目标硬件上独立验证,不应直接套用本文测得的具体数值。

**(2) 仅 top-down 全可见相机。** 全可见条件最大化了可见实例数量,使得 GPU 负载主要由场景规模而非可见性比决定。在具有低可见性比的相机路径下(如第一人称视角、室内场景),绘制提交和剔除的收益分布可能与本文的观测存在数量级差异。同样地,本文的相机配置代表 CPU culling 在 chunk 级的最不利场景——在部分遮挡的环境下,S2 的相对表现应该优于本文的测量,而 S3 的 GPU culling 收益(通过减少 GPU 处理的实例数)可能更加显著。

**(3) 简单几何体。** 场景仅由立方体和球体组成,材质和着色器复杂度统一。具有高面数网格、多层材质和透明度、或程序化生成纹理的场景,可能改变 CPU/GPU 瓶颈在三个方案间的分布——片段着色器工作占比会升高,GPU culling 的相对 CPU 收益比例也会随之改变。

**(4) CPU culling 实现的保守性。** 本文的 CPU frustum culling 采用最朴素的逐 chunk 遍历,未使用空间加速结构(如 BVH、八叉树或层次包围体)。在实践应用中,CPU culling 可以使用这些结构降低遍历成本——因此本文中 S2 的退化成本是一个**上界估计**,实践中使用 BVH 或八叉树加速的 CPU culling 可能比本文报道的更有效。然而,如 Haar \& Aaltonen [2015] 所展示的,CPU culling 即使使用空间层次,在实例数达到数百万量级时仍然可能成为瓶颈。

**(5) chunk=4 / grid=4096 极端配置。** 在 grid=4096 下使用 chunk=4 产生了约 100 万个 chunk——每个 chunk 仅包含几个实例。这一配置的实践意义有限(它在空间划分上过于细粒度),但作为 GPU 驱动渲染的极限压力测试具有价值,并且是本文测得 S3 的 `gpu_exec` 开销急剧上升(见 §5.2.1)这一现象的来源。一旦去掉这一配置,本文的整体结论——间接绘制带来一致收益,GPU 端剔除的收益随 chunk 数扩大——仍然成立,但"GPU 端剔除存在 chunk 粒度上界"这一发现目前只在这一个极端配置上观测到,尚未确定该上界具体出现在多大的 chunk 数量级,需要在 chunk4 和 chunk8/16 之间补充更细的采样点才能刻画。

## 7.3 Future Work

本文开启了几条值得进一步探索的方向:

**(1) 跨 GPU 硬件对比。** 将相同的参数扫描在多种 GPU 架构上复现——AMD RDNA 3/4、Intel Arc、移动 GPU、集成 GPU——量化 GPU 驱动收益在硬件平台间的普遍性和差异性。这一扩展对于建立本文结论的普遍适用性至关重要。

**(2) 非 top-down 相机路径。** 将相机配置从 top-down 替换为第一人称或过肩视角路径,观察可见性比的动态变化如何影响三种方案的相对表现。特别地,在非全可见条件下,S3 的 GPU culling 可能不仅解放 CPU,还能通过减少 GPU 端处理的实例数量来直接降低 `gpu_exec`——这是本文全可见配置下未能观测到的一种收益机制。

**(3) Mesh Shader / Work Graphs 作为第四方案。** 在本文的三方案对照中,S3 仅将 culling 迁移到 GPU(compute shader),而几何处理管线(顶点/片段着色器)与传统方案相同。Vulkan 的 Mesh Shader 扩展和 DirectX 的 Work Graphs 代表了下一代的 GPU 驱动管线——几何处理本身也可在 GPU 端以更灵活的方式执行。将这些新特性纳入方案矩阵,直接比较 compute-based culling(S3)和 mesh-shader-based culling(Xylem 的第三方案),是一步自然的延伸。

**(4) 网格家族级锁定的范围与缓解。** 本文已确认 seed9999/dens80 的 11.4× GPU 时间差异源自立方体/球体网格家族的构成翻转,且这一锁定现象在本文邻接规则和 density=80\% 权重配置下具有普遍性(约 40\% 的独立种子会锁定为球体家族)。尚未验证的是:这一现象在其他密度、其他 tile 目录设计(如更多网格家族、不同的家族间邻接兼容性)下的普遍程度,以及它是否是这类"家族间仅通过 EMPTY 连接"的邻接规则的通用属性,还是本文特定瓦片目录的特例。后续工作应在更多邻接规则设计和密度参数上系统验证这一机制,并探索缓解方向——例如在邻接矩阵中允许不同网格家族直接相邻(打破"仅通过 EMPTY 连接"的结构),或在生成后按各网格类型的实际三角形数估计渲染成本、接入 WFC 生成循环作为重新选择种子的依据。

---

# References

[Gumin 2016] M. Gumin. "Wave Function Collapse." GitHub repository, 2016. https://github.com/mxgmn/WaveFunctionCollapse

[Haar & Aaltonen 2015] U. Haar and S. Aaltonen. "GPU-Driven Rendering Pipelines." In *SIGGRAPH 2015: Advances in Real-Time Rendering in Games*, 2015.

[Fang et al. 2025] Y. Fang, Q. Wang, and W. Wang. "Aokana: A GPU-Driven Voxel Rendering Framework for Open World Games." *ACM Trans. Graph. (SIGGRAPH Asia 2025)*, 2025. DOI: 10.1145/3728299

[Unterguggenberger et al. 2021] J. Unterguggenberger, B. Kerbl, J. Pernsteiner, and M. Wimmer. "Conservative Meshlet Bounds for Robust Culling of Skinned Meshes." *Computer Graphics Forum (Pacific Graphics 2021)*, Vol. 40, No. 7, pp. 57--69, 2021. DOI: 10.1111/cgf.14401

[Li et al. 2023] F. Li, S. Liu, N. Ma, Y. Liu, G. Xing, and Y. Zhang. "A GPU-friendly hybrid occlusion culling algorithm for large scenes." *Displays*, Vol. 80, Article 102533, 2023. DOI: 10.1016/j.displa.2023.102533

[Galajda 2020] R. Galajda. "Designing a Modern High-Level Graphics API." Master's thesis, Czech Technical University in Prague, 2020.

[Gonakhchyan 2018] V. I. Gonakhchyan. "Efficient Command Buffer Recording for Accelerated Rendering of Large 3D Scenes." In *Proc. CGVCVIP 2018* (IADIS), pp. 397--402, 2018.

[Sundararaman 2026] S. Sundararaman. "Xylem: A Comparative Analysis of GPU Dispatch Pipelines for Large-Scale, Procedural Environments." Master's thesis, California Polytechnic State University, 2026.

[Wang & Yu 2023a] P. Wang and Z. Yu. "RayBench: An Advanced NVIDIA-Centric GPU Rendering Benchmark Suite for Optimal Performance Analysis." *Electronics*, Vol. 12, No. 19, Article 4124, 2023. DOI: 10.3390/electronics12194124

[Wang & Yu 2023b] P. Wang and Z. Yu. "RenderBench: The CPU Rendering Benchmark Suite Based on Microarchitecture-Independent Characteristics." *Electronics*, Vol. 12, No. 19, Article 4153, 2023. DOI: 10.3390/electronics12194153

[Engel 2024] W. Engel (ed.). *GPU Zen 3: Advanced Rendering Techniques*. Independently published, 2024. ISBN 979-8-3442-3679-7.
