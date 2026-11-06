# 第二次作业 · 视觉目标管理与跟踪系统

本工程按照原始作业要求实现普通装甲与旋转装甲的创建、跟踪、漏检预测、丢失及重新追踪，并在更新前校验整帧数据。

原始题目保存在 [docs/assignment.md](docs/assignment.md)。初学者可先读 [逐步讲解](docs/STEP_BY_STEP.md)，提交操作见 [提交指南](docs/SUBMISSION.md)。

## 0. 基本信息

- 姓名：待本人填写
- 学号：待本人填写
- 代码与测试完成日期：2026-11-06
- 是否完成全部基础功能：是
- 使用标准：C++17
- 外部运行库：无，仅使用 C++ 标准库
- 个人仓库地址：待创建并推送后填写

## 1. 类设计

| 类名 | 职责 | 继承自 |
| --- | --- | --- |
| `TrackingBase` | 保存私有状态和上次真实观测；处理位置、速度、预测、漏检和清零 | 无，是抽象基类 |
| `ArmorTarget` | 提供连续漏检 3 帧的阈值，不启用角度跟踪 | `TrackingBase` |
| `SpinningTarget` | 提供连续漏检 5 帧的阈值，启用角度跟踪 | `TrackingBase` |
| `TargetManager` | 持有目标，校验输入，更新目标，生成快照和四类计数 | 无 |

```text
TargetManager
  └── vector<unique_ptr<TrackingBase>>：拥有所有目标对象
                  ├── ArmorTarget
                  └── SpinningTarget
```

声明位于 `include/类名/类名.hpp`，实现位于 `src/类名/类名.cpp`。数据成员全部为 `private`；派生类通过覆写 `lostThreshold()` 和 `tracksAngle()` 提供差异规则，公共计算过程放在基类中。

基类的析构函数为 `virtual`，通过基类指针释放对象时可以正确调用派生类析构函数。`getSnapshot()` 返回副本，调用者修改结果不会改变管理器内部状态。

### 1.1 保存两种状态的原因

- `snapshot_`：用于对外输出的当前状态，包含观测位置和预测位置。
- `last_observation_`、`last_observation_timestamp_ns_`：最近一次真实观测及其时间，供下次计算速度使用。

旋转目标漏检时，快照中的 `angle` 会推进，而上次真实观测角度不应改变，因此不能只保存一份角度。

### 1.2 时间差与预测公式

```text
dt     = (本帧时间戳 - 上一成功帧时间戳) / 1e9
dt_obs = (本帧时间戳 - 此目标上次真实观测时间戳) / 1e9

vx = (本次 x - 上次观测 x) / dt_obs
vy = (本次 y - 上次观测 y) / dt_obs
角速度 = (本次观测角度 - 上次观测角度) / dt_obs
```

`1e9` 是浮点数十亿，将纳秒换成秒；首个成功帧的 `dt` 为 0。

观测到已有 `TRACK` 目标时，预测位置为 `(x + vx * dt, y + vy * dt)`。未观测到但仍为 `TRACK` 时，在原预测位置上累加 `(vx * dt, vy * dt)`，不改写 `x/y`。

旋转装甲被观测时，`predicted_angle = angle + angular_velocity * dt`；漏检但未丢失时，先推进 `angle`，再计算 `predicted_angle`。普通装甲的三个角度字段始终为 0。

新建或从 `LOST` 恢复的目标，速度与角速度为 0，预测值等于当前观测。进入 `LOST` 时九个运动数值全部清零，之后保留对象，`missed_count` 停在阈值，不继续增加。

## 2. 一帧处理流程

`TargetManager::onFrame()` 的实际执行顺序如下：

1. `isValidFrame()` 检查帧号和时间戳是否严格递增，并检查负数目标编号、同帧重复编号、已有编号类型冲突。
2. 任何一项不合格，直接 `makeResult(frame.frame_id, false)`，返回当前全部目标的快照。
3. 校验通过，计算当前帧间隔 `dt`。
4. 遍历观测：已存在的编号调用 `observe()`；新编号按类型创建派生类对象。
5. 遍历所有目标：本帧没有观测到的目标调用 `miss()`，根据阈值推进预测或转为 `LOST`。
6. 更新最近成功帧的帧号、时间戳和“已有成功帧”标记。
7. `makeResult()` 复制全部目标快照，统计 `TRACK/LOST` 与 `ARMOR/SPINNING` 数量，返回 `accepted=true`。

