#include <windows.h>
#include <easyx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "snake_logic.h"
#include "snake_services.h"


/* ============================================================
 *  snake.cpp - 合并版：全部模块归并实现（教学分块版）
 *
 *  本文件整合自原始项目中的 11 个模块文件：
 *  snake.cpp / food.cpp / item.cpp / ai.cpp / movingobs.cpp /
 *  floattext.cpp / combo.cpp / achievement.cpp / record.cpp /
 *  sound.cpp / input.cpp / gfx.cpp / main.cpp。
 *
 *  为便于 3 位同学分别讲解，按负责范围划分为 3 个板块：
 *  1) 板块一：游戏玩法核心（食物/蛇/道具/AI/移动障碍/连击/成就）
 *  2) 板块二：系统服务层（持久化/音效/输入）
 *  3) 板块三：渲染与主流程（视图 + 主循环）
 *
 *  仅为满足作业“1 个 cpp + 2 个 h”的合并版，
 *  模块功能、流程与现有实现保持一致。
 * ============================================================ */



/* ============================================================
 *  ===== 板块一：游戏玩法核心 =====
 *  负责：核心玩法状态与规则实现，包含食物、移动、道具、AI、障碍、连击、成就等。
 * ------------------------------------------------------------
 *  从 food.cpp 合并
 * ============================================================ */
/* ============================================================
 *  food.cpp - 食物子系统实现
 *
 *  从 snake.cpp 提取的食物放置、计时与计分逻辑。
 * ============================================================ */


/* ===================================================
 *  内部辅助
 * =================================================== */

/* 在网格中找一个空格子（避开已有食物） */
static int randomEmptyCell(GameState *g, Position *p, int forbidX, int forbidY)
{
    int tries = g->mapSize * g->mapSize * 2;
    int x, y;

    if (!g || !p || g->mapSize <= 0) return 0;

    while (tries-- > 0) {
        x = rand() % g->mapSize;
        y = rand() % g->mapSize;
        if (g->grid[y][x] == CELL_EMPTY &&
            !(x == forbidX && y == forbidY)) {
            p->x = x;
            p->y = y;
            return 1;
        }
    }

    for (y = 0; y < g->mapSize; y++) {
        for (x = 0; x < g->mapSize; x++) {
            if (g->grid[y][x] == CELL_EMPTY && !(x == forbidX && y == forbidY)) {
                p->x = x;
                p->y = y;
                return 1;
            }
        }
    }

    return 0;
}

/* ===================================================
 *  食物逻辑函数
 * =================================================== */

/* 放置蓝色食物 — 避开红色食物位置 */
void gamePlaceBlueFood(GameState *g)
{
    Position p;
    int tries = g->mapSize * g->mapSize * 2;
    while (tries-- > 0) {
        int x = rand() % g->mapSize;
        int y = rand() % g->mapSize;
        if (g->grid[y][x] == CELL_EMPTY &&
            !(g->hasRed && x == g->redFood.x && y == g->redFood.y)) {
            g->blueFood.x = x;
            g->blueFood.y = y;
            g->grid[y][x] = CELL_BLUE;
            return;
        }
    }

    if (randomEmptyCell(g, &p, -1, -1)) {
        g->blueFood = p;
        g->grid[p.y][p.x] = CELL_BLUE;
    }
}

/* 放置红色食物 — 避开蓝色食物位置 */
void gamePlaceRedFood(GameState *g)
{
    Position p;
    int tries = g->mapSize * g->mapSize * 2;
    while (tries-- > 0) {
        int x = rand() % g->mapSize;
        int y = rand() % g->mapSize;
        if (g->grid[y][x] == CELL_EMPTY && !(x == g->blueFood.x && y == g->blueFood.y)) {
            g->redFood.x = x;
            g->redFood.y = y;
            g->grid[y][x] = CELL_RED;
            g->hasRed = 1;
            g->redTimer = (float)RED_TIMER_MAX;
            return;
        }
    }

    if (randomEmptyCell(g, &p, g->blueFood.x, g->blueFood.y)) {
        g->redFood = p;
        g->grid[p.y][p.x] = CELL_RED;
        g->hasRed = 1;
        g->redTimer = (float)RED_TIMER_MAX;
    }
}

void gameUpdateRedTimer(GameState *g, float dt)
{
    if (!g->hasRed) return;
    g->redTimer -= dt;
    if (g->redTimer <= 0.0f) {
        if (g->grid[g->redFood.y][g->redFood.x] == CELL_RED)
            g->grid[g->redFood.y][g->redFood.x] = CELL_EMPTY;
        g->hasRed = 0;
        g->redTimer = 0.0f;
    }
}

int calcRedScore(const GameState *g)
{
    int score = (int)((float)RED_BASE * g->redTimer / (float)RED_TIMER_MAX);
    if (score < RED_MIN) score = RED_MIN;
    return score * g->mult / 10;
}


/* ============================================================
 *  从 snake.cpp 合并
 * ============================================================ */
/* ============================================================
 *  snake.cpp - 游戏模型层实现
 *
 *  把原先 snake.h 中的全部实现移动到本文件，形成
 *  逻辑模块与接口声明分离的结构。
 * ============================================================ */


/* ===================================================
 *  内部辅助函数
 * =================================================== */

/* 应用单人难度参数 */
static void applyDifficulty(GameState *g, int difficulty)
{
    g->difficulty = difficulty;
    if (difficulty == 0) {
        g->speed = EASY_SPEED;
        g->obsCount = EASY_OBS;
        g->mult = EASY_MULT;
    } else if (difficulty == 1) {
        g->speed = NORMAL_SPEED;
        g->obsCount = NORMAL_OBS;
        g->mult = NORMAL_MULT;
    } else {
        g->difficulty = 2;
        g->speed = HARD_SPEED;
        g->obsCount = HARD_OBS;
        g->mult = HARD_MULT;
    }
    g->baseSpeed = g->speed;
}

/* 应用双人难度参数 */
static void applyDualDifficulty(GameState *g, int difficulty)
{
    g->difficulty = difficulty;
    if (difficulty == 0) g->speed = DUAL_EASY_SPEED;
    else if (difficulty == 1) g->speed = DUAL_NORMAL_SPEED;
    else {
        g->difficulty = 2;
        g->speed = DUAL_HARD_SPEED;
    }
    g->obsCount = NORMAL_OBS;
    g->mult = EASY_MULT;
    g->baseSpeed = g->speed;
    g->speed2 = g->speed;
    g->baseSpeed2 = g->speed;
}

/* 初始化网格为全空（边界由坐标检查处理，不再占用内部格子） */
static void setupGrid(GameState *g)
{
    int x, y;
    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            g->grid[y][x] = CELL_EMPTY;
        }
    }
}

/* 随机放置障碍物（带失败回退，避免极端状态下死循环） */
static void placeObstacles(GameState *g)
{
    int i;
    for (i = 0; i < g->obsCount && i < MAX_OBS; i++) {
        int x, y, tries;
        int placed = 0;

        for (tries = 0; tries < g->mapSize * g->mapSize * 2; tries++) {
            x = rand() % g->mapSize;
            y = rand() % g->mapSize;
            if (g->grid[y][x] == CELL_EMPTY) {
                g->obs[i].x = x;
                g->obs[i].y = y;
                g->grid[y][x] = CELL_OBS;
                placed = 1;
                break;
            }
        }

        if (!placed) {
            int yy, xx;
            for (yy = 0; yy < g->mapSize && !placed; yy++) {
                for (xx = 0; xx < g->mapSize; xx++) {
                    if (g->grid[yy][xx] == CELL_EMPTY) {
                        g->obs[i].x = xx;
                        g->obs[i].y = yy;
                        g->grid[yy][xx] = CELL_OBS;
                        placed = 1;
                        break;
                    }
                }
            }
        }

        if (!placed) {
            break;
        }
    }
}

/* 将蛇身体标记到网格 */
static void markSnake(const Snake *s, int cell, int grid[GRID_SIZE][GRID_SIZE])
{
    int i;
    for (i = 0; i < s->len; i++)
        grid[s->body[i].y][s->body[i].x] = cell;
}

