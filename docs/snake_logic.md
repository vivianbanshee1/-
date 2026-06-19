# 板块一：游戏玩法核心 — 教学文档

> **对应文件：** `snake_logic.h` + `snake_logic.cpp`  
> **模块定位：** 整个项目的"大脑"——所有游戏规则、碰撞判定、AI 行为、道具效果、计分逻辑都在这里。

---

## 1. 模块总览

```
snake_logic 板块一
├── 蛇核心逻辑      snake.cpp 部分    — 移动/碰撞/初始化/难度系统
├── 食物子系统      food.cpp 部分     — 蓝食物/红食物的放置与计时
├── 道具子系统      item.cpp 部分     — 8种道具的生成/收集/效果管理
├── AI 蛇子系统     ai.cpp 部分       — 贪心寻路的敌方蛇
├── 移动障碍        movingobs.cpp 部分 — 漂移反弹的移动障碍物
├── 连击系统        combo.cpp 部分    — 3秒窗口连击计分
├── 飘字系统        floattext.cpp 部分 — 得分/成就浮动文字
└── 成就系统        achievement.cpp 部分 — 10个成就的解锁与持久化
```

---

## 2. 核心数据结构

### 2.1 `Position` — 坐标

```c
typedef struct {
    int x, y;
} Position;
```

整个游戏使用 **网格坐标系**，原点在左上角，x 向右递增，y 向下递增。地图大小为 `mapSize × mapSize`（16/18/20）。

### 2.2 `Snake` — 蛇

```c
typedef struct {
    Position body[MAX_LEN];   // 身体数组，body[0] 是蛇头
    int      len;             // 当前长度
    int      dir;             // 玩家预设方向 (DIR_UP/DOWN/LEFT/RIGHT)
    int      lastDir;         // 实际最近移动方向（防止180度掉头自杀）
} Snake;
```

**关键设计：** `dir` 和 `lastDir` 是分离的。玩家按键只修改 `dir`，实际移动时用 `lastDir` 来禁止反向操作。例如蛇正在向右移动（`lastDir = RIGHT`），玩家按左键只会设置 `dir = LEFT`，但移动时 `gameSetDir` 会拒绝 180 度反转。

### 2.3 `GameState` — 游戏全局状态

这是一个超级结构体，包含约 70 个字段。按功能分组：

| 分组 | 关键字段 | 说明 |
|---|---|---|
| 网格 | `grid[20][20]` | 一格一值的网格世界，用 `CELL_*` 宏标识内容 |
| 蛇 | `snake`, `snake2` | P1 和 P2 的蛇数据 |
| 食物 | `blueFood`, `redFood`, `hasRed`, `redTimer`, `blueCount` | 食物位置与状态 |
| 计分 | `score`, `score2`, `highScore`, `mult` | 得分与倍率（系数×10） |
| 难度 | `difficulty`, `speed`, `baseSpeed`, `obsCount` | 难度影响速度/障碍数/得分系数 |
| 道具 | `itemOnField`, `itemPos`, `p1Effects`, `p1Timers`, `p1Shields` 等 | 道具位掩码 + 独立定时器数组 |
| AI | `ai[MAX_AI]`, `aiCount`, `aiSpawnCD` | AI 蛇数组与生成冷却 |
| 连击 | `comboCD`, `comboCount`, `maxCombo`（P1/P2 各独立一套） | 3秒窗口连击 |
| 成就 | `achUnlocked`, `achBlueTotal`, `achAIKills` 等 | 位掩码 + 累计计数 |

---

## 3. 格子类型系统

```c
#define CELL_EMPTY    0   // 空格
#define CELL_SNAKE    1   // P1 蛇身
#define CELL_BLUE     2   // 蓝色食物（永久存在）
#define CELL_RED      3   // 红色食物（限时，随时间衰减分数）
#define CELL_OBS      4   // 障碍物
#define CELL_WALL     5   // 边框墙
#define CELL_SNAKE2   6   // P2 蛇身（双人对战）
#define CELL_ITEM     7   // 道具
#define CELL_AI       8   // AI 蛇身
```