整帧校验和修改分成两遍。校验函数只读数据，拒绝路径不会进入 `observe()`、`miss()` 或创建对象的代码，也不会更新成功帧记录。因此，即使一帧前面的观测合法、最后一个观测非法，也不会留下部分修改。

快照包含 `LOST` 目标；类型统计同样包含它们。结果中的 `frame_id` 始终来自本次输入，包括被拒绝的帧。

## 3. 附加项

### 3.1 完成情况

| 附加项 | 是否完成 | 说明 |
| --- | --- | --- |
| 附加一：`LOST` 目标超时删除 | 未完成 | 保留全部 `LOST` 对象，遵循基础验收规则 |
| 附加二：类型变化时重建目标 | 未完成 | 类型冲突整帧拒绝 |
| 附加三：使用 `static` | 已完成 | 两种派生类中的丢失阈值 |
| 附加四：使用智能指针 | 已完成 | 管理器独占持有目标 |
| 附加五：目标排序 | 未完成 | 输出保持目标首次创建的顺序 |

### 3.2 静态成员与 `static`

新增代码中有两处静态成员：

- `include/ArmorTarget/ArmorTarget.hpp`：`static constexpr int LOST_THRESHOLD = 3;`
- `include/SpinningTarget/SpinningTarget.hpp`：`static constexpr int LOST_THRESHOLD = 5;`

阈值由目标类别决定，同一类的所有对象共用，不需要每个对象保存一份。`static` 表示成员属于类；`constexpr` 表示它是编译期常量。项目使用 C++17，这种静态常量可直接在类内定义。

如果改成普通的 `const int` 数据成员，功能仍然可以正确实现，但每个对象逻辑上都带有相同的成员，不能准确表达“类级规则”。如果改成可修改的普通成员，还可能出现同一种目标阈值不一致的问题。

原框架 `test_frames.hpp` 自带的局部 `static` 未修改，不计为新增实现。`static_cast` 是类型转换语法，与这里的静态成员不是一回事。

### 3.3 智能指针

使用位置：

- `include/TargetManager/TargetManager.hpp`：`std::vector<std::unique_ptr<TrackingBase>> targets_`。
- `src/TargetManager/TargetManager.cpp`：使用 `std::make_unique<ArmorTarget>()`、`std::make_unique<SpinningTarget>()` 创建对象并放入容器。

一个目标只归一个管理器所有，因此选择 `unique_ptr`，无需共享所有权。管理器销毁时，容器中的智能指针自动释放对象，避免手动 `new/delete` 的遗漏。通过基类指针调用公共函数，可以统一管理两种派生对象，并通过虚函数使用各自规则。

`Observation`、`InputFrame`、`TargetSnapshot`、`Result` 都是普通数据，适合按值存储或通过 `const` 引用传递，不需要智能指针。程序中的管理器本身是 `main()` 的局部对象，也无需动态分配。

## 4. 遇到的问题 / 未完成的部分

| 实现时需要解决的问题 | 处理方式 |
| --- | --- |
| 漏检后误用帧间隔计算速度 | 分开保存并使用 `dt` 与 `dt_obs` |
| 推算角度覆盖真实观测，导致下一次角速度错误 | 单独保存 `last_observation_` |
| 先更新前几个目标，随后发现非法观测 | 先完成整帧只读校验，再修改状态 |
| 首帧帧号和时间戳为 0，被初始值误拒绝 | 使用 `has_successful_frame_` 判断是否有比较基准 |
| `LOST` 后恢复时算出不合理的大速度 | 恢复首帧清零速度，从本次观测重新开始 |
| 结果计数使用了未初始化的整数 | `rm::Result result{}` 将计数初始化为 0 |

