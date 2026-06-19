# 板块三：渲染与主流程 — 教学文档

> **对应文件：** `snake_render.h` + `snake_render.cpp`  
> **模块定位：** 游戏的"脸面"和"心脏"——所有的画面绘制、UI交互、状态机驱动、主循环都在这里。

---

## 1. 模块总览

```
snake_render 板块三
├── 颜色常量定义     — 全局视觉方案的 RGB 宏
├── 基础绘制工具     — 文字居中、颜色混合、棋盘背景
├── 地图绘制         — 网格棋盘、木框、渐变色背景
├── 像素画绘制       — 蛇头(带眼睛)/蛇身/障碍/蓝莓/草莓/道具图标
├── UI 面板系统      — 像素风按钮、信息面板、覆盖弹窗
├── 菜单系统         — 7 级菜单的状态化渲染
├── 游戏 HUD         — 计分板、效果指示器、连击提示、飘字
├── 死亡/结算界面    — GAME OVER 动画、双人胜负、按键提示
├── 镜像对决渲染     — 双地图并排、立即结算按钮
└── 主循环 runGame()  — 整个游戏的状态机与帧驱动
```

---

## 2. EasyX 图形库简介

EasyX 是 Windows 下的入门级 C/C++ 图形库，封装了 GDI。核心概念：

```c
initgraph(WIN_W, WIN_H);    // 创建绘图窗口
cleardevice();              // 清屏
setfillcolor(RGB(r,g,b));   // 设置填充色
solidrectangle(x1,y1,x2,y2); // 画实心矩形
outtextxy(x, y, text);      // 输出文字
FlushBatchDraw();           // 批量提交绘制
```

本项目的渲染全部使用 **批量绘制模式**：
```c
BeginBatchDraw();           // 开始批绘
// ... 所有绘制调用 ...
FlushBatchDraw();           // 一次性提交到屏幕
EndBatchDraw();             // 结束批绘（仅退出时调用）
```

---

## 3. 布局常量体系

### 3.1 窗口与棋盘

```c
#define CELL_PX  24        // 每格像素边长
#define WIN_W    1100      // 窗口宽度（扩宽以容纳双地图）
#define WIN_H    600       // 窗口高度

// 单地图模式：棋盘在窗口正中偏下
#define MAP_X    60
#define MAP_Y    80

// 镜像对决：两张地图并排
#define MIRROR_MAP1_X  50
#define MIRROR_MAP2_X  570
#define MIRROR_MAP_Y   56
```

### 3.2 菜单按钮布局

```c
#define MENU_BTN_W        320    // 按钮宽
#define MENU_BTN_H         56    // 按钮高
#define MENU_BTN_GAP        62    // 按钮间距
#define MENU_BTN_START_Y   206    // 第一个按钮 Y
#define MENU_BTN_X   (WIN_W/2 - MENU_BTN_W/2)  // 居中
```

所有按钮通过 `MENU_BTN_X` 水平居中，Y 坐标 = `MENU_BTN_START_Y + (index-1) * MENU_BTN_GAP`。

---

## 4. 视觉方案：像素风深蓝主题

### 4.1 色彩分层

```
深色背景层 CLR_BG_DARK (14,44,87)
    ↓ 渐变棋盘
中层面板 CLR_PANEL_SOFT (45,104,173)
    ↓ 面板描边
亮色边框 CLR_PANEL_GLOW (117,191,255)
    ↓ UI点缀
金色高亮 CLR_GOLD (255,215,93)
```

### 4.2 渐变色生成

```c
// 颜色混合：t=0.0→src, t=1.0→dst
static COLORREF blendColor(COLORREF src, COLORREF dst, float t) {
    return RGB(
        clampi(sr + (dr - sr) * t, 0, 255),
        clampi(sg + (dg - sg) * t, 0, 255),
        clampi(sb + (db - sb) * t, 0, 255)
    );
}

// 棋盘光晕：越靠近中心越亮
static COLORREF boardGlowAt(baseColor, x, y, cx, cy, maxDist) {
    float d = sqrt((x-cx)² + (y-cy)²);
    float k = 1.0 - d / maxDist;  // 0~1，中心亮
    return blendColor(baseColor, RGB(230,250,255), 0.05 + 0.12*k²);
}
```