网格 `grid[y][x]` 中每个格子存一个 `CELL_*` 值。碰撞检测就是查目标格子的类型。

---

## 4. 蛇移动算法（核心）

### 4.1 单人移动 `gameMove()`

```
1. 计算蛇头下一步位置 (nx, ny)
2. 越界检查（出界→死亡，除非有护盾）
3. 读取目标格 cell = grid[ny][nx]
4. 道具检测：若目标格是 ITEM，虚拟清除（延迟收集）
5. 食物检测：eatBlue / eatRed
6. "虚拟清尾"——若不增长，先把尾巴从 grid 清除
7. 碰撞判定（重新检查 cell，考虑虚拟清尾后的状态）：
   - WALL/OBS/AI/SNAKE2 → 死亡（护盾可挡）
   - SNAKE 且非穿墙 → 死亡（护盾可挡）
8. 通过→收集道具（如果目标格是道具）
9. advanceSnake：蛇头前进，蛇尾（若不增长）移除
10. 得分处理（蓝食物/红食物/磁铁吸附）
```

### 4.2 双人移动 `gameMoveDual()`

增加了更多复杂度：
- `moveP1`/`moveP2` 参数支持各自独立速度
- **冻结效果**：被冻结的蛇 `moveP1/moveP2 = 0`
- **头对头碰撞**：两蛇同时移动到同一格→同归于尽
- **交叉穿越**：互换位置也算碰撞
- **护盾回滚**：碰撞后护盾生效→取消该蛇的移动

### 4.3 方向设置 `gameSetDir()`

```c
void gameSetDir(Snake *s, int dir) {
    // 禁止 180 度反转
    if (dir == DIR_UP    && s->lastDir == DIR_DOWN)  return;
    if (dir == DIR_DOWN  && s->lastDir == DIR_UP)    return;
    if (dir == DIR_LEFT  && s->lastDir == DIR_RIGHT) return;
    if (dir == DIR_RIGHT && s->lastDir == DIR_LEFT)  return;
    s->dir = dir;
}
```

### 4.4 速度系统 `gameApplySpeed()`

速度优先级：**极速(×3) > 按键加速(×2) > 正常 > 减速(×1.5)**

```c
if (itemHasEffect(g, player, EFF_TURBO))
    speed = baseSpeed / 3;       // 帧间隔除3 → 速度3倍
else if (boost)
    speed = baseSpeed / 2;       // 按键加速
else if (itemHasEffect(g, player, EFF_SLOW))
    speed = baseSpeed * 3 / 2;   // 帧间隔乘1.5 → 速度变慢
else
    speed = baseSpeed;
```

---

## 5. 食物系统

### 5.1 蓝食物
- 永久存在，被吃后立即重新生成
- 基础分 = `BLUE_BASE * mult / 10`（mult 是系数的 10 倍存储）
- 双倍道具下 ×2
- 累计吃 `RED_TRIGGER(3)` 个后触发红食物

### 5.2 红食物
- 限时存活 `RED_TIMER_MAX(10)` 秒
- 分数随时间衰减：`score = RED_BASE * redTimer / RED_TIMER_MAX`，最低 `RED_MIN(5)`
- 计时归零自动消失

### 5.3 放置算法 `randomEmptyCell()`
1. 随机尝试 `mapSize² × 2` 次找空格
2. 若失败，线性扫描整个网格找空格
3. 避开已有食物位置

---

## 6. 道具系统（8 种）

```c
#define ITEM_TURBO   1  // 极速：速度×3，5秒
#define ITEM_SHIELD  2  // 护盾：免死1次，可叠3层，15秒
#define ITEM_SLOW    3  // 减速：速度×1.5，4秒（负面）
#define ITEM_MAGNET  4  // 磁铁：3格内自动吃蓝食物，6秒
#define ITEM_FREEZE  5  // 冻结：双人模式冻结对手3秒
#define ITEM_SHRINK  6  // 缩短：蛇尾减3节，即时生效（负面）
#define ITEM_GHOST   7  // 穿墙：穿过自己身体，6秒
#define ITEM_DOUBLE  8  // 双倍：得分×2，10秒
```