/* 计算蛇头下一步位置 */
static void nextPosition(const Snake *s, int *x, int *y)
{
    *x = s->body[0].x;
    *y = s->body[0].y;
    if (s->dir == DIR_UP) (*y)--;
    else if (s->dir == DIR_DOWN) (*y)++;
    else if (s->dir == DIR_LEFT) (*x)--;
    else if (s->dir == DIR_RIGHT) (*x)++;
}

/* 蛇前进一格 */
static void advanceSnake(Snake *s, int nx, int ny, int grow)
{
    int i;
    int newLen = s->len + (grow ? 1 : 0);
    if (newLen > MAX_LEN) newLen = MAX_LEN;
    for (i = newLen - 1; i > 0; i--)
        s->body[i] = s->body[i - 1];
    s->body[0].x = nx;
    s->body[0].y = ny;
    s->len = newLen;
    s->lastDir = s->dir;
}

static int samePos(Position p, int x, int y)
{
    return p.x == x && p.y == y;
}

static int cellAfterVirtualTailClear(int cell, int x, int y,
                                     Position tail1, int clearTail1,
                                     Position tail2, int clearTail2)
{
    if (clearTail1 && cell == CELL_SNAKE && samePos(tail1, x, y))
        return CELL_EMPTY;
    if (clearTail2 && cell == CELL_SNAKE2 && samePos(tail2, x, y))
        return CELL_EMPTY;
    return cell;
}

static int gameApplyMagnet(GameState *g, int player, int hx, int hy, int mult)
{
    int dx, dy;

    if (!g || !itemHasEffect(g, player, EFF_MAGNET)) {
        return 0;
    }

    for (dy = -MAGNET_RANGE; dy <= MAGNET_RANGE; dy++) {
        for (dx = -MAGNET_RANGE; dx <= MAGNET_RANGE; dx++) {
            int mx = hx + dx;
            int my = hy + dy;
            int dist = abs(dx) + abs(dy);
            if (dist > 0 && dist <= MAGNET_RANGE &&
                mx >= 0 && my >= 0 && mx < g->mapSize && my < g->mapSize &&
                g->grid[my][mx] == CELL_BLUE) {
                int base = (BLUE_BASE * g->mult / 10) * mult;
                int act;
                int comb;
                if (player == 1)
                    act = comboOnEatForPlayer(g, base, 1);
                else
                    act = comboOnEatForPlayer(g, base, 0);

                if (player == 1) {
                    g->score2 += act;
                } else {
                    g->score += act;
                }
                g->blueCount++;
                achOnEatBlue(g);
                soundPlayEatBlue();
                g->grid[my][mx] = CELL_EMPTY;
                gamePlaceBlueFood(g);
                if (g->blueCount % RED_TRIGGER == 0 && !g->hasRed)
                    gamePlaceRedFood(g);

                comb = comboCountForPlayer(g, player);
                {
                    TCHAR b[16];
                    _stprintf(b, _T("+%d"), act);
                    floatTextAdd(g, (float)mx, (float)my, b,
                        comb >= 3 ? RGB(255,200,0) : RGB(230,230,230), 1.2f);
                }

                achCheckAll(g, g->gameMode);
                return 1;
            }
        }
    }

    return 0;
}

/* ===================================================
 *  游戏逻辑函数
 * =================================================== */

void gameInit(GameState *g, int difficulty, const GameConfig *cfg)
{
    int highScore = g->highScore;
    unsigned achUnlocked = g->achUnlocked;
    int achBlueTotal = g->achBlueTotal;
    int achAIKills = g->achAIKills;
    int achShieldBlocks = g->achShieldBlocks;
    GameConfig savedCfg;

    savedCfg.snakeColor = cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN;
    savedCfg.mapSize = cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE;
    savedCfg.itemMode = cfg ? normalizeItemMode(cfg->itemMode) : GAMEPLAY_ITEM;
    savedCfg.aiEnabled = cfg ? normalizeAiEnabled(cfg->aiEnabled) : 1;
    if (cfg) {
        snprintf(savedCfg.menuBgImagePath, sizeof(savedCfg.menuBgImagePath), "%s", cfg->menuBgImagePath);
        snprintf(savedCfg.menuMusicPath, sizeof(savedCfg.menuMusicPath), "%s", cfg->menuMusicPath);
    } else {
        savedCfg.menuBgImagePath[0] = '\0';
        savedCfg.menuMusicPath[0] = '\0';
    }


    memset(g, 0, sizeof(*g));
    g->highScore = highScore;
    g->achUnlocked = achUnlocked;
    g->achBlueTotal = achBlueTotal;
    g->achAIKills = achAIKills;
    g->achShieldBlocks = achShieldBlocks;
    g->config = savedCfg;
    g->mapSize = savedCfg.mapSize;
    g->gameMode = MODE_SINGLE;
    g->gameState = STATE_PLAYING;
    applyDifficulty(g, difficulty);
    setupGrid(g);

    /* 出生点行/列随机：避免每局都在地图正中央 */
    {
        int rx, ry, tries = 0;
        const int minBodyY = 1;
        const int maxBodyY = g->mapSize - 3;
        do {
            rx = g->mapSize / 2 + (rand() % 5) - 2;  /* mapSize/2 + {-2,-1,0,1,2} */
            ry = g->mapSize / 2 + (rand() % 5) - 2;
            tries++;
        } while (tries < 16 &&
                 (rx < 1 || ry < 1 || rx > g->mapSize - 2 || ry > maxBodyY));

        if (rx < 1) rx = 1;
        if (ry < minBodyY) ry = minBodyY;
        if (rx > g->mapSize - 2) rx = g->mapSize - 2;
        if (ry > maxBodyY) ry = maxBodyY;

        /* 让蛇头朝上，身子在下方（不影响随机性，只影响渲染） */
        g->snake.body[0].x = rx;
        g->snake.body[0].y = ry;
        g->snake.body[1].x = rx;
        g->snake.body[1].y = ry + 1;
        g->snake.body[2].x = rx;
        g->snake.body[2].y = ry + 2;

        /* 头部附近清理：避免出生点压在障碍/食物/墙上 */
        {
            int bx, by;
            for (by = ry; by <= ry + 2; by++) {
                if (by < 0 || by >= g->mapSize)
                    continue;
                for (bx = rx; bx <= rx; bx++) {
                    if (bx < 0 || bx >= g->mapSize)
                        continue;
                    if (g->grid[by][bx] != CELL_OBS)
                        g->grid[by][bx] = CELL_SNAKE;
                }
            }
        }
    }
    g->snake.len = 3;
    g->snake.dir = DIR_UP;
    g->snake.lastDir = DIR_UP;
    markSnake(&g->snake, CELL_SNAKE, g->grid);

    placeObstacles(g);
    if (g->config.aiEnabled) {
        aiInit(g);
        aiMarkAll(g);
    } else {
        g->aiCount = 0;
    }
    g->diffFactor = 1.0f;
    movingObsInit(g);
    gamePlaceBlueFood(g);
}