### 4.3 蛇颜色方案

**P1 三色可选：**

| 选项 | 蛇头 | 蛇身 |
|---|---|---|
| 绿色 (默认) | `RGB(32,205,130)` | `RGB(28,170,95)` → `RGB(19,102,60)` 渐变 |
| 橙色 | `RGB(255,170,20)` | `RGB(255,132,20)` → `RGB(210,74,8)` |
| 紫色 | `RGB(228,92,255)` | `RGB(195,66,227)` → `RGB(138,28,162)` |

**P2 固定：** 金黄色系 `RGB(255,196,70)` → `RGB(160,115,35)`

**AI 固定：** 淡紫色系 `RGB(190,130,255)` → `RGB(96,65,158)`

---

## 5. 核心绘制函数

### 5.1 `drawMapAt(g, ox, oy)` — 地图绘制

```
1. drawWoodFrame(ox, oy, mapSize)      → 外框（深色木框 + 内嵌面板）
2. drawMapChecker(ox, oy, mapSize)     → 棋盘底色（带光晕渐变 + 网格线）
3. 遍历 grid[y][x]：
   ├─ CELL_WALL     → drawPixelWall        // 实心方块
   ├─ CELL_OBS      → drawPixelObs         // 圆角方块
   ├─ CELL_BLUE     → drawPixelBlueFood    // 蓝莓图标
   ├─ CELL_RED      → drawPixelRedFood     // 草莓图标
   ├─ CELL_ITEM     → drawPixelItem        // 彩色圆+字母
   ├─ CELL_SNAKE    → drawPixelSnakeHead/Body  // 蛇头带眼睛
   ├─ CELL_SNAKE2   → drawPixelSnakeHead/Body  // P2 颜色
   └─ CELL_AI       → drawPixelSnakeHead/Body  // AI 颜色
```

### 5.2 蛇头绘制 — 方向感知

蛇头是一个实心方块 + 两只白色眼睛。眼睛位置根据蛇的方向变化：

```c
// 朝上：眼睛在上半区，纵向短线
eyeX = cx ± CELL_PX/6;
eyeY = top + CELL_PX/3;
line(eyeX, eyeY, eyeX, eyeY + eyeLen);  // 竖线

// 朝下：眼睛在下半区，纵向短线
eyeY = top + CELL_PX*2/3 - eyeLen;

// 朝左：眼睛在左前方，横向短线
line(eyeX, eyeY, eyeX + eyeLen, eyeY);  // 横线

// 朝右：眼睛在右前方，横向短线
```

### 5.3 食物绘制 — 水果图标

**蓝莓：** 深紫色实心圆 + 白色高光斑 + 顶部绿色小三角形叶子 + 底部深色果蒂

**草莓：** 红色倒三角形（上宽下窄）+ 顶部绿色圆冠 + 5片小叶 + 散布的黄色籽点

### 5.4 道具绘制 — 彩色圆 + 字母

```c
// 每种道具：不同底色 + 白色字母标识
ITEM_TURBO  → 黄色圆 + "Z"     ITEM_SLOW   → 紫色圆 + "−" (红框)
ITEM_SHIELD → 蓝色圆 + "S"     ITEM_SHRINK → 红色圆 + "X" (红框)
ITEM_MAGNET → 橙色圆 + "M"     ITEM_FREEZE → 冰蓝圆 + "F"
ITEM_GHOST  → 白色圆 + "G"     ITEM_DOUBLE → 金色圆 + "2"
```

负面道具（减速/缩短）使用红色边框区分。

---

## 6. UI 系统

### 6.1 像素风面板

