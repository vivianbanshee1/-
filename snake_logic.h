/* ============================================================
 *  snake_logic.h - 游戏逻辑模块接口
 *
 *  负责：
 *  - 游戏状态/规则相关数据类型与常量
 *  - 核心游戏逻辑函数的对外声明
 *  - 食物/道具/AI/移动障碍/连击/飘字/成就子系统声明
 *
 *  对应实现文件：snake_logic.cpp
 * ============================================================ */
#ifndef SNAKE_LOGIC_H
#define SNAKE_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>     /* DWORD, COLORREF */
#include <tchar.h>       /* TCHAR */

/* ===================================================
 *  ===== 第一部分：基础常量与数据结构 =====
 *  - 记录网格/方向/格子/模式/道具/游戏难度等规则常量
 *  - 定义蛇、AI、道具、游戏状态与全局配置结构体
 * =================================================== */
#define GRID_SIZE     20    /* 最大地图边长（含边框墙）*/
#define MAX_LEN       400   /* 蛇最大长度 */
#define MAX_OBS       10    /* 障碍数上限 */

#define MAP_SIZE_SMALL  16  /* 设置页小地图 */
#define MAP_SIZE_MEDIUM 18  /* 设置页中地图 */
#define MAP_SIZE_LARGE  20  /* 设置页大地图 */

/* ===================================================
 *  方向 / 格子类型 / 游戏状态
 * =================================================== */
#define DIR_UP        0
#define DIR_DOWN      1
#define DIR_LEFT      2
#define DIR_RIGHT     3

#define CELL_EMPTY    0     /* 空格 */
#define CELL_SNAKE    1     /* P1 蛇身 */
#define CELL_BLUE     2     /* 蓝色食物（永久）*/
#define CELL_RED      3     /* 红色食物（限时）*/
#define CELL_OBS      4     /* 障碍物 */
#define CELL_WALL     5     /* 边框墙 */
#define CELL_SNAKE2   6     /* P2 蛇身（双人对战模式）*/
#define CELL_ITEM     7     /* 道具 */
#define CELL_AI       8     /* AI蛇身 */

#define STATE_MENU        0
#define STATE_PLAYING     1
#define STATE_PAUSED      2
#define STATE_DEAD_TITLE  3   /* "GAME OVER"标题阶段 */
#define STATE_DEAD        4   /* 分数结算阶段 */

#define MENU_MAIN         0   /* 一级菜单 */
#define MENU_SINGLE_DIFF  1   /* 单人难度选择 */
#define MENU_DUAL_DIFF    2   /* 双人难度选择 */
#define MENU_SETTINGS     3   /* 设置页 */
#define MENU_DUAL_MODE    4   /* 双人模式选择 */
#define MENU_DUAL_MIRROR  5   /* 镜像对决难度选择 */
#define MENU_ACHIEVEMENTS 6   /* 成就查看 */
#define MENU_HOWTOPLAY    7   /* 玩法说明 */

/* ===================================================
 *  道具类型
 * =================================================== */
#define ITEM_NONE     0
#define ITEM_TURBO    1     /* 极速：速度×3，5秒 */
#define ITEM_SHIELD   2     /* 护盾：免死1次，可叠3层，15秒 */
#define ITEM_SLOW     3     /* 减速：速度×1.5，4秒 */
#define ITEM_MAGNET   4     /* 磁铁：3格内自动吃蓝食物，6秒 */
#define ITEM_FREEZE   5     /* 冻结：双人模式冻结对手3秒 */
#define ITEM_SHRINK   6     /* 缩短：蛇尾减3节，即时 */
#define ITEM_GHOST    7     /* 穿墙：穿过自己身体，6秒 */
#define ITEM_DOUBLE   8     /* 双倍：得分×2，10秒 */
#define ITEM_MAX      8

/* 效果索引（用于位掩码和定时器数组） */
#define EFF_TURBO     0
#define EFF_SHIELD    1
#define EFF_MAGNET    2
#define EFF_GHOST     3
#define EFF_DOUBLE    4
#define EFF_SLOW      5
#define EFF_FROZEN    6
#define NUM_EFFECTS   7

/* ===================================================
 *  游戏模式
 * =================================================== */
#define MODE_SINGLE       0   /* 单人 */
#define MODE_DUAL         1   /* 双人经典 */
#define MODE_DUAL_TIMED   2   /* 双人限时赛 */
#define MODE_DUAL_MIRROR  3   /* 镜像对决 */
#define MODE_SURVIVAL     4   /* 生存模式 */