void gameInitDual(GameState *g, int difficulty, int mode, const GameConfig *cfg)
{
    int highScore = g->highScore;
    unsigned achUnlocked = g->achUnlocked;
    int achBlueTotal = g->achBlueTotal;
    int achAIKills = g->achAIKills;
    int achShieldBlocks = g->achShieldBlocks;
    GameConfig savedCfg;
    int y, x1, x2;

    savedCfg.snakeColor = cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN;
    savedCfg.mapSize = cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE;
    savedCfg.itemMode = cfg ? normalizeItemMode(cfg->itemMode) : GAMEPLAY_ITEM;
    savedCfg.aiEnabled = cfg ? normalizeAiEnabled(cfg->aiEnabled) : 1;
    if (cfg) {
        snprintf(savedCfg.menuBgImagePath, sizeof(savedCfg.menuBgImagePath), "%s", cfg->menuBgImagePath);
        snprintf(savedCfg.menuMusicPath, sizeof(savedCfg.menuMusicPath), "%s", cfg->menuMusicPath);
    } else {
        savedCfg.menuBgImagePath[0] = '\0';
        savedCfg.menuMusicPath[0] = '\0';
    }


    memset(g, 0, sizeof(*g));
    g->highScore = highScore;
    g->achUnlocked = achUnlocked;
    g->achBlueTotal = achBlueTotal;
    g->achAIKills = achAIKills;
    g->achShieldBlocks = achShieldBlocks;
    g->config = savedCfg;
    g->mapSize = savedCfg.mapSize;
    g->gameMode = mode;
    g->gameState = STATE_PLAYING;
    g->winner = WIN_NONE;

    /* 多人模式默认关闭 AI 敌人（避免和玩家蛇互相干扰） */
    if (mode == MODE_DUAL || mode == MODE_DUAL_TIMED || mode == MODE_DUAL_MIRROR)
        g->config.aiEnabled = 0;

    /* 限时赛设置初始时长 */
    if (mode == MODE_DUAL_TIMED) {
        if (difficulty == 0) g->matchTimer = (float)TIMED_EASY;
        else if (difficulty == 1) g->matchTimer = (float)TIMED_NORMAL;
        else g->matchTimer = (float)TIMED_HARD;
    }

    applyDualDifficulty(g, difficulty);
    setupGrid(g);

    y = g->mapSize / 2;
    /* 出生点行随机，避免双方开局位置固定 */
    {
        int dy = (rand() % 3) - 1;  /* -1/0/+1 */
        int y2 = y + dy;
        if (y2 < 1) y2 = 1;
        if (y2 > g->mapSize - 2) y2 = g->mapSize - 2;
        y = y2;
    }
    x1 = g->mapSize / 4;
    x2 = g->mapSize * 3 / 4;
    /* 在各自 1/4 象限内再做小范围随机 */
    x1 += (rand() % 3) - 1;
    x2 += (rand() % 3) - 1;
    if (x1 < 1) x1 = 1;
    if (x2 > g->mapSize - 2) x2 = g->mapSize - 2;
    if (x1 > g->mapSize / 2 - 4) x1 = g->mapSize / 2 - 4;
    if (x2 < g->mapSize / 2 + 3) x2 = g->mapSize / 2 + 3;

    g->snake.len = 3;
    g->snake.dir = DIR_RIGHT;
    g->snake.lastDir = DIR_RIGHT;
    g->snake.body[0].x = x1; g->snake.body[0].y = y;
    g->snake.body[1].x = x1 - 1; g->snake.body[1].y = y;
    g->snake.body[2].x = x1 - 2; g->snake.body[2].y = y;

    g->snake2.len = 3;
    g->snake2.dir = DIR_LEFT;
    g->snake2.lastDir = DIR_LEFT;
    g->snake2.body[0].x = x2; g->snake2.body[0].y = y;
    g->snake2.body[1].x = x2 + 1; g->snake2.body[1].y = y;
    g->snake2.body[2].x = x2 + 2; g->snake2.body[2].y = y;

    markSnake(&g->snake, CELL_SNAKE, g->grid);
    markSnake(&g->snake2, CELL_SNAKE2, g->grid);
    placeObstacles(g);
    if (g->config.aiEnabled) {
        aiInit(g);
        aiMarkAll(g);
    } else {
        g->aiCount = 0;
    }
    g->diffFactor = 1.0f;
    movingObsInit(g);
    gamePlaceBlueFood(g);
}

/* 单人移动：0=死亡 1=正常 2=吃蓝 3=吃红 */
int gameMove(GameState *g)
{
    int nx, ny, cell, eatBlue, eatRed, grow;
    int hasGhost = itemHasEffect(g, 0, EFF_GHOST);
    int hasDouble = itemHasEffect(g, 0, EFF_DOUBLE);
    int mult = hasDouble ? 2 : 1;
    Position tail = g->snake.body[g->snake.len - 1];

    nextPosition(&g->snake, &nx, &ny);
    if (nx < 0 || ny < 0 || nx >= g->mapSize || ny >= g->mapSize) {
        if (itemConsumeShield(g, 0)) return 1;
        return 0;
    }

    cell = g->grid[ny][nx];

    /* 道具收集：仅在确认能移动时才生效，这里只记录 */
    {
        int itemAtTarget = (g->config.itemMode == GAMEPLAY_ITEM && cell == CELL_ITEM);
        if (itemAtTarget) {
            cell = CELL_EMPTY;  /* 虚拟清除道具格，后续碰撞检测不会误判 */
        }
    }

    eatBlue = (cell == CELL_BLUE);
    eatRed = (cell == CELL_RED && g->hasRed);
    grow = eatBlue || eatRed;

    /* 先清尾再判碰撞（与双人模式保持一致），避免"移动到本回合会移走的尾巴"被误判死亡 */
    if (!grow) {
        if (g->grid[tail.y][tail.x] == CELL_SNAKE)
            g->grid[tail.y][tail.x] = CELL_EMPTY;
    }

    /* 重算目标格子的语义（虚拟清尾后） */
    cell = cellAfterVirtualTailClear(cell, nx, ny,
                                    tail, !grow,
                                    tail, 0);

    /* 碰撞判定（穿墙效果允许穿过自己的身体） */
    if (cell == CELL_WALL || cell == CELL_OBS || cell == CELL_AI || cell == CELL_SNAKE2) {
        if (itemConsumeShield(g, 0)) {
            /* 护盾生效：蛇不移动，恢复尾巴 */
            if (!grow) g->grid[tail.y][tail.x] = CELL_SNAKE;
            return 1;
        }
        soundPlayDeath();
        return 0;
    }
    /* 自撞判定：尾巴已清除，只需检查剩余身体（ghost 效果可穿透） */
    if (cell == CELL_SNAKE && !hasGhost) {
        if (itemConsumeShield(g, 0)) {
            if (!grow) g->grid[tail.y][tail.x] = CELL_SNAKE;
            return 1;
        }
        soundPlayDeath();
        return 0;
    }

    /* 确认移动：头部格子若为道具，真正收集 */
    if (g->config.itemMode == GAMEPLAY_ITEM && g->grid[ny][nx] == CELL_ITEM) {
        itemCollect(g, 0, g->itemOnField);
    }

    advanceSnake(&g->snake, nx, ny, grow);
    g->grid[ny][nx] = CELL_SNAKE;

    if (eatBlue) {
        int base = (BLUE_BASE * g->mult / 10) * mult;
        int act = comboOnEat(g, base);
        g->score += act;
        g->blueCount++;
        achOnEatBlue(g);
        {
            TCHAR buf[16];
            _stprintf(buf, _T("+%d"), act);
            floatTextAdd(g, (float)nx, (float)ny, buf,
                comboCount(g) >= 3 ? RGB(255,200,0) : RGB(230,230,230), 1.2f);
        }
        soundPlayEatBlue();
        gamePlaceBlueFood(g);
        if (g->blueCount % RED_TRIGGER == 0 && !g->hasRed)
            gamePlaceRedFood(g);
        achCheckAll(g, g->gameMode);
        return 2;
    }
    if (eatRed) {
        int base = calcRedScore(g) * mult;
        int act = comboOnEat(g, base);
        g->score += act;
        soundPlayEatRed();
        if (base >= RED_BASE * mult) achUnlock(g, ACH_RED50);
        g->hasRed = 0;
        {
            TCHAR buf[16];
            _stprintf(buf, _T("+%d"), act);
            floatTextAdd(g, (float)nx, (float)ny, buf,
                RGB(230,70,70), 1.2f);
        }
        achCheckAll(g, g->gameMode);
        return 3;
    }

    /* 磁铁吸附：移动后在头部3格内找蓝色食物 */
    if (gameApplyMagnet(g, 0, nx, ny, mult))
        return 2;

    return 1;
}

/* 双人移动：0=有人死(看winner) 1=正常
 * moveP1/moveP2 指示本次哪些蛇需要移动，支持各自独立速度 */