```c
static void drawPixelPanel(x, y, w, h, fill) {
    // 三层结构：
    // 1. 最外层：CLR_PANEL_DK  (深色底，偏移-2)
    // 2. 中层：  CLR_PANEL_LINE (描边色，精确尺寸)
    // 3. 内层：  fill           (填充色，缩进+2)
}
```

模拟像素游戏的"浮雕"边框效果。

### 6.2 按钮组件

```c
static void drawPixelButton(int index, LPCTSTR text, int hover) {
    drawPixelPanel(x, y, w, h, fill);           // 面板底
    rectangle(x+2, y+2, x+w-2, y+h-2);          // 内框
    // 左上角色块：5个按钮各分配主题色
    solidcircle(x+14, y+14, hover?7:5);         // 圆点装饰
    // hover 时右下角小三角
    // 文字居中
}
```

**按钮主题色分配：**

| 按钮 | 主题色 | 含义 |
|---|---|---|
| 单人模式 | 青色 | 清新 |
| 双人对战 | 粉色 | 对抗 |
| 玩法说明 | 琥珀色 | 温暖 |
| 设置 | 嫩绿色 | 调节 |
| 成就 | 金色 | 荣耀 |

### 6.3 信息面板

```c
static void drawInfoPanel(x, y, w, h, title, content, titleColor) {
    drawPixelPanel(...);
    标题文字 (16px, 左对齐)
    分割线
    内容文字 (14px)
}
```

用于游戏中的计分板、难度/地图显示、效果指示器等。

### 6.4 覆盖弹窗

```c
static void drawOverlayPanel(x, y, w, h) {
    drawPixelPanel(..., CLR_PANEL_SOFT);
    line(x+6, y+h+1, x+w-6, y+h+1);  // 底部发光分割线
}
```

用于暂停界面、GAME OVER 弹窗等需要遮盖游戏画面的场景。

---

## 7. 菜单系统（7 级状态）

菜单使用 `menuPage` 变量驱动状态切换，每个页面独立渲染：

```
MENU_MAIN (0)
├─ '1' → MENU_SINGLE_DIFF (1)  — 选难度或生存模式
├─ '2' → MENU_DUAL_MODE (4)    — 选经典/限时/镜像
├─ '3' → MENU_HOWTOPLAY (7)    — 玩法说明（含道具列表+AI说明）
├─ '4' → MENU_SETTINGS (3)     — 蛇颜色/地图/玩法/AI设置
└─ '5' → MENU_ACHIEVEMENTS (6) — 10个成就的解锁状态

MENU_DUAL_MODE (4)
├─ '1' → MENU_DUAL_DIFF (2)   — 经典对战难度选择
├─ '2' → MENU_DUAL_DIFF (2)   — 限时赛难度选择
└─ '3' → MENU_DUAL_MIRROR (5) — 镜像对决难度选择
```

### 7.1 玩法说明页 (`MENU_HOWTOPLAY`)

包含完整的游戏规则说明：
- 经典模式 vs 道具模式对比
- 8 种道具的名称、图标色块、描述
- AI 敌人特性说明（贪心寻路、生成规则、击杀奖励）

---

## 8. 游戏 HUD

### 8.1 布局

```
┌──────────────┬──────────────────────┬──────────┐
│ P1 计分板     │                      │ 难度/地图 │
│ 得分 / 时间   │      游戏地图         │          │
├──────────────┤                      ├──────────┤
│ P1 效果列表   │                      │ P1 效果   │
│              │                      │ 红食物条  │
└──────────────┴──────────────────────┴──────────┘
│              操作提示 / COMBO 提示                  │
└────────────────────────────────────────────────────┘
```

### 8.2 效果指示器

```c
static void drawEffectIndicators(g, player, startX, startY) {
    // 逐项检查位掩码，显示活动效果名称+剩余秒数
    if (EFF_TURBO)  → "极速: 3.2s"  (黄色)
    if (EFF_SHIELD) → "护盾 x2"    (蓝色，显示层数)
    if (EFF_SLOW)   → "减速: 1.5s"  (紫色)
    if (EFF_MAGNET) → "磁铁: 4.1s"  (橙色)
    // ... 等等
    if (全部无)     → "无状态"      (灰色)
}
```

