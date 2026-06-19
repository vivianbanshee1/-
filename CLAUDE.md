# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概况

这是一个图形版贪吃蛇课程设计（"大作业"），使用 C++11 + MinGW + EasyX 图形库实现，运行于 Windows。源码全部为 UTF-8 编码，编译期需通过 `-finput-charset=UTF-8 -fexec-charset=CP936` 处理中文界面文字。

## 构建命令

主构建入口是仓库根目录的 `build_cn.bat`（默认输出 `snake.exe`，会自动查找 `TDM-GCC-64` / `mingw64` 中的 `g++` 和 `easyx.h`）。常用形式：

```bat
:: 推荐：仓库自带脚本
build_cn.bat

:: 手动指定编译器
build_cn.bat "C:\TDM-GCC-64\bin\g++.exe"

:: 手动 g++（与 build_cn.bat 等价）
g++ -I"C:\TDM-GCC-64\x86_64-w64-mingw32\include" -O2 -std=c++11 ^
    -finput-charset=UTF-8 -fexec-charset=CP936 ^
    *.cpp -o snake.exe ^
    -leasyx -lwinmm -lgdi32 -luser32 -lkernel32 -lws2_32
```

- 输出名固定为 `snake.exe`（约定不生成 `snake_xxx.exe`）。
- `EasyX` 仅 Windows 头/库；如头文件路径不同，修改 `-I` 或设置 `EASYX_INC`。
- 终端出现中文乱码时，可临时执行 `chcp 936` 切换到 GBK 代码页。
- `.o` 与 `*.exe` 已写入 `.gitignore`，不要提交产物。

仓库**没有**自动化测试套件或 lint 配置；改完后用 `build_cn.bat` 重新编译即为"回归"手段。

## 代码架构（三模块分层）

代码按教学需要拆分为 3 个板块，每个板块配独立的 `.h` + `.cpp`：

### 板块一：游戏玩法核心 — `snake_logic.h` / `snake_logic.cpp`

所有游戏规则与数据。包含以下子系统：

- **蛇核心**：网格 `grid[GRID_SIZE][GRID_SIZE]`、`GameState` 大结构体、`Snake`/`AISnake`/`Position` 等数据结构；`gameInit / gameInitDual / gameMove / gameMoveDual / gameSetDir / gameApplySpeed / gameUpdateTimer`。
- **食物**：蓝/红食物的放置、计时、计分（`gamePlaceBlueFood / gamePlaceRedFood / gameUpdateRedTimer / calcRedScore`）。
- **道具**：8 种道具（极速/护盾/减速/磁铁/冻结/缩短/穿墙/双倍）的生成、收集、效果定时。用位掩码 + 独立定时器数组管理。
- **AI 蛇**：贪心寻路（Manhattan 距离向蓝食物移动），`aiInit / aiUpdate / aiMarkAll / aiKill / aiScoreReward`。
- **移动障碍**：漂移+反弹，受 `diffFactor` 难度加成。
- **连击**：3 秒窗口连击，`comboOnEat` 计算倍率（×1.0 → ×1.5 → ×2.0...）。
- **飘字**：得分动画（`floatTextAdd / floatTextUpdate`），最多 16 个并发。
- **成就**：10 个成就（位掩码）+ 累计统计，统一存于 `record.txt`。

### 板块二：系统服务层 — `snake_services.h` / `snake_services.cpp`

外围基础设施，不涉及游戏规则：

- **存档**：`record.txt` 读写（最高分/蛇颜色/地图大小/玩法/AI 开关/成就）。支持旧版 `achievements.txt` 兼容读取。
- **音效**：WAV 文件播放 + Beep 频率回退 + MCI 背景音乐。`soundPlayEatBlue / soundPlayDeath / soundPlayItemCollect` 等。
- **输入**：键盘/鼠标消息解包。`gfxGetKey / gfxGetMenuInput / gfxGetDeadInput / gfxIsKeyDown`。`VK_BOOST_P1='J'`、`VK_BOOST_P2='0'` 是加速键。

### 板块三：渲染与主流程 — `snake_render.h` / `snake_render.cpp`

所有 EasyX 绘制 + `main()` + `runGame()` 主状态机：

- **视图**：像素风深蓝主题、颜色渐变系统、蛇头方向感知绘制、蓝莓/草莓图标、道具彩色圆+字母。
- **菜单**：7 级菜单（主菜单/单人难度/双人模式/双人难度/镜像难度/设置/成就/玩法说明）。
- **HUD**：计分板、效果指示器、红食物倒计时条、连击提示。
- **主循环**：`STATE_MENU / STATE_PLAYING / STATE_PAUSED / STATE_DEAD_TITLE / STATE_DEAD` 状态机；`Sleep(16)` ≈ 60fps；`GetTickCount()` 时间驱动。

### 依赖关系

```
snake_logic.h          (基础层：数据结构+常量+所有逻辑API)
    ↑
snake_render.h         (渲染层：布局常量+gfx声明，含 snake_logic.h)
    ↑
snake_services.h       (服务层：存档/音效/输入声明，含 snake_render.h)
```

- `snake_logic.cpp` → `#include "snake_logic.h"` + `"snake_services.h"`
- `snake_render.cpp` → `#include "snake_render.h"` + `"snake_services.h"`
- `snake_services.cpp` → `#include "snake_services.h"`
- `snake.cpp` → 空文件，保留用于旧构建脚本兼容

教学文档位于 `docs/` 目录：`snake_logic.md` / `snake_services.md` / `snake_render.md`。

## 构建命令

```bat
:: 手动 g++
g++ -I"C:\TDM-GCC-64\x86_64-w64-mingw32\include" -O2 -std=c++11 ^
    -finput-charset=UTF-8 -fexec-charset=CP936 ^
    snake_logic.cpp snake_services.cpp snake_render.cpp snake.cpp -o snake.exe ^
    -leasyx -lwinmm -lgdi32 -luser32 -lkernel32 -lws2_32 -lcomdlg32
```

## 开发约定

- 不要改全局变量语义或破坏模块边界；`snake_logic.h` 是其它模块共同依赖的数据字典。
- 修改 `GameState` 字段后，需同步 `gameInit / gameInitDual` 中的 `memset` 重置路径以及 `snake_services.cpp` 中 record 部分的持久化字段。
- 新增 `CELL_*` 类型时，要更新 `snake_render.cpp` 的 `drawMapAt` 分支、`snake_logic.cpp` 中的碰撞判定，以及 AI 部分的 `isPassable`。
- 新增道具：在 `snake_logic.h` 加 `ITEM_*` 宏与 `EFF_*` 索引，并在 `snake_logic.cpp` 的 `itemCollect / setEffect` 中实现效果；在 `snake_render.cpp` 的 `drawPixelItem` 中加颜色与符号。
- 持久化文件：`record.txt` 统一存储设置+最高分+成就，位于程序工作目录（一般是仓库根）。
- 中文文案沿用现有写法（`_T("...")`、`drawtext`），保持 UTF-8 源 + CP936 执行字符集。
- 渲染层使用 `BeginBatchDraw / FlushBatchDraw / EndBatchDraw` 批绘。