/* 游戏玩法 */
#define GAMEPLAY_CLASSIC  0   /* 经典版：无道具 */
#define GAMEPLAY_ITEM     1   /* 道具版：有道具 */

#define SNAKE_COLOR_GREEN   0  /* P1绿色 */
#define SNAKE_COLOR_ORANGE  1  /* P1橙色 */
#define SNAKE_COLOR_PURPLE  2  /* P1紫色 */
#define SNAKE_COLOR_BLUE    SNAKE_COLOR_ORANGE  /* 兼容保留：不在菜单中展示 */

/* ===================================================
 *  双人胜负
 * =================================================== */
#define WIN_NONE      0       /* 进行中 */
#define WIN_P1        1
#define WIN_P2        2
#define WIN_DRAW      3       /* 平局（同归于尽）*/

/* ===================================================
 *  难度参数（速度/障碍数/得分系数）
 *  系数x10存为整数避免浮点
 * =================================================== */
#define EASY_SPEED    200   /* ms/帧 */
#define NORMAL_SPEED  130
#define HARD_SPEED    80

#define EASY_OBS      2
#define NORMAL_OBS    5
#define HARD_OBS      10

#define EASY_MULT     10    /* x1.0 */
#define NORMAL_MULT   50    /* x5.0 */
#define HARD_MULT     100   /* x10.0 */

#define DUAL_EASY_SPEED    180   /* 双人休闲 */
#define DUAL_NORMAL_SPEED  160   /* 双人标准 */
#define DUAL_HARD_SPEED    140   /* 双人激烈 */

#define TIMED_EASY   180   /* 限时赛简单（秒） */
#define TIMED_NORMAL 120   /* 限时赛普通（秒） */
#define TIMED_HARD   60    /* 限时赛困难（秒） */

/* ===================================================
 *  食物参数
 * =================================================== */
#define BLUE_BASE     10    /* 蓝色食物基础分 */
#define RED_BASE      50    /* 红色食物最高分 */
#define RED_MIN       5     /* 红色食物最低分 */
#define RED_TIMER_MAX 10    /* 红色食物存活秒数 */
#define RED_TRIGGER   3     /* 累计吃N个蓝色后触发红色 */

/* ===================================================
 *  数据结构
 * =================================================== */
typedef struct {
    int x, y;
} Position;

typedef struct {
    Position body[MAX_LEN];
    int      len;
    int      dir;             /* 玩家预设方向 */
    int      lastDir;         /* 实际最近移动方向（防180度自杀）*/
} Snake;

/* AI 蛇 */
#define MAX_AI          3      /* 场上最大AI蛇数（降低AI压力） */
#define MOVING_OBS_MAX  5      /* 移动障碍上限 */
#define AI_SPAWN_MIN  18.0f  /* AI生成最短间隔（秒） */
#define AI_SPAWN_MAX  30.0f  /* AI生成最长间隔（秒） */
#define AI_MIN_LEN    2      /* AI初始最小长度 */
#define AI_MAX_LEN    4      /* AI初始最大长度 */
#define AI_SPEED      220    /* AI帧间隔ms（数值越大越慢）*/

typedef struct {
    Position body[MAX_LEN];
    int      len;
    int      dir;
    int      lastDir;
} AISnake;

#define MAX_FLOAT_TEXTS 16
typedef struct {
    float  x, y;
    float  life;
    TCHAR  text[16];
    COLORREF color;
} FloatText;

typedef struct {
    int      snakeColor;         /* SNAKE_COLOR_* */
    int      mapSize;            /* 16 / 18 / 20 */
    int      itemMode;           /* GAMEPLAY_CLASSIC / GAMEPLAY_ITEM */
    int      aiEnabled;          /* 0=关闭，1=开启 */

    /* 菜单可持久化自定义外观 */
    char     menuBgImagePath[MAX_PATH];
    char     menuMusicPath[MAX_PATH];
} GameConfig;