### 8.3 红食物倒计时条

红食物随着 `redTimer` 递减，进度条从满宽渐变到零，颜色为 `CLR_RED_FOOD` 红色。

### 8.4 连击提示

当 `comboCount >= 2` 时，屏幕顶部显示金色 "COMBO x3 !"。

---

## 9. 死亡与结算界面

### 9.1 两阶段死亡动画

```
STATE_DEAD_TITLE  — "GAME OVER" 大字 + 缓冲倒计时（0.~秒）
    ↓ 计时到或按任意键
STATE_DEAD        — 分数面板 + R/M/Q 按键选项
```

### 9.2 死亡结算面板

```c
static void drawKeyHintsVertical(px, pw, startY, hoverIndex) {
    // 3个选项，hover时高亮
    // "R 重新开始" (绿色) | "M 返回菜单" (棕色) | "Q 退出游戏" (红色)
}
```

### 9.3 双人结算

双人模式下，胜负判定优先级：

1. `winner == WIN_DRAW` → 平局
2. `winner == WIN_P1` 且 P1 分高 → P1 获胜
3. `winner == WIN_P2` 且 P2 分高 → P2 获胜
4. 无人获胜但分不同 → 分高者获胜
5. 分相同 → 平局

---

## 10. 镜像对决渲染

镜像对决使用双地图并排布局：

```
┌─────────────┬──┬─────────────┐
│  P1 计分板   │  │  P2 计分板   │
│  P1 效果     │  │  P2 效果     │
│             │  │             │
│   P1 地图   │  │   P2 地图   │
│  (左 480px) │  │  (右 480px) │
└─────────────┴──┴─────────────┘
```

两张地图使用**相同随机种子**生成 → 完全相同的障碍/食物布局 → 公平对战。

### 10.1 立即结算按钮

当一方阵亡时，另一方可以点击"立即结算"按钮提前结束比赛（无需等另一方也阵亡）。

```c
static void computeMirrorEndRect(&x, &y, &w, &h) {
    // 按钮居中在窗口下半部
    cx = MIRROR_GAP_CX;  // 550
    cy = WIN_H/2 + 100;
    w = 128; h = 42;
}
```

---

## 11. 主循环 `runGame()` — 完整状态机

```
runGame()
  │
  ├─ 初始化：
  │   ├─ 加载存档 → g.highScore, cfg, 成就状态
  │   ├─ gfxInit() → 创建 EasyX 窗口
  │   └─ soundInit() → 解析音效目录
  │
  └─ while (running):
      │
      ├─ gfxWindowCloseRequested() → break
      │
      ├─ if (STATE_MENU):
      │   ├─ gfxDrawMenu(&g, menuPage, hoverIndex)
      │   ├─ gfxGetMenuInput(&hoverIndex)
      │   └─ 处理菜单导航/游戏启动
      │
      ├─ if (STATE_PLAYING) — 镜像对决：
      │   ├─ 处理 P1/P2 输入
      │   ├─ DWORD now = GetTickCount()
      │   ├─ 每帧推进两蛇移动/道具/AI/障碍
      │   ├─ gfxDrawMirrorGame(&g, &g2, ...)
      │   ├─ 死亡检测 → STATE_DEAD_TITLE
      │   └─ 立即结算按钮检测
      │
      ├─ if (STATE_PLAYING) — 单人/双人：
      │   ├─ 处理输入
      │   ├─ DWORD now = GetTickCount()
      │   ├─ if (now - lastTickP1 >= speed)  → gameMove()
      │   ├─ if (now - lastTickP2 >= speed2) → gameMoveDual()
      │   ├─ itemUpdate / aiUpdate / movingObsUpdate / ...
      │   ├─ gfxDrawGame(&g)
      │   └─ 死亡检测 → STATE_DEAD_TITLE
      │
      ├─ if (STATE_PAUSED):
      │   └─ gfxDrawPause()
      │
      ├─ if (STATE_DEAD_TITLE):
      │   ├─ gfxDrawDeadTitle(&g, remainingMs)
      │   ├─ 倒计时递减
      │   └─ 计时到或按键 → STATE_DEAD
      │
      └─ if (STATE_DEAD):
          ├─ 更新最高分
          ├─ gfxDrawGameOver / gfxDrawDualOver
          ├─ gfxGetDeadInput(&hoverIndex)
          └─ 'R'→重新开始 'M'→菜单 'Q'→退出
      │
      └─ Sleep(16)  → 约 60fps
```