int gameMoveDual(GameState *g, int moveP1, int moveP2)
{
    int nx1, ny1, nx2, ny2, c1, c2;
    int dead1 = 0, dead2 = 0;
    int eat1 = 0, eat2 = 0, eatRed1 = 0, eatRed2 = 0;
    int item1 = 0, item2 = 0;       /* 目标格是否有道具（延迟到碰撞通过后再收集） */
    int peek1 = CELL_EMPTY, peek2 = CELL_EMPTY;
    int ghost1 = itemHasEffect(g, 0, EFF_GHOST);
    int ghost2 = itemHasEffect(g, 1, EFF_GHOST);
    int mult1 = itemHasEffect(g, 0, EFF_DOUBLE) ? 2 : 1;
    int mult2 = itemHasEffect(g, 1, EFF_DOUBLE) ? 2 : 1;
    Position oldHead1 = g->snake.body[0];
    Position oldHead2 = g->snake2.body[0];
    Position tail1 = g->snake.body[g->snake.len - 1];
    Position tail2 = g->snake2.body[g->snake2.len - 1];

    /* 冻结效果：被冻结者不能移动 */
    if (itemHasEffect(g, 0, EFF_FROZEN)) moveP1 = 0;
    if (itemHasEffect(g, 1, EFF_FROZEN)) moveP2 = 0;

    /* 只计算需要移动的蛇的下一步位置（不修改 grid，避免护盾回滚时状态不一致） */
    if (moveP1) {
        nextPosition(&g->snake, &nx1, &ny1);
        peek1 = (nx1 < 0 || ny1 < 0 || nx1 >= g->mapSize || ny1 >= g->mapSize)
                ? CELL_WALL : g->grid[ny1][nx1];
        /* 道具标记：仅记录，不在此处收集（否则护盾取消移动时 grid 已被修改） */
        if (g->config.itemMode == GAMEPLAY_ITEM && peek1 == CELL_ITEM) {
            item1 = 1;
            peek1 = CELL_EMPTY;
        }
        eat1 = (peek1 == CELL_BLUE);
        eatRed1 = (peek1 == CELL_RED && g->hasRed);
    } else {
        nx1 = oldHead1.x; ny1 = oldHead1.y;
    }

    if (moveP2) {
        nextPosition(&g->snake2, &nx2, &ny2);
        peek2 = (nx2 < 0 || ny2 < 0 || nx2 >= g->mapSize || ny2 >= g->mapSize)
                ? CELL_WALL : g->grid[ny2][nx2];
        if (g->config.itemMode == GAMEPLAY_ITEM && peek2 == CELL_ITEM) {
            item2 = 1;
            peek2 = CELL_EMPTY;
        }
        eat2 = (peek2 == CELL_BLUE);
        eatRed2 = (peek2 == CELL_RED && g->hasRed);
    } else {
        nx2 = oldHead2.x; ny2 = oldHead2.y;
    }

    /* 头对头碰撞（仅两蛇同时移动时检查） */
    if (moveP1 && moveP2) {
        if (nx1 == nx2 && ny1 == ny2) dead1 = dead2 = 1;
        if (nx1 == oldHead2.x && ny1 == oldHead2.y && nx2 == oldHead1.x && ny2 == oldHead1.y)
            dead1 = dead2 = 1;
    }

    /* 以”虚拟清尾”重新检查目标格，避免护盾取消移动时破坏 grid */
    if (moveP1) {
        c1 = (nx1 < 0 || ny1 < 0 || nx1 >= g->mapSize || ny1 >= g->mapSize)
           ? CELL_WALL : g->grid[ny1][nx1];
        c1 = cellAfterVirtualTailClear(c1, nx1, ny1,
            tail1, moveP1 && !(eat1 || eatRed1),
            tail2, moveP2 && !(eat2 || eatRed2));
        if (c1 == CELL_WALL || c1 == CELL_OBS || c1 == CELL_AI || c1 == CELL_SNAKE2) {
            if (itemConsumeShield(g, 0)) moveP1 = 0; else dead1 = 1;
        } else if (c1 == CELL_SNAKE && !ghost1) {
            if (itemConsumeShield(g, 0)) moveP1 = 0; else dead1 = 1;
        }
    }
    if (moveP2) {
        c2 = (nx2 < 0 || ny2 < 0 || nx2 >= g->mapSize || ny2 >= g->mapSize)
           ? CELL_WALL : g->grid[ny2][nx2];
        c2 = cellAfterVirtualTailClear(c2, nx2, ny2,
            tail1, moveP1 && !(eat1 || eatRed1),
            tail2, moveP2 && !(eat2 || eatRed2));
        if (c2 == CELL_WALL || c2 == CELL_OBS || c2 == CELL_AI || c2 == CELL_SNAKE) {
            if (itemConsumeShield(g, 1)) moveP2 = 0; else dead2 = 1;
        } else if (c2 == CELL_SNAKE2 && !ghost2) {
            if (itemConsumeShield(g, 1)) moveP2 = 0; else dead2 = 1;
        }
    }

    /* 碰撞通过后才真正收集道具（此时蛇确定会移动到目标格） */
    if (moveP1 && item1) itemCollect(g, 0, g->itemOnField);
    if (moveP2 && item2) itemCollect(g, 1, g->itemOnField);

    if (dead1 || dead2) {
        soundPlayDeath();
        if (dead1 && dead2) g->winner = WIN_DRAW;
        else if (dead1) g->winner = WIN_P2;
        else g->winner = WIN_P1;
        return 0;
    }

    /* 清除蛇在网格上的旧位置 */
    if (moveP1) markSnake(&g->snake, CELL_EMPTY, g->grid);
    if (moveP2) markSnake(&g->snake2, CELL_EMPTY, g->grid);

    /* 蛇前进 */
    if (moveP1) advanceSnake(&g->snake, nx1, ny1, eat1 || eatRed1);
    if (moveP2) advanceSnake(&g->snake2, nx2, ny2, eat2 || eatRed2);

    /* 计分（双倍道具 + 连击） */
    if (eat1) {
        soundPlayEatBlue();
        int act = comboOnEatForPlayer(g, BLUE_BASE * mult1, 0);
        g->score += act;
        achOnEatBlue(g);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx1, (float)ny1, b,
              comboCountForPlayer(g, 0) >= 3 ? RGB(255,200,0) : RGB(230,230,230), 1.2f); }
    }
    if (eat2) {
        soundPlayEatBlue();
        int act = comboOnEatForPlayer(g, BLUE_BASE * mult2, 1);
        g->score2 += act;
        achOnEatBlue(g);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx2, (float)ny2, b,
              comboCountForPlayer(g, 1) >= 3 ? RGB(255,200,0) : RGB(230,230,230), 1.2f); }
    }
    if (moveP1 && !eat1 && !eatRed1) {
        gameApplyMagnet(g, 0, nx1, ny1, mult1);
    }
    if (moveP2 && !eat2 && !eatRed2) {
        gameApplyMagnet(g, 1, nx2, ny2, mult2);
    }

    if (eatRed1) {
        soundPlayEatRed();
        int base = calcRedScore(g) * mult1;
        int act = comboOnEatForPlayer(g, base, 0);
        g->score += act;
        g->hasRed = 0;
        if (base >= RED_BASE * mult1) achUnlock(g, ACH_RED50);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx1, (float)ny1, b, RGB(230,70,70), 1.2f); }
    }
    if (eatRed2) {
        soundPlayEatRed();
        int base = calcRedScore(g) * mult2;
        int act = comboOnEatForPlayer(g, base, 1);
        g->score2 += act;
        g->hasRed = 0;
        if (base >= RED_BASE * mult2) achUnlock(g, ACH_RED50);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx2, (float)ny2, b, RGB(230,70,70), 1.2f); }
    }
    achCheckAll(g, g->gameMode);

    /* 重新标记蛇在网格上的新位置 */
    if (moveP1) markSnake(&g->snake, CELL_SNAKE, g->grid);
    if (moveP2) markSnake(&g->snake2, CELL_SNAKE2, g->grid);

    /* 食物刷新 */
    if (eat1 || eat2) {
        g->blueCount++;
        gamePlaceBlueFood(g);
        if (g->blueCount % RED_TRIGGER == 0 && !g->hasRed)
            gamePlaceRedFood(g);
    }

    return 1;
}

void gameSetDir(Snake *s, int dir)
{
    if (!s) return;
    if (dir == DIR_UP && s->lastDir == DIR_DOWN) return;
    if (dir == DIR_DOWN && s->lastDir == DIR_UP) return;
    if (dir == DIR_LEFT && s->lastDir == DIR_RIGHT) return;
    if (dir == DIR_RIGHT && s->lastDir == DIR_LEFT) return;
    s->dir = dir;
}

void gameSetBoost(GameState *g, int p1Boost, int p2Boost)
{
    g->speed  = p1Boost ? g->baseSpeed  / 2 : g->baseSpeed;
    g->speed2 = p2Boost ? g->baseSpeed2 / 2 : g->baseSpeed2;
}

