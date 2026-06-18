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

## 代码架构（模块化分层）

代码按"模型层 + 视图层 + 子系统"拆分，`main.cpp` 是唯一入口，仅承担菜单状态机与主循环驱动。

- `snake.h` / `snake.cpp` — 游戏模型层核心：常量、网格 `grid[GRID_SIZE][GRID_SIZE]`、方向/格子/状态/模式/难度宏、`GameState` 大结构体；`gameInit / gameInitDual / gameMove / gameMoveDual / gameSetDir / gameApplySpeed / gameUpdateTimer`。
- `food.h` / `food.cpp` — 蓝/红食物的放置、计时、计分（`gamePlaceBlueFood / gamePlaceRedFood / gameUpdateRedTimer / calcRedScore`）。
- `ai.h` / `ai.cpp` — AI 蛇生命周期、贪心寻路（`Manhattan` 距离向蓝食物移动）、`aiInit / aiUpdate / aiMarkAll / aiKill / aiScoreReward`。AI 由设置 `aiEnabled` 控制是否开启。
- `item.h` / `item.cpp` — 8 种道具（极速/护盾/减速/磁铁/冻结/缩短/穿墙/双倍）的生成、收集、效果定时；道具系统仅在 `itemMode == GAMEPLAY_ITEM` 时启用，启用 `gfx` 才绘制道具。
- `movingobs.h` / `movingobs.cpp` — 移动障碍：漂移+反弹，`MOVING_OBS_SPAWN` 控制生成节奏，受 `diffFactor` 加成。
- `combo.h` / `combo.cpp` — 3 秒窗口连击，吃食时 `comboOnEat` 计算倍率。
- `floattext.h` / `floattext.cpp` — 飘字动画（`+score`、成就提示等）。
- `achievement.h` / `achievement.cpp` — 10 个成就（位掩码）+ 累计统计（蓝食物/AI 击杀/护盾拦截），存于 `achievements.txt`。
- `record.h` / `record.cpp` — `record.txt` 的最高分与设置（蛇颜色/地图大小/玩法/AI 开关）读写。
- `gfx.h` / `gfx.cpp` — 视图层：所有 EasyX 绘制、布局常量（`CELL_PX / WIN_W / WIN_H / MAP_X / MAP_Y` 等）、镜像对决双地图布局、按钮命中检测。
- `input.h` / `input.cpp` — 键盘/鼠标输入与悬停检测（`gfxGetKey / gfxGetMenuInput / gfxGetDeadInput / gfxIsKeyDown`）。`VK_BOOST_P1='J'`、`VK_BOOST_P2='0'` 是加速键。
- `main.cpp` — `runGame()` 主状态机：`STATE_MENU / STATE_PLAYING / STATE_PAUSED / STATE_DEAD_TITLE / STATE_DEAD`；菜单包括 `MENU_MAIN / MENU_SINGLE_DIFF / MENU_DUAL_MODE / MENU_DUAL_MIRROR / MENU_DUAL_DIFF / MENU_SETTINGS / MENU_ACHIEVEMENTS`。

跨模块关系：`main.cpp` 调用 `snake.cpp` 的移动函数，依次触发 `itemUpdate / comboUpdate / floatTextUpdate / movingObsUpdate / aiUpdate / gameUpdateRedTimer` 等子系统；任何子系统判定到事件都直接 `achCheckAll` 检查成就。`gfx.cpp` 渲染时只读 `GameState`，不修改逻辑。

游戏模式：`MODE_SINGLE` / `MODE_DUAL`（经典双人对战，WASD vs 方向键）/ `MODE_DUAL_TIMED`（限时赛）/ `MODE_DUAL_MIRROR`（两张相同地图各自独立）/ `MODE_SURVIVAL`。镜像对决在 `main.cpp` 中以 `g` 和 `g2` 两个独立 `GameState` 模拟。

## 开发约定

- 不要改全局变量语义或破坏模块边界（每个 `.cpp` 配同名 `.h`）；`snake.h` 是其它模块共同依赖的数据字典。
- 修改 `GameState` 字段后，需同步 `gameInit / gameInitDual` 中的 `memset` 重置路径以及 `record.cpp` 的持久化字段。
- 新增 `CELL_*` 类型时，要更新 `gfx.cpp` 的 `drawMapAt` 分支、`snake.cpp` 中的碰撞判定，以及 `ai.cpp` 的 `isPassable`。
- 新增道具：在 `item.h` 加 `ITEM_*` 宏与 `EFF_*` 索引，并在 `item.cpp` 的 `itemCollect / setEffect` 中实现效果；在 `gfx.cpp` 的 `drawPixelItem` 中加颜色与符号。
- 持久化文件：`record.txt`（设置+最高分）、`achievements.txt`（成就位掩码+累计计数），位于程序工作目录（一般是仓库根）。
- 中文文案沿用现有写法（`_T("...")`、`drawtext`），保持 UTF-8 源 + CP936 执行字符集。
- 渲染层使用 `BeginBatchDraw / FlushBatchDraw / EndBatchDraw` 批绘，不要在循环里直接 `cleardevice` 之外做无谓全屏重绘。