### 11.1 帧率控制

```c
Sleep(16);  // 每帧休眠 16ms ≈ 62.5fps 上限
```

不依赖垂直同步，用简单 `Sleep` 控制 CPU 占用。游戏逻辑使用 `GetTickCount()` 做时间增量驱动。

### 11.2 双时间系统

**逻辑更新** 用 `GetTickCount()` 比较：
```c
DWORD now = GetTickCount();
if (now - g.lastTickP1 >= g.speed) {
    gameMove(&g);           // 蛇移动
    g.lastTickP1 = now;     // 更新上次移动时刻
}
```

**道具/AI/障碍** 用 `dt`（delta time，秒）推进：
```c
float dt = (now - lastTick) / 1000.0f;
itemUpdate(&g, dt);         // 每帧调用，内部按 dt 衰减
aiUpdate(&g, dt);
movingObsUpdate(&g, dt);
```

---

## 12. 渲染性能优化

### 12.1 批量绘制

所有 EasyX 绘制操作在 `BeginBatchDraw()` 和 `FlushBatchDraw()` 之间执行，减少屏幕闪烁：

```c
BeginBatchDraw();
while (running) {
    cleardevice();
    // ... 绘制所有内容 ...
    FlushBatchDraw();
}
EndBatchDraw();
```

### 12.2 局部重绘 vs 全屏重绘

当前采用**每帧全屏重绘**策略。原因：
- 游戏画面元素多且变化频繁（蛇移动、飘字、效果指示器）
- EasyX 的局部更新能力有限
- `Sleep(16)` 下 60fps 全屏重绘性能足够

---

## 13. 鼠标命中检测

### 13.1 菜单按钮命中

```c
int gfxHitMenuButton(int mx, int my) {
    // 遍历 s_menuBtns[] 数组
    // 每个按钮在 drawPixelButton 时注册位置
    // 返回 1-based 按钮索引，0=未命中
}
```

### 13.2 死亡界面按钮命中

```c
int gfxHitDeadButton(int mx, int my) {
    // s_deadBtnX, s_deadBtnW, s_deadBtnY[] 全局缓存
    // 3个按钮：R(重新开始) M(菜单) Q(退出)
}
```

### 13.3 菜单角落按钮

```c
int gfxHitMenuCornerAction(int mx, int my) {
    // 4个功能按钮：
    // "图" → 选择背景图   "清图" → 清除背景图
    // "清音" → 清除音乐    "音" → 选择背景音乐
}
```

---

## 14. 完整函数调用链

```
main()
  └─ runGame()
       ├─ loadRecordConfig()     [板块二]
       ├─ achLoad()              [板块一]
       ├─ gfxInit()              [本模块]
       ├─ soundInit()            [板块二]
       └─ while(running):
            ├─ gfxWindowCloseRequested()  [板块二]
            ├─ gfxDrawMenu/Game/Pause/... [本模块]
            ├─ gfxGetMenuInput/Key/...    [板块二]
            ├─ gameInit/gameInitDual      [板块一]
            ├─ gameMove/gameMoveDual      [板块一]
            ├─ itemUpdate/aiUpdate/...    [板块一]
            ├─ gameSetDir/gameApplySpeed  [板块一]
            ├─ soundPlay*()               [板块二]
            ├─ saveRecordConfig()         [板块二]
            └─ gfxClose()                 [本模块]
```