typedef struct {
    int        grid[GRID_SIZE][GRID_SIZE];

    /* --- 蛇 --- */
    Snake      snake;          /* P1（单人/双人）*/
    Snake      snake2;         /* P2（仅双人模式）*/

    /* --- 食物 --- */
    Position   blueFood;
    Position   redFood;
    int        hasRed;
    float      redTimer;
    int        blueCount;      /* 累计吃蓝色数（触发红色用）*/

    /* --- 计分 --- */
    int        score;          /* P1 / 单人得分 */
    int        score2;         /* P2 得分（仅双人）*/
    int        highScore;      /* 历史最高分（单人）*/

    /* --- 难度 --- */
    int        difficulty;     /* 0=简单 1=普通 2=困难 */
    int        speed;          /* P1 帧间隔ms */
    int        speed2;         /* P2 帧间隔ms（双人模式）*/
    int        baseSpeed;      /* P1 原始速度 */
    int        baseSpeed2;     /* P2 原始速度 */
    int        mult;           /* 得分系数x10 */

    /* --- 障碍 --- */
    int        obsCount;
    Position   obs[MAX_OBS];

    /* --- 配置 --- */
    GameConfig config;         /* 从record.txt读取的设置 */
    int        mapSize;        /* 本局有效地图边长 */

    /* --- 模式 / 状态机 --- */
    int        gameMode;       /* MODE_SINGLE / MODE_DUAL */
    int        winner;         /* 双人模式胜负: WIN_* */
    int        gameState;
    DWORD      deadTick;       /* 死亡时刻tick（用于死亡延迟）*/
    float      matchTimer;     /* 限时赛剩余秒数 */
    DWORD      lastTickP1;     /* P1 上次移动时刻 */
    DWORD      lastTickP2;     /* P2 上次移动时刻 */

    /* --- 道具系统 --- */
    int        itemOnField;    /* 场上道具类型（ITEM_NONE=无）*/
    Position   itemPos;        /* 道具位置 */
    float      itemLife;       /* 道具剩余存活时间 */
    float      itemSpawnCD;    /* 下次生成冷却 */

    /* P1 效果 */
    unsigned   p1Effects;      /* 位掩码 */
    float      p1Timers[NUM_EFFECTS];  /* 各效果剩余时间 */
    int        p1Shields;      /* 护盾层数（可叠加） */

    /* P2 效果 */
    unsigned   p2Effects;
    float      p2Timers[NUM_EFFECTS];
    int        p2Shields;

    /* --- AI 蛇 --- */
    AISnake    ai[MAX_AI];
    int        aiCount;        /* 场上AI数量 */
    float      aiSpawnCD;      /* AI生成冷却 */
    float      aiTickTimer;    /* AI移动计时器 */

    /* --- 连击系统（P1/P2独立） --- */
    float      comboCD;      /* P1：与单人/共享逻辑兼容 */
    int        comboCount;   /* P1：当前连击 */
    int        maxCombo;     /* P1：本局最高连击 */

    float      comboCD2;     /* P2：双人模式独立连击 */
    int        comboCount2;  /* P2：当前连击 */
    int        maxCombo2;    /* P2：本局最高连击 */

    /* --- 难度递增 --- */
    float      elapsedTime;
    float      diffFactor;

    /* --- 飘字 --- */
    FloatText  floatTexts[MAX_FLOAT_TEXTS];

    /* --- 移动障碍 --- */
    Position   obsMoving[MOVING_OBS_MAX];
    int        obsMovingDir[MOVING_OBS_MAX];
    int        obsMovingCount;
    float      obsMovingSpawnCD;
    float      obsMovingTimer;

    /* --- 成就系统 --- */
    unsigned   achUnlocked;
    unsigned   achNewUnlock;
    int        achBlueThisGame;
    int        achBlueTotal;
    int        achAIKills;
    int        achShieldBlocks;
} GameState;

/* ===================================================
 *  ===== 第二部分：核心工具函数 =====
 *  - 对外可见的规范化工具，用于输入参数的边界保护
 *  - 与核心 API 配套使用，保证跨模块一致性
 * =================================================== */
static inline int normalizeSnakeColor(int color)
{
    if (color >= SNAKE_COLOR_GREEN && color <= SNAKE_COLOR_PURPLE) return color;
    return SNAKE_COLOR_GREEN;
}

static inline int normalizeMapSize(int mapSize)
{
    if (mapSize == MAP_SIZE_SMALL || mapSize == MAP_SIZE_MEDIUM || mapSize == MAP_SIZE_LARGE)
        return mapSize;
    return MAP_SIZE_LARGE;
}

static inline int normalizeItemMode(int itemMode)
{
    return (itemMode == GAMEPLAY_CLASSIC) ? GAMEPLAY_CLASSIC : GAMEPLAY_ITEM;
}