/* 综合道具效果和按键加速计算最终速度
 * 优先级：极速(×3) > 按键加速(×2) > 正常 > 减速(×1.5) */
void gameApplySpeed(GameState *g, int p1Boost, int p2Boost)
{
    /* P1 */
    if (itemHasEffect(g, 0, EFF_TURBO))
        g->speed = g->baseSpeed / 3;
    else if (p1Boost)
        g->speed = g->baseSpeed / 2;
    else if (itemHasEffect(g, 0, EFF_SLOW))
        g->speed = g->baseSpeed * 3 / 2;
    else
        g->speed = g->baseSpeed;

    /* P2 */
    if (itemHasEffect(g, 1, EFF_TURBO))
        g->speed2 = g->baseSpeed2 / 3;
    else if (p2Boost)
        g->speed2 = g->baseSpeed2 / 2;
    else if (itemHasEffect(g, 1, EFF_SLOW))
        g->speed2 = g->baseSpeed2 * 3 / 2;
    else
        g->speed2 = g->baseSpeed2;
}

/* 限时赛倒计时：返回0=时间到 */
int gameUpdateTimer(GameState *g, float dt)
{
    g->matchTimer -= dt;
    if (g->matchTimer <= 0.0f) {
        g->matchTimer = 0.0f;
        return 0;
    }
    return 1;
}


/* ============================================================
 *  从 item.cpp 合并
 * ============================================================ */
/* ============================================================
 *  item.cpp - 道具系统实现
 *
 *  管理道具生成、收集、效果定时与过期。
 * ============================================================ */


/* 内部常量 */
#define ITEM_SPAWN_MIN  8.0f   /* 生成最短间隔 */
#define ITEM_SPAWN_MAX  15.0f  /* 生成最长间隔 */
#define ITEM_LIFE       8.0f   /* 道具场上存活时间 */
#define TURBO_DURATION  5.0f
#define SHIELD_DURATION 15.0f
#define SLOW_DURATION   4.0f
#define MAGNET_DURATION 6.0f
#define FREEZE_DURATION 3.0f
#define GHOST_DURATION  6.0f
#define DOUBLE_DURATION 10.0f
#define MAX_SHIELDS     3

/* ===================================================
 *  内部辅助
 * =================================================== */

static int itemFeatureEnabled(const GameState *g)
{
    return g && normalizeItemMode(g->config.itemMode) == GAMEPLAY_ITEM;
}

static void clearActiveItemState(GameState *g)
{
    if (!g) return;

    g->p1Effects = 0;
    g->p2Effects = 0;
    g->p1Shields = 0;
    g->p2Shields = 0;
    itemRemove(g);
    g->itemOnField = ITEM_NONE;
    g->itemLife = 0.0f;
    g->itemSpawnCD = 0.0f;
    memset(g->p1Timers, 0, sizeof(g->p1Timers));
    memset(g->p2Timers, 0, sizeof(g->p2Timers));
}

/* 随机生成新道具 */
static void spawnItem(GameState *g)
{
    int type, tries, x, y;
    int allowFreeze = (g->gameMode == MODE_DUAL ||
                       g->gameMode == MODE_DUAL_TIMED ||
                       g->gameMode == MODE_DUAL_MIRROR);

    /* 随机选取类型（非双人模式排除冻结） */
    type = 1 + rand() % ITEM_MAX;
    if (type == ITEM_FREEZE && !allowFreeze)
        type = ITEM_TURBO;  /* 单人模式不生成冻结，用极速替代 */

    /* 随机找空格 */
    tries = g->mapSize * g->mapSize * 2;
    while (tries-- > 0) {
        x = rand() % g->mapSize;
        y = rand() % g->mapSize;
        if (g->grid[y][x] == CELL_EMPTY) {
            g->itemPos.x = x;
            g->itemPos.y = y;
            g->itemOnField = type;
            g->itemLife = ITEM_LIFE;
            g->grid[y][x] = CELL_ITEM;
            return;
        }
    }
    /* 没找到空格就不生成 */
    g->itemOnField = ITEM_NONE;
    g->itemLife = 0.0f;
}

/* 设置或重置某个效果的位掩码和定时器 */
static void setEffect(GameState *g, int player, int eff, float duration)
{
    unsigned *ef = (player == 0) ? &g->p1Effects : &g->p2Effects;
    float *tm = (player == 0) ? g->p1Timers : g->p2Timers;
    *ef |= (1u << eff);
    tm[eff] = duration;
}

/* ===================================================
 *  公开函数
 * =================================================== */

void itemUpdate(GameState *g, float dt)
{
    unsigned *ef[2];
    float *tm[2];
    int *shields[2];
    int p;

    if (!itemFeatureEnabled(g)) {
        clearActiveItemState(g);
        return;
    }

    ef[0] = &g->p1Effects; tm[0] = g->p1Timers; shields[0] = &g->p1Shields;
    ef[1] = &g->p2Effects; tm[1] = g->p2Timers; shields[1] = &g->p2Shields;

    /* --- 道具生成逻辑 --- */
    if (g->itemOnField == ITEM_NONE) {
        g->itemSpawnCD -= dt;
        if (g->itemSpawnCD <= 0.0f) {
            spawnItem(g);
            g->itemSpawnCD = ITEM_SPAWN_MIN +
                (float)(rand() % (int)((ITEM_SPAWN_MAX - ITEM_SPAWN_MIN) * 10 + 1)) / 10.0f;
        }
    } else {
        /* 道具存活倒计时 */
        g->itemLife -= dt;
        if (g->itemLife <= 0.0f) {
            itemRemove(g);
        }
    }

    /* --- 更新所有玩家效果定时器 --- */
    for (p = 0; p < 2; p++) {
        int i;
        for (i = 0; i < NUM_EFFECTS; i++) {
            if (*ef[p] & (1u << i)) {
                if (i == EFF_FROZEN) {
                    /* 冻结只影响对手，由对手的 EFF_FROZEN 位管理 */
                }
                tm[p][i] -= dt;
                if (tm[p][i] <= 0.0f) {
                    tm[p][i] = 0.0f;
                    if (i == EFF_SHIELD) {
                        /* 护盾过期：消耗一层 */
                        (*shields[p])--;
                        if (*shields[p] > 0) {
                            tm[p][i] = SHIELD_DURATION;  /* 下一层护盾计时 */
                        } else {
                            *ef[p] &= ~(1u << i);
                        }
                    } else {
                        *ef[p] &= ~(1u << i);
                    }
                }
            }
        }
        /* 护盾数为0但位还在的清理 */
        if (*shields[p] <= 0) {
            *shields[p] = 0;
            *ef[p] &= ~(1u << EFF_SHIELD);
        }
    }
}

void itemCollect(GameState *g, int player, int itemType)
{
    if (!itemFeatureEnabled(g)) return;

    switch (itemType) {
    case ITEM_TURBO:
        setEffect(g, player, EFF_TURBO, TURBO_DURATION);
        break;

    case ITEM_SHIELD: {
        int *sh = (player == 0) ? &g->p1Shields : &g->p2Shields;
        *sh = (*sh < MAX_SHIELDS) ? (*sh + 1) : MAX_SHIELDS;
        /* 重置护盾计时（第一层护盾计15秒，每加一层也重置） */
        setEffect(g, player, EFF_SHIELD, SHIELD_DURATION);
        break;
    }

    case ITEM_SLOW:
        setEffect(g, player, EFF_SLOW, SLOW_DURATION);
        break;

    case ITEM_MAGNET:
        setEffect(g, player, EFF_MAGNET, MAGNET_DURATION);
        break;

    case ITEM_FREEZE:
        /* 冻结对手 */
        setEffect(g, 1 - player, EFF_FROZEN, FREEZE_DURATION);
        break;

    case ITEM_SHRINK: {
        Snake *s = (player == 0) ? &g->snake : &g->snake2;
        int cell = (player == 0) ? CELL_SNAKE : CELL_SNAKE2;
        int removeCount = 3;
        int i;
        if (s->len <= 1) break;  /* 至少保留1节 */
        for (i = 0; i < removeCount && s->len > 1; i++) {
            Position tail = s->body[s->len - 1];
            if (g->grid[tail.y][tail.x] == cell)
                g->grid[tail.y][tail.x] = CELL_EMPTY;
            s->len--;
        }
        break;
    }

    case ITEM_GHOST:
        setEffect(g, player, EFF_GHOST, GHOST_DURATION);
        break;

    case ITEM_DOUBLE:
        setEffect(g, player, EFF_DOUBLE, DOUBLE_DURATION);
        break;
    }

    /* 拾取后移除场上道具 */
    soundPlayItemCollect(itemType);
    itemRemove(g);
}