### 6.1 效果管理

用 **位掩码 + 定时器数组** 管理：

```c
unsigned p1Effects;              // 位掩码：第 i 位=1 表示第 i 个效果生效中
float   p1Timers[NUM_EFFECTS];   // 每个效果的独立倒计时
int     p1Shields;               // 护盾层数（可叠加到 MAX_SHIELDS=3）
```

`setEffect(g, player, eff, duration)` 设置位掩码并启动定时器。`itemUpdate()` 每帧递减所有活动效果的定时器，过期自动清除。

### 6.2 生成节奏

道具随机生成间隔 8-15 秒，场上最多 1 个。存活 8 秒后自动消失。非双人模式不生成冻结道具。

### 6.3 护盾消耗

```c
int itemConsumeShield(GameState *g, int player) {
    if (*shields <= 0) return 0;
    (*shields)--;                           // 消耗一层
    achOnShieldBlock(g);                    // 成就计数
    if (*shields <= 0)
        *ef &= ~(1u << EFF_SHIELD);         // 层数归零→清除效果位
    else
        tm[EFF_SHIELD] = SHIELD_DURATION;   // 还有护盾→重置下一层计时
    return 1;                                // 护盾生效，免死
}
```

---

## 7. AI 蛇系统

### 7.1 贪心寻路策略

AI 不使用 BFS/A*，而是简单的**贪心算法**：

```
aiChooseDir():
  对四个方向：
    排除：越界 / 反向 / 不可通行格子
    计算：到蓝食物的曼哈顿距离
    选：距离最近的方向
  若无可行方向 → 保持原方向（等死）
```

### 7.2 曼哈顿距离

```c
static int manhattan(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}
```

### 7.3 生命周期

- 初始数量 0，场上最多 `MAX_AI(3)` 条
- 生成间隔 18-30 秒随机
- 初始长度 2-4 节随机
- 移动速度 `AI_SPEED(220ms)` 比玩家慢
- 撞墙/障碍→死亡；撞玩家蛇身→AI死亡+玩家得分

### 7.4 可通行判定

```c
static int isPassable(int cell) {
    return cell == CELL_EMPTY || cell == CELL_BLUE || cell == CELL_RED;
}
```

AI 不拾取道具（`CELL_ITEM` 排除在可通行之外），避免道具过期将 AI 身体格清空的 bug。

---

## 8. 移动障碍

- 场上最多 `MOVING_OBS_MAX(5)` 个
- 每 `MOVING_OBS_SPAWN(20s)` 生成一个（受 `diffFactor` 加速）
- 每秒移动一格，遇边界/静态障碍反弹
- 难度越高移动越快（`interval = 1.0 / diffFactor`）

---

## 9. 连击系统

- **窗口**：3 秒内连续吃食触发连击
- **倍率**：`baseScore × (1 + (comboCount-1) × 0.5)`
  - 1 连 → ×1.0
  - 2 连 → ×1.5
  - 3 连 → ×2.0
  - 4 连 → ×2.5
- P1 和 P2 各自独立的连击计数器

---

## 10. 飘字系统

```c
#define MAX_FLOAT_TEXTS 16

typedef struct {
    float x, y;          // 世界坐标（格子单位）
    float life;          // 剩余生命（秒）
    TCHAR text[16];      // 显示的文本
    COLORREF color;      // 文字颜色
} FloatText;
```

- 最多 16 个并发飘字
- 每秒上飘 0.8 格
- 随 `life` 衰减透明度渐出
- 吃蓝食物→白色 "+N"，连击≥3→金色，吃红食物→红色

---

## 11. 成就系统（10 个）