static inline int normalizeAiEnabled(int aiEnabled)
{
    return aiEnabled ? 1 : 0;
}

/* ===================================================
 *  ===== 第三部分：核心游戏逻辑 API =====
 *  - 与主循环直接交互的核心控制接口
 * =================================================== */
void gameInit(GameState *g, int difficulty, const GameConfig *cfg);
void gameInitDual(GameState *g, int difficulty, int mode, const GameConfig *cfg);
int  gameMove(GameState *g);
int  gameMoveDual(GameState *g, int moveP1, int moveP2);
void gameSetDir(Snake *s, int dir);
void gameSetBoost(GameState *g, int p1Boost, int p2Boost);
void gameApplySpeed(GameState *g, int p1Boost, int p2Boost);
int  gameUpdateTimer(GameState *g, float dt);

/* ===================================================
 *  ===== 第四部分：食物子系统 =====
 * =================================================== */
void gamePlaceBlueFood(GameState *g);
void gamePlaceRedFood(GameState *g);
void gameUpdateRedTimer(GameState *g, float dt);
int  calcRedScore(const GameState *g);

/* ===================================================
 *  ===== 第五部分：道具子系统 =====
 * =================================================== */
#define MAGNET_RANGE 3

void itemUpdate(GameState *g, float dt);
void itemCollect(GameState *g, int player, int itemType);
int  itemHasEffect(const GameState *g, int player, int eff);
float itemTimer(const GameState *g, int player, int eff);
int  itemShieldCount(const GameState *g, int player);
int  itemIsNegative(int itemType);
void itemRemove(GameState *g);
int  itemConsumeShield(GameState *g, int player);

/* ===================================================
 *  ===== 第六部分：AI 子系统 =====
 * =================================================== */
void aiInit(GameState *g);
void aiUpdate(GameState *g, float dt);
void aiMarkAll(GameState *g);
void aiKill(GameState *g, int index);
int  aiScoreReward(int aiLen, int mult);

/* ===================================================
 *  ===== 第七部分：移动障碍子系统 =====
 * =================================================== */
#define MOVING_OBS_SPAWN  20.0f

void movingObsInit(GameState *g);
void movingObsUpdate(GameState *g, float dt);

/* ===================================================
 *  ===== 第八部分：连击子系统 =====
 * =================================================== */
#define COMBO_WINDOW 3.0f

void comboUpdate(GameState *g, float dt);
void comboUpdateForPlayer(GameState *g, float dt, int player);
int  comboOnEat(GameState *g, int baseScore);
int  comboOnEatForPlayer(GameState *g, int baseScore, int player);
int  comboCount(const GameState *g);
int  comboCountForPlayer(const GameState *g, int player);
int  comboMax(const GameState *g);
int  comboMaxForPlayer(const GameState *g, int player);

/* ===================================================
 *  ===== 第九部分：飘字子系统 =====
 * =================================================== */
void floatTextAdd(GameState *g, float x, float y,
                  LPCTSTR text, COLORREF color, float life);
void floatTextUpdate(GameState *g, float dt);
void floatTextClearAll(GameState *g);

/* ===================================================
 *  ===== 第十部分：成就子系统 =====
 * =================================================== */
#define ACH_MAX 10

/* 成就 ID */
#define ACH_FIRST100    0   /* 首次得分>100 */
#define ACH_BLUE100     1   /* 累计吃100个蓝食物 */
#define ACH_BLUE20      2   /* 单局吃20个蓝食物 */
#define ACH_COMBO5      3   /* 达成5连击 */
#define ACH_KILLAI10    4   /* 累计杀死10条AI */
#define ACH_SHIELD10    5   /* 累计用护盾挡10次 */
#define ACH_SPEED200    6   /* 60秒内得200分 */
#define ACH_SURVIVE180  7   /* 生存模式活过180秒 */
#define ACH_MIRRORWIN   8   /* 镜像对决中获胜 */
#define ACH_RED50       9   /* 吃到满分红食物(50分) */

unsigned achLoad(void);
void achLoadState(int *blueTotal, int *aiKills, int *shieldBlocks);
void achSaveState(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);
int  achUnlock(GameState *g, int achId);
int  achCheckAll(GameState *g, int gameMode);
LPCTSTR achName(int achId);
LPCTSTR achDesc(int achId);
void achOnEatBlue(GameState *g);
void achOnKillAI(GameState *g);
void achOnShieldBlock(GameState *g);

#endif /* SNAKE_LOGIC_H */