int itemHasEffect(const GameState *g, int player, int eff)
{
    if (!itemFeatureEnabled(g)) return 0;

    unsigned ef = (player == 0) ? g->p1Effects : g->p2Effects;
    return (ef & (1u << eff)) != 0;
}

float itemTimer(const GameState *g, int player, int eff)
{
    if (!itemFeatureEnabled(g)) return 0.0f;

    const float *tm = (player == 0) ? g->p1Timers : g->p2Timers;
    return tm[eff];
}

int itemShieldCount(const GameState *g, int player)
{
    if (!itemFeatureEnabled(g)) return 0;

    return (player == 0) ? g->p1Shields : g->p2Shields;
}

int itemIsNegative(int itemType)
{
    return (itemType == ITEM_SLOW || itemType == ITEM_SHRINK);
}

void itemRemove(GameState *g)
{
    if (g->itemOnField != ITEM_NONE) {
        if (g->itemPos.x >= 0 && g->itemPos.y >= 0 &&
            g->itemPos.x < g->mapSize && g->itemPos.y < g->mapSize &&
            g->grid[g->itemPos.y][g->itemPos.x] == CELL_ITEM) {
            g->grid[g->itemPos.y][g->itemPos.x] = CELL_EMPTY;
        }
        g->itemOnField = ITEM_NONE;
        g->itemLife = 0.0f;
    }
}

int itemConsumeShield(GameState *g, int player)
{
    if (!itemFeatureEnabled(g)) return 0;

    int *shields = (player == 0) ? &g->p1Shields : &g->p2Shields;
    unsigned *ef = (player == 0) ? &g->p1Effects : &g->p2Effects;
    float *tm = (player == 0) ? g->p1Timers : g->p2Timers;

    if (*shields <= 0) return 0;
    (*shields)--;
    achOnShieldBlock(g);
    soundPlayShieldBlock();
    if (*shields <= 0) {
        *shields = 0;
        *ef &= ~(1u << EFF_SHIELD);
    } else {
        tm[EFF_SHIELD] = SHIELD_DURATION;
    }
    return 1;
}


/* ============================================================
 *  从 ai.cpp 合并
 * ============================================================ */
/* ============================================================
 *  ai.cpp - AI 敌人蛇实现
 *
 *  贪心寻路策略：
 *  1. 枚举四个方向，排除越界/墙/障碍/蛇身/反向
 *  2. 从可行方向中选择离蓝食物曼哈顿距离最近的方向
 *  3. 无可行方向则保持原方向（撞死）
 *
 *  AI 吃蓝食物 → 增长，食物刷新
 *  AI 吃红食物 → 食物消失，无增长
 *  AI 不吃道具（CELL_ITEM 视为空格）
 * ============================================================ */


/* ===================================================
 *  内部辅助
 * =================================================== */

/* 探测方向 dx,dy */
static const int dirDX[4] = { 0, 0, -1, 1 };   /* UP DOWN LEFT RIGHT */
static const int dirDY[4] = { -1, 1, 0, 0 };

/* 反向映射 */
static int oppositeDir(int d)
{
    if (d == DIR_UP) return DIR_DOWN;
    if (d == DIR_DOWN) return DIR_UP;
    if (d == DIR_LEFT) return DIR_RIGHT;
    if (d == DIR_RIGHT) return DIR_LEFT;
    return -1;
}