| ID | 名称 | 条件 |
|---|---|---|
| ACH_FIRST100 | 初出茅庐 | 首次得分 > 100 |
| ACH_BLUE100 | 百炼成钢 | 累计吃 100 个蓝食物 |
| ACH_BLUE20 | 大胃王 | 单局吃 20 个蓝食物 |
| ACH_COMBO5 | 连击大师 | 达成 5 连击 |
| ACH_KILLAI10 | 蛇猎人 | 累计杀死 10 条 AI |
| ACH_SHIELD10 | 钢铁之躯 | 护盾挡 10 次死亡 |
| ACH_SPEED200 | 速通达人 | 60 秒内得 200 分 |
| ACH_SURVIVE180 | 生存专家 | 生存模式活过 180 秒 |
| ACH_MIRRORWIN | 镜像王者 | 镜像对决获胜 |
| ACH_RED50 | 满分猎人 | 吃到满分红食物 |

**存储方式：** 位掩码 `achUnlocked`（每位代表一个成就）+ 累计计数写入 `record.txt`。

`achCheckAll()` 在每次吃食/AI击杀/护盾触发后调用，批量检查所有条件。

---

## 12. 难度系统

三档难度通过三个维度影响游戏：

| 难度 | 速度(ms/帧) | 障碍数 | 得分系数 |
|---|---|---|---|
| 简单 | 200 | 2 | ×1.0 |
| 普通 | 130 | 5 | ×5.0 |
| 困难 | 80 | 10 | ×10.0 |

双人模式有独立的难度速度参数（整体偏快以减少等待感）。

---

## 13. 游戏初始化流程

### 13.1 `gameInit()` — 单人初始化

```
1. 保存持久化数据（highScore/成就/配置）
2. memset 清零整个 GameState
3. 恢复持久化数据
4. 应用难度参数
5. setupGrid（全空）
6. 随机出生点（地图中央±2格范围内）
7. 初始蛇长3，朝上
8. 放置障碍物
9. AI 初始化（若开启）
10. 移动障碍初始化
11. 放置第一个蓝食物
```

### 13.2 `gameInitDual()` — 双人初始化

额外处理：
- P1 出生在左 1/4 象限，朝右
- P2 出生在右 3/4 象限，朝左
- 双人模式默认关闭 AI
- 限时赛设置初始倒计时

---

## 14. 关键设计模式

### 14.1 参数归一化

```c
static inline int normalizeSnakeColor(int color) { ... }
static inline int normalizeMapSize(int mapSize)   { ... }
static inline int normalizeItemMode(int itemMode) { ... }
static inline int normalizeAiEnabled(int aiEnabled) { ... }
```

所有外部输入通过归一化函数校验，防止非法值污染游戏状态。

### 14.2 虚拟清尾

碰撞判定前先将"本回合会移走的尾巴"从 grid 临时清除，再检查目标格。避免蛇即将离开的尾巴被误判为障碍。

### 14.3 护盾回滚

护盾触发时不推进蛇，而是取消该回合的移动，并恢复已清除的尾巴。确保护盾消耗后游戏状态一致。

---

## 15. 模块间调用关系

```
main.cpp (主循环)
  ├─→ gameInit / gameInitDual
  ├─→ gameMove / gameMoveDual
  │     ├─→ itemCollect         (吃到道具)
  │     ├─→ comboOnEat          (连击计分)
  │     ├─→ floatTextAdd        (飘字)
  │     ├─→ gamePlaceBlueFood   (刷新食物)
  │     ├─→ achCheckAll         (成就检查)
  │     └─→ gameApplyMagnet     (磁铁吸附)
  ├─→ gameApplySpeed
  ├─→ itemUpdate               (每帧)
  ├─→ aiUpdate                 (每帧)
  ├─→ movingObsUpdate          (每帧)
  ├─→ comboUpdate              (每帧)
  ├─→ floatTextUpdate          (每帧)
  └─→ gameUpdateTimer          (限时赛)
```