全部必做代码已实现，并通过下列测试。附加一、二、五未实现。当前没有发现基础规则范围内的失败用例，但固定测试和补充测试不代表穷尽所有输入。

提交准备仍需本人填写姓名、学号和仓库链接。个人远程仓库尚未创建，当前环境 GitHub 认证无效，因此尚未推送；本地 Git 历史不等于已上传成功。

## 5. 编译与运行

需要支持 C++17 的编译器、CMake 3.16 或更高版本，以及对应构建工具。工程不读取摄像头或图片，而是处理框架给定的观测序列。

在项目根目录执行（Linux / WSL / macOS）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
./build/tracker
ctest --test-dir build --output-on-failure
```

Windows 使用 Visual Studio 多配置生成器时：

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\tracker.exe
ctest --test-dir build -C Debug --output-on-failure
```

Windows 步骤供对应开发环境使用；本次实际验证环境是 Linux。`.vscode/c_cpp_properties.json` 是原框架的 WSL 配置，其他系统需要在 VS Code 中选择自己的编译器，不影响 CMake 源码逻辑。

## 6. 自测结果

验证日期：2026-11-06。环境：Debian GNU/Linux 13，GCC 14.2.0，CMake 4.4.4，Debug 构建。

本次编译额外设置了 `-Wall -Wextra -Wpedantic -Werror`，构建成功，无编译警告。运行 `ctest --test-dir build --output-on-failure`，结果为 **4/4 测试组通过**：

| 测试组 | 验证内容 |
| --- | --- |
| `provided_frames` | 原始 28 帧，23 次接受、5 次拒绝；关键运动数值、阈值、计数与快照 |
| `rejection_atomicity` | 五类拒绝条件；合法前缀不能被部分执行；拒绝后时间与帧号基准不变 |
| `variable_time_and_reacquisition` | 0.2/0.3/0.4 秒等非固定间隔；x/y 与角速度、跨帧号漏检、重新追踪 |
| `empty_first_frame` | 非法首帧、空首帧、编号 0、负坐标、返回快照与内部状态隔离 |

关键结果：

| 帧号 | 结果 |
| --- | --- |
| 1 | 目标 1 速度 `(10,0)`、预测位置 `(3,2)`；目标 2 角速度 `2`、预测角 `0.4` |
| 2–5 | 目标 2 的预测角依次为 `0.6、0.8、1.0、1.2` |
| 6 | 目标 2 进入 `LOST`；目标 1 速度约 `3.33`、预测 x 约 `5.33` |
| 7–8 | 目标 1 的预测 x 约为 `5.67、6.00`，观测坐标 x 仍为 `5` |
| 12–13 | 目标 1 丢失后重新追踪，速度归零 |
| 首次 16、首次 19、第二次 15、23、24 | 全部拒绝；各目标的每个快照字段和四个计数不变 |
| 第二次 17 | 创建目标 9，目标总数变为 4 |
| 20 | 四个目标全部为 `LOST`，对象仍保留 |
| 21–22 | 目标 2、9 分别恢复，速度为 0，预测等于当前观测 |

完整运行输出见 [docs/test-output.txt](docs/test-output.txt)。测试预期根据题目和公式手动填写，未由被测程序自动生成；比较计算结果时使用浮点误差容限，比较拒绝前后的状态时使用精确比较。

`include/data_types.hpp`、`include/test_frames.hpp` 未修改；`main()`、`printTarget()`、`printResult()` 未修改，仅替换了占位管理器并添加头文件引用。

## 7. Git 与 Markdown

代码在 `develop` 分支完成，按功能、测试、文档分别提交，再用 `--no-ff` 合并到 `main`，保留开发与合并记录。个人仓库创建、补全基本信息和推送步骤见 [提交指南](docs/SUBMISSION.md)。

本报告采用 Markdown 的标题、表格、代码块和相对链接。已查阅 [GitHub 官方 Markdown 语法说明](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax) 和 [VS Code 官方 Markdown 预览说明](https://code.visualstudio.com/docs/languages/markdown)。Windows/Linux 的 VS Code 中按 `Ctrl+Shift+V` 打开预览；依次按 `Ctrl+K`、`V` 可在侧边预览。