/* 曼哈顿距离 */
static int manhattan(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

/* 格子是否可通行。
 * CELL_ITEM 故意排除：AI 不拾取道具。若未来需要 AI 踩道具，
 * 必须在移动时同时调用 itemRemove，否则道具过期会把 AI 身体格清空。 */
static int isPassable(int cell)
{
    return cell == CELL_EMPTY || cell == CELL_BLUE || cell == CELL_RED;
}

/* AI 允许的移动目标（含虚拟清尾：移动到自身尾巴允许，
 * 因为该尾格本回合会被清空）。 */
static int canStepTo(const GameState *g, const AISnake *a, int x, int y)
{
    int cell = g->grid[y][x];
    if (cell == CELL_AI) {
        int last = a->len - 1;
        if (last >= 0 && x == a->body[last].x && y == a->body[last].y)
            return 1;
        return 0;
    }

    return isPassable(cell);
}

/* 计算 AI 蛇最佳移动方向 */
static int aiChooseDir(const GameState *g, int aiIdx)
{
    const AISnake *a = &g->ai[aiIdx];
    int hx = a->body[0].x;
    int hy = a->body[0].y;
    int opp = oppositeDir(a->lastDir);
    int bestDir = a->dir;
    int bestDist = 9999;
    int hasValid = 0;
    int d;

    for (d = 0; d < 4; d++) {
        int nx = hx + dirDX[d];
        int ny = hy + dirDY[d];

        /* 不能反向 */
        if (d == opp) continue;

        /* 边界检查 */
        if (nx < 0 || ny < 0 || nx >= g->mapSize || ny >= g->mapSize)
            continue;

        /* 可通过格子 */
        if (!canStepTo(g, a, nx, ny)) continue;

        /* 通过！如果有蓝食物，计算距离 */
        hasValid = 1;
        {
            int distToFood = manhattan(nx, ny,
                g->blueFood.x, g->blueFood.y);
            if (distToFood < bestDist) {
                bestDist = distToFood;
                bestDir = d;
            }
        }
    }

    if (!hasValid) {
        /* 全部堵死，保持方向等死 */
        return a->dir;
    }
    return bestDir;
}

/* 尝试在指定位置生成 AI 蛇 */
static int trySpawnAt(GameState *g, int startX, int startY, int dir, int len)
{
    int i, x = startX, y = startY;
    /* 检查起点 */
    if (!isPassable(g->grid[y][x])) return 0;

    /* 检查在前 len 步是否都安全 */
    for (i = 0; i < len; i++) {
        if (x < 0 || y < 0 || x >= g->mapSize || y >= g->mapSize)
            return 0;
        if (!isPassable(g->grid[y][x])) return 0;
        x += dirDX[dir];
        y += dirDY[dir];
    }
    return 1;
}

/* ===================================================
 *  公开 API
 * =================================================== */

void aiInit(GameState *g)
{
    g->aiCount = 0;
    g->aiSpawnCD = AI_SPAWN_MIN +
        (float)(rand() % (int)((AI_SPAWN_MAX - AI_SPAWN_MIN) * 10 + 1)) / 10.0f;
    g->aiTickTimer = 0.0f;
}

void aiUpdate(GameState *g, float dt)
{
    int i, d;

    if (!g || !g->config.aiEnabled)
        return;

    /* --- 生成逻辑 --- */
    if (g->aiCount < MAX_AI) {
        g->aiSpawnCD -= dt;
        if (g->aiSpawnCD <= 0.0f) {
            int len = AI_MIN_LEN + rand() % (AI_MAX_LEN - AI_MIN_LEN + 1);
            int attempts = g->mapSize * g->mapSize;
            while (attempts-- > 0) {
                int sx = rand() % g->mapSize;
                int sy = rand() % g->mapSize;
                d = rand() % 4;
                if (trySpawnAt(g, sx, sy, d, len)) {
                    /* 生成！ */
                    AISnake *a = &g->ai[g->aiCount];
                    int k, cx = sx, cy = sy;
                    for (k = 0; k < len; k++) {
                        a->body[k].x = cx;
                        a->body[k].y = cy;
                        cx += dirDX[d];
                        cy += dirDY[d];
                    }
                    a->len = len;
                    a->dir = d;
                    a->lastDir = d;
                    g->aiCount++;
                    /* 标记到网格 */
                    for (k = 0; k < len; k++)
                        g->grid[a->body[k].y][a->body[k].x] = CELL_AI;
                    break;
                }
            }
            /* 重置冷却 */
            g->aiSpawnCD = AI_SPAWN_MIN +
                (float)(rand() % (int)((AI_SPAWN_MAX - AI_SPAWN_MIN) * 10 + 1)) / 10.0f;
        }
    }

    /* --- 移动逻辑 --- */
    g->aiTickTimer += dt;
    {
        float interval = (float)AI_SPEED / 1000.0f;
        if (g->aiTickTimer < interval) return;
        g->aiTickTimer -= interval;
    }

    i = 0;
    while (i < g->aiCount) {
        AISnake *a = &g->ai[i];
        int nx, ny, cell;

        /* 选择方向 */
        a->dir = aiChooseDir(g, i);

        nx = a->body[0].x + dirDX[a->dir];
        ny = a->body[0].y + dirDY[a->dir];

        /* 边界 */
        if (nx < 0 || ny < 0 || nx >= g->mapSize || ny >= g->mapSize) {
            aiKill(g, i); continue; /* 不递增 i */
        }

        cell = g->grid[ny][nx];

        /* 撞障碍/墙 */
        if (cell == CELL_WALL || cell == CELL_OBS) {
            aiKill(g, i); continue;
        }

        /* 撞 AI 身体：尾巴在本回合会被清空，允许走入尾格 */
        if (cell == CELL_AI) {
            Position tail = a->body[a->len - 1];
            if (nx != tail.x || ny != tail.y) {
                aiKill(g, i); continue;
            }
        }

        /* 撞玩家蛇身 → AI 死亡，玩家得分 */
        if (cell == CELL_SNAKE || cell == CELL_SNAKE2) {
            int rewardP1 = (cell == CELL_SNAKE);
            if (rewardP1)
                g->score += aiScoreReward(a->len, g->mult);
            else
                g->score2 += aiScoreReward(a->len, g->mult);
            achOnKillAI(g);
            achCheckAll(g, g->gameMode);
            aiKill(g, i); continue;
        }

        /* 吃蓝食物 */
        if (cell == CELL_BLUE) {
            int grow = (a->len < MAX_LEN);
            if (!grow) {
                Position tail = a->body[a->len - 1];
                if (g->grid[tail.y][tail.x] == CELL_AI)
                    g->grid[tail.y][tail.x] = CELL_EMPTY;
            }
            {
                int k;
                int newLen = a->len + (grow ? 1 : 0);
                for (k = newLen - 1; k > 0; k--)
                    a->body[k] = a->body[k - 1];
                a->body[0].x = nx;
                a->body[0].y = ny;
                a->len = newLen;
                a->lastDir = a->dir;
            }
            g->grid[ny][nx] = CELL_AI;
            g->blueCount++;
            gamePlaceBlueFood(g);
            if (g->blueCount % RED_TRIGGER == 0 && !g->hasRed)
                gamePlaceRedFood(g);
            i++; continue;
        }

        /* 吃红食物 */
        if (cell == CELL_RED && g->hasRed) {
            Position tail = a->body[a->len - 1];
            if (g->grid[tail.y][tail.x] == CELL_AI)
                g->grid[tail.y][tail.x] = CELL_EMPTY;
            {
                int k;
                for (k = a->len - 1; k > 0; k--)
                    a->body[k] = a->body[k - 1];
                a->body[0].x = nx;
                a->body[0].y = ny;
                a->lastDir = a->dir;
            }
            g->grid[ny][nx] = CELL_AI;
            g->hasRed = 0;
            g->redTimer = 0.0f;
            i++; continue;
        }

        /* 正常前进 */
        {
            Position tail = a->body[a->len - 1];
            if (g->grid[tail.y][tail.x] == CELL_AI)
                g->grid[tail.y][tail.x] = CELL_EMPTY;
        }
        {
            int k;
            for (k = a->len - 1; k > 0; k--)
                a->body[k] = a->body[k - 1];
            a->body[0].x = nx;
            a->body[0].y = ny;
            a->lastDir = a->dir;
        }
        g->grid[ny][nx] = CELL_AI;
        i++;
    }
}

void aiMarkAll(GameState *g)
{
    int i, j;
    for (i = 0; i < g->aiCount; i++) {
        const AISnake *a = &g->ai[i];
        for (j = 0; j < a->len; j++)
            g->grid[a->body[j].y][a->body[j].x] = CELL_AI;
    }
}

void aiKill(GameState *g, int index)
{
    AISnake *a = &g->ai[index];
    int j, last;

    /* 清理网格 */
    for (j = 0; j < a->len; j++) {
        int x = a->body[j].x, y = a->body[j].y;
        if (x >= 0 && y >= 0 && x < GRID_SIZE && y < GRID_SIZE &&
            g->grid[y][x] == CELL_AI)
            g->grid[y][x] = CELL_EMPTY;
    }

    /* 数组收缩（将最后一个 AI 移到当前位置） */
    last = g->aiCount - 1;
    if (index != last) {
        g->ai[index] = g->ai[last];
    }
    g->aiCount--;
}

int aiScoreReward(int aiLen, int mult)
{
    return aiLen * BLUE_BASE * mult / 10;
}


/* ============================================================
 *  从 movingobs.cpp 合并
 * ============================================================ */
/* ============================================================
 *  movingobs.cpp - 移动障碍实现
 * ============================================================ */


static const int obsDirDX[4] = { 0, 0, -1, 1 };
static const int obsDirDY[4] = { -1, 1, 0, 0 };

/* 翻转方向 */
static int flipDir(int d)
{
    if (d == DIR_UP) return DIR_DOWN;
    if (d == DIR_DOWN) return DIR_UP;
    if (d == DIR_LEFT) return DIR_RIGHT;
    return DIR_LEFT;
}

void movingObsInit(GameState *g)
{
    g->obsMovingCount = 0;
    g->obsMovingSpawnCD = MOVING_OBS_SPAWN;
}

void movingObsUpdate(GameState *g, float dt)
{
    int i;
    /* 生成 */
    if (g->obsMovingCount < MOVING_OBS_MAX) {
        g->obsMovingSpawnCD -= dt * (1.0f / g->diffFactor);
        if (g->obsMovingSpawnCD <= 0.0f) {
            int tries = 100;
            while (tries-- > 0) {
                int x = rand() % g->mapSize;
                int y = rand() % g->mapSize;
                if (g->grid[y][x] == CELL_EMPTY) {
                    int idx = g->obsMovingCount;
                    g->obsMoving[idx].x = x;
                    g->obsMoving[idx].y = y;
                    g->obsMovingDir[idx] = rand() % 4;
                    g->grid[y][x] = CELL_OBS;
                    g->obsMovingCount++;
                    break;
                }
            }
            g->obsMovingSpawnCD = MOVING_OBS_SPAWN;
        }
    }

    /* 移动（每秒移动一格） */
    g->obsMovingTimer += dt;
    {
        float interval = 1.0f / g->diffFactor;  /* 越难越快 */
        while (g->obsMovingTimer >= interval) {
            g->obsMovingTimer -= interval;
            for (i = 0; i < g->obsMovingCount; i++) {
                int nx = g->obsMoving[i].x + obsDirDX[g->obsMovingDir[i]];
                int ny = g->obsMoving[i].y + obsDirDY[g->obsMovingDir[i]];
                int cell;

                if (nx < 0 || ny < 0 || nx >= g->mapSize ||
                    ny >= g->mapSize) {
                    g->obsMovingDir[i] = flipDir(g->obsMovingDir[i]);
                    continue;
                }
                cell = g->grid[ny][nx];
                if (cell == CELL_WALL || cell == CELL_OBS) {
                    g->obsMovingDir[i] = flipDir(g->obsMovingDir[i]);
                    continue;
                }
                if (cell == CELL_EMPTY) {
                    g->grid[g->obsMoving[i].y][g->obsMoving[i].x] = CELL_EMPTY;
                    g->obsMoving[i].x = nx;
                    g->obsMoving[i].y = ny;
                    g->grid[ny][nx] = CELL_OBS;
                }
                /* 如果目标格有蛇/AI/食物，不移动（障碍不会主动压上去） */
            }
        }
    }
}


/* ============================================================
 *  从 floattext.cpp 合并
 * ============================================================ */
/* ============================================================
 *  floattext.cpp - 飘字系统实现
 * ============================================================ */


void floatTextAdd(GameState *g, float x, float y,
                  LPCTSTR text, COLORREF color, float life)
{
    int i;
    for (i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (g->floatTexts[i].life <= 0.0f) {
            g->floatTexts[i].x = x;
            g->floatTexts[i].y = y;
            g->floatTexts[i].life = life;
            g->floatTexts[i].color = color;
            _tcscpy_s(g->floatTexts[i].text, 16, text);
            return;
        }
    }
}

void floatTextUpdate(GameState *g, float dt)
{
    int i;
    for (i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (g->floatTexts[i].life > 0.0f) {
            g->floatTexts[i].life -= dt;
            g->floatTexts[i].y -= 0.8f * dt;  /* 每秒上飘0.8格 */
            if (g->floatTexts[i].life < 0.0f)
                g->floatTexts[i].life = 0.0f;
        }
    }
}

void floatTextClearAll(GameState *g)
{
    memset(g->floatTexts, 0, sizeof(g->floatTexts));
}


/* ============================================================
 *  从 combo.cpp 合并
 * ============================================================ */
/* ============================================================
 *  combo.cpp - 连击系统实现
 * ============================================================ */

static float *comboGetCD(GameState *g, int player)
{
    if (player == 1) return &g->comboCD2;
    return &g->comboCD;
}

static int *comboGetCount(GameState *g, int player)
{
    if (player == 1) return &g->comboCount2;
    return &g->comboCount;
}

static int *comboGetMax(GameState *g, int player)
{
    if (player == 1) return &g->maxCombo2;
    return &g->maxCombo;
}

void comboUpdate(GameState *g, float dt)
{
    comboUpdateForPlayer(g, dt, 0);
}

void comboUpdateForPlayer(GameState *g, float dt, int player)
{
    float *comboCD = comboGetCD(g, player);
    int *comboCount = comboGetCount(g, player);

    if (!g || !comboCount || !comboCD)
        return;

    if (*comboCount > 0) {
        *comboCD -= dt;
        if (*comboCD <= 0.0f) {
            *comboCD = 0.0f;
            *comboCount = 0;
        }
    }
}

int comboOnEat(GameState *g, int baseScore)
{
    return comboOnEatForPlayer(g, baseScore, 0);
}

int comboOnEatForPlayer(GameState *g, int baseScore, int player)
{
    float *comboCD = comboGetCD(g, player);
    int *comboCount = comboGetCount(g, player);
    int *maxCount = comboGetMax(g, player);

    if (!g || !comboCount || !comboCD || !maxCount)
        return baseScore;

    if (*comboCD > 0.0f) {
        (*comboCount)++;
    } else {
        *comboCount = 1;
    }
    *comboCD = COMBO_WINDOW;

    if (*comboCount > *maxCount)
        *maxCount = *comboCount;

    return (int)(baseScore * (1.0f + (*comboCount - 1) * 0.5f));
}

int comboCount(const GameState *g) { return comboCountForPlayer(g, 0); }

int comboCountForPlayer(const GameState *g, int player)
{
    if (!g) return 0;
    if (player == 1) return g->comboCount2;
    return g->comboCount;
}

int comboMax(const GameState *g)   { return comboMaxForPlayer(g, 0); }

int comboMaxForPlayer(const GameState *g, int player)
{
    if (!g) return 0;
    if (player == 1) return g->maxCombo2;
    return g->maxCombo;
}


/* ============================================================
 *  从 achievement.cpp 合并
 * ============================================================ */
/* ============================================================
 *  achievement.cpp - 成就系统实现
 * ============================================================ */


static const LPCTSTR aName[ACH_MAX] = {
    _T("初出茅庐"), _T("百炼成钢"), _T("大胃王"),
    _T("连击大师"), _T("蛇猎人"),   _T("钢铁之躯"),
    _T("速通达人"), _T("生存专家"), _T("镜像王者"),
    _T("满分猎人")
};

static const LPCTSTR aDesc[ACH_MAX] = {
    _T("首次得分超过100"),
    _T("累计吃掉100个蓝食物"),
    _T("单局吃掉20个蓝食物"),
    _T("达成5连击"),
    _T("累计杀死10条AI蛇"),
    _T("累计用护盾挡住10次死亡"),
    _T("60秒内获得200分"),
    _T("生存模式活过180秒"),
    _T("镜像对决中获胜"),
    _T("吃到满分的红食物")
};

static void parseCounterLine(const char *line, const char *key, int *out)
{
    const char *p = line;
    size_t keyLen = strlen(key);

    if (!out || !line || !key)
        return;

    while (*p && isspace((unsigned char)*p))
        p++;

    if (strncmp(p, key, keyLen) != 0)
        return;

    p += keyLen;
    if (*p == '\0' || isspace((unsigned char)*p) || *p == '=' || *p == ':') {
        while (*p && (isspace((unsigned char)*p) || *p == '=' || *p == ':'))
            p++;

        if (*p) {
            char *end = NULL;
            long val = strtol(p, &end, 10);
            if (end != p)
                *out = (int)val;
        }
    }
}

LPCTSTR achName(int achId) { return aName[achId]; }
LPCTSTR achDesc(int achId) { return aDesc[achId]; }

unsigned achLoad(void)
{
    unsigned mask = 0;
    /* 优先从 record.txt 读取 */
    if (loadRecordAchievements(&mask, NULL, NULL, NULL))
        return mask;

    /* 兼容旧存档：从未迁移的 achievements.txt 读取 */
    {
        FILE *fp = fopen("achievements.txt", "r");
        if (fp) {
            fscanf(fp, "%u", &mask);
            fclose(fp);
        }
    }
    return mask;
}

void achLoadState(int *blueTotal, int *aiKills, int *shieldBlocks)
{
    if (loadRecordAchievements(NULL, blueTotal, aiKills, shieldBlocks))
        return;

    /* 兼容旧存档 */
    {
        FILE *fp = fopen("achievements.txt", "r");
        char line[128];

        if (blueTotal) *blueTotal = 0;
        if (aiKills) *aiKills = 0;
        if (shieldBlocks) *shieldBlocks = 0;

        if (!fp) return;

        while (fgets(line, sizeof(line), fp)) {
            parseCounterLine(line, "blueTotal", blueTotal);
            parseCounterLine(line, "aiKills", aiKills);
            parseCounterLine(line, "shieldBlocks", shieldBlocks);
        }
        fclose(fp);
    }
}

void achSaveState(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    /* 统一写入 record.txt（保留原有最高分/配置） */
    saveRecordAchievements(mask, blueTotal, aiKills, shieldBlocks);
}

int achUnlock(GameState *g, int achId)
{
    if (g->achUnlocked & (1u << achId)) return 0;
    g->achUnlocked |= (1u << achId);
    g->achNewUnlock |= (1u << achId);  /* 标记为新解锁（用于gfx通知） */
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
    return 1;
}

void achOnEatBlue(GameState *g)
{
    g->achBlueThisGame++;
    g->achBlueTotal++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

void achOnKillAI(GameState *g)
{
    g->achAIKills++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

void achOnShieldBlock(GameState *g)
{
    g->achShieldBlocks++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

int achCheckAll(GameState *g, int gameMode)
{
    int newCount = 0;

    if (g->score > 100)
        newCount += achUnlock(g, ACH_FIRST100);

    if (g->achBlueTotal >= 100)
        newCount += achUnlock(g, ACH_BLUE100);

    if (g->achBlueThisGame >= 20)
        newCount += achUnlock(g, ACH_BLUE20);

    if (g->maxCombo >= 5)
        newCount += achUnlock(g, ACH_COMBO5);

    if (g->achAIKills >= 10)
        newCount += achUnlock(g, ACH_KILLAI10);

    if (g->achShieldBlocks >= 10)
        newCount += achUnlock(g, ACH_SHIELD10);

    if ((gameMode == MODE_SINGLE || gameMode == MODE_DUAL_TIMED) &&
        g->score >= 200 && g->elapsedTime > 0.0f && g->elapsedTime <= 60.0f)
        newCount += achUnlock(g, ACH_SPEED200);

    if (gameMode == MODE_SURVIVAL && g->elapsedTime >= 180.0f)
        newCount += achUnlock(g, ACH_SURVIVE180);

    /* ACH_MIRRORWIN and ACH_RED50 checked externally */

    return newCount;
}


/* ============================================================
 *  ===== 板块一结束 =====
 * ============================================================ */
