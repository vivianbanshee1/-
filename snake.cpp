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

#include "snake.h"
#include "game_api.h"


/* ============================================================
 *  snake.cpp - 合并版：全部模块归并实现
 *
 *  本文件整合自原始项目中的 11 个模块文件：
 *  snake.cpp / food.cpp / item.cpp / ai.cpp / movingobs.cpp /
 *  floattext.cpp / combo.cpp / achievement.cpp / record.cpp /
 *  sound.cpp / input.cpp / gfx.cpp / main.cpp。
 *
 *  仅为满足作业“1 个 cpp + 2 个 h”的合并版，
 *  模块功能、流程与现有实现保持一致。
 * ============================================================ */



/* ============================================================
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
 *  从 record.cpp 合并
 * ============================================================ */
/* ============================================================
 *  record.cpp - 记录持久化实现
 *
 *  从 snake.cpp 提取的文件读写逻辑。
 * ============================================================ */


static void trimString(char *text)
{
    char *p;
    size_t len;

    if (!text) return;

    while (isspace((unsigned char) *text)) {
        memmove(text, text + 1, strlen(text));
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char) text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

static int parseKeyValueLine(const char *srcLine, char *key, int keyCap,
                            char *value, int valueCap)
{
    char line[512];
    char *p;
    char *sep;
    size_t len;

    if (!srcLine || !key || keyCap <= 0 || !value || valueCap <= 0) {
        return 0;
    }

    snprintf(line, sizeof(line), "%s", srcLine);
    trimString(line);
    if (line[0] == '\0' || line[0] == '#') {
        return 0;
    }

    p = line;
    sep = strpbrk(p, " \t");
    if (!sep) {
        snprintf(key, keyCap, "%s", p);
        value[0] = '\0';
        return 1;
    }

    *sep = '\0';
    snprintf(key, keyCap, "%s", p);

    sep++;
    while (*sep && isspace((unsigned char) *sep)) {
        sep++;
    }

    len = strlen(sep);
    if (len >= (size_t) (valueCap)) {
        len = (size_t) (valueCap - 1);
    }

    memcpy(value, sep, len);
    value[len] = '\0';
    trimString(value);

    return 2;
}

static int parseIntValue(const char *value, int *out)
{
    if (!value || !*value || !out) return 0;
    if (sscanf(value, "%d", out) != 1) return 0;
    return 1;
}

static int readTopScore(FILE *fp, int *highScore)
{
    char line[512];

    if (!fp || !highScore) return 0;

    if (!fgets(line, sizeof(line), fp)) {
        *highScore = 0;
        return 0;
    }

    if (sscanf(line, "%d", highScore) != 1) {
        *highScore = 0;
        return 0;
    }

    return 1;
}

void loadRecordConfig(int *highScore, GameConfig *cfg)
{
    FILE *fp;
    char line[512];
    char key[64];
    char value[448];
    int valueInt;

    if (highScore) *highScore = 0;
    if (cfg) {
        cfg->snakeColor = SNAKE_COLOR_GREEN;
        cfg->mapSize = MAP_SIZE_LARGE;
        cfg->itemMode = GAMEPLAY_ITEM;
        cfg->aiEnabled = 1;
        cfg->menuBgImagePath[0] = '\0';
        cfg->menuMusicPath[0] = '\0';
    }

    fp = fopen("record.txt", "r");
    if (!fp) return;

    readTopScore(fp, highScore);

    while (fgets(line, sizeof(line), fp)) {
        int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

        if (parsed <= 0) {
            continue;
        }

        if (!cfg) {
            continue;
        }

        if (strcmp(key, "snakeColor") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->snakeColor = normalizeSnakeColor(valueInt);
            }
        } else if (strcmp(key, "mapSize") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->mapSize = normalizeMapSize(valueInt);
            }
        } else if (strcmp(key, "itemMode") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->itemMode = normalizeItemMode(valueInt);
            }
        } else if (strcmp(key, "aiEnabled") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->aiEnabled = normalizeAiEnabled(valueInt);
            }
        } else if (strcmp(key, "menuBgImagePath") == 0) {
            if (parsed == 2) {
                snprintf(cfg->menuBgImagePath, sizeof(cfg->menuBgImagePath), "%s", value);
            } else {
                cfg->menuBgImagePath[0] = '\0';
            }
        } else if (strcmp(key, "menuMusicPath") == 0) {
            if (parsed == 2) {
                snprintf(cfg->menuMusicPath, sizeof(cfg->menuMusicPath), "%s", value);
            } else {
                cfg->menuMusicPath[0] = '\0';
            }
        }
    }

    fclose(fp);
}

void saveRecordConfig(int highScore, const GameConfig *cfg)
{
    unsigned achMask = 0;
    int achBlueTotal = 0, achAIKills = 0, achShieldBlocks = 0;
    int hasAch = loadRecordAchievements(&achMask, &achBlueTotal, &achAIKills, &achShieldBlocks);
    FILE *fp = fopen("record.txt", "w");

    if (!fp) return;

    fprintf(fp, "%d\n", highScore);
    fprintf(fp, "snakeColor %d\n", cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN);
    fprintf(fp, "mapSize %d\n", cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE);
    fprintf(fp, "itemMode %d\n", cfg ? normalizeItemMode(cfg->itemMode) : GAMEPLAY_ITEM);
    fprintf(fp, "aiEnabled %d\n", cfg ? normalizeAiEnabled(cfg->aiEnabled) : 1);
    if (cfg) {
        fprintf(fp, "menuBgImagePath %s\n", cfg->menuBgImagePath[0] ? cfg->menuBgImagePath : "");
        fprintf(fp, "menuMusicPath %s\n", cfg->menuMusicPath[0] ? cfg->menuMusicPath : "");
    }
    if (hasAch) {
        fprintf(fp, "achMask %u\n", achMask);
        fprintf(fp, "achBlueTotal %d\n", achBlueTotal);
        fprintf(fp, "achAIKills %d\n", achAIKills);
        fprintf(fp, "achShieldBlocks %d\n", achShieldBlocks);
    }
    fclose(fp);
}

/* --- 成就数据合并到 record.txt ---
 * 格式：行首为 "ach " 前缀的字段由本模块管理，其它字段保持 record.txt 原样。
 * 这样 record.txt 成为唯一的存档文件。 */

static void rewriteRecordWithAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    FILE *fp = fopen("record.txt", "r");
    int highScore = 0;
    char line[512];

    typedef struct {
        char k[64];
        char v[448];
    } RecordLine;

    RecordLine extras[32];
    int extraCount = 0;
    char key[64];
    char value[448];
    int i;

    if (fp) {
        readTopScore(fp, &highScore);

        while (fgets(line, sizeof(line), fp)) {
            int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

            if (parsed <= 0) {
                continue;
            }
            if (strcmp(key, "achMask") == 0) continue;
            if (strcmp(key, "achBlueTotal") == 0) continue;
            if (strcmp(key, "achAIKills") == 0) continue;
            if (strcmp(key, "achShieldBlocks") == 0) continue;

            if (extraCount < 32) {
                snprintf(extras[extraCount].k, sizeof(extras[extraCount].k), "%s", key);
                snprintf(extras[extraCount].v, sizeof(extras[extraCount].v), "%s", (parsed == 2 ? value : ""));
                extraCount++;
            }
        }
        fclose(fp);
    }

    fp = fopen("record.txt", "w");
    if (!fp) return;

    fprintf(fp, "%d\n", highScore);
    for (i = 0; i < extraCount; i++) {
        if (extras[i].v[0] == '\0') {
            fprintf(fp, "%s\n", extras[i].k);
        } else {
            fprintf(fp, "%s %s\n", extras[i].k, extras[i].v);
        }
    }

    fprintf(fp, "achMask %u\n", mask);
    fprintf(fp, "achBlueTotal %d\n", blueTotal);
    fprintf(fp, "achAIKills %d\n", aiKills);
    fprintf(fp, "achShieldBlocks %d\n", shieldBlocks);
    fclose(fp);
}

int loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks)
{
    FILE *fp = fopen("record.txt", "r");
    char line[512];
    char key[64];
    char value[448];
    int valueInt;
    int found = 0;

    if (mask) *mask = 0;
    if (blueTotal) *blueTotal = 0;
    if (aiKills) *aiKills = 0;
    if (shieldBlocks) *shieldBlocks = 0;

    if (!fp) return 0;

    {
        int dummy = 0;
        readTopScore(fp, &dummy);
    }

    while (fgets(line, sizeof(line), fp)) {
        int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

        if (parsed < 2) {
            continue;
        }

        if (strcmp(key, "achMask") == 0) {
            if (parseIntValue(value, &valueInt)) {
                if (mask) *mask = (unsigned) valueInt;
                found = 1;
            }
        }
        else if (strcmp(key, "achBlueTotal") == 0 && blueTotal && parseIntValue(value, &valueInt)) {
            *blueTotal = valueInt;
        }
        else if (strcmp(key, "achAIKills") == 0 && aiKills && parseIntValue(value, &valueInt)) {
            *aiKills = valueInt;
        }
        else if (strcmp(key, "achShieldBlocks") == 0 && shieldBlocks && parseIntValue(value, &valueInt)) {
            *shieldBlocks = valueInt;
        }
    }

    fclose(fp);
    return found;
}

void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    rewriteRecordWithAchievements(mask, blueTotal, aiKills, shieldBlocks);
}


/* ============================================================
 *  从 sound.cpp 合并
 * ============================================================ */
/* ============================================================
 *  sound.cpp - 简易音效实现
 *
 *  优先播放本地 wav 文件（可从网上下载后放入 assets/sounds），
 *  若文件不存在则回退到短频率 Beep。
 * ============================================================ */


static int s_soundEnabled = 1;
static char s_soundDir[MAX_PATH] = "assets/sounds";

static const char BGM_ALIAS[] = "SnakeBGM";
static char s_bgmPath[MAX_PATH] = "";
static int s_bgmLoaded = 0;
static int s_bgmPlaying = 0;

static void closeBackgroundMusicNoLock(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded) return;

    snprintf(cmd, sizeof(cmd), "close %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "stop %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "seek %s to start", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);

    s_bgmLoaded = 0;
    s_bgmPlaying = 0;
}

static int fileExists(const char *path)
{
    if (path == NULL || *path == '\0') return 0;
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static void soundResolveDir(void)
{
    char exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, exePath, (DWORD) (sizeof(exePath) - 1));
    if (len == 0 || len >= sizeof(exePath) - 1) return;

    char *sep = strrchr(exePath, '\\');
    if (sep == NULL) return;
    *sep = '\0';

    snprintf(s_soundDir, sizeof(s_soundDir), "%s/assets/sounds", exePath);
}

static int playSoundFile(const char *file)
{
    char path[MAX_PATH] = {0};
    if (file == NULL || *file == '\0') return 0;

    if (s_soundDir[0] != '\0') {
        snprintf(path, sizeof(path), "%s/%s", s_soundDir, file);
        if (fileExists(path) && PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)) {
            return 1;
        }
    }

    snprintf(path, sizeof(path), "%s", file);
    if (fileExists(path) && PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)) {
        return 1;
    }

    return 0;
}

static void playToneSeq(const unsigned *freq, const unsigned *ms, int n)
{
    int i;
    if (!s_soundEnabled || n <= 0) return;

    for (i = 0; i < n; i++) {
        if (freq[i] == 0 || ms[i] == 0) {
            Sleep((DWORD)ms[i]);
            continue;
        }
        Beep((unsigned int)freq[i], (int)ms[i]);
    }
}

static void playSoundOrTone(const char *file, const unsigned *freq, const unsigned *ms, int n)
{
    if (!s_soundEnabled) {
        return;
    }

    if (!playSoundFile(file)) {
        playToneSeq(freq, ms, n);
    }
}

void soundInit(void)
{
    soundResolveDir();
}

void soundSetEnabled(int enabled)
{
    s_soundEnabled = (enabled != 0);

    if (!s_soundEnabled) {
        soundStopBackgroundMusic();
    } else if (s_bgmLoaded) {
        soundPlayBackgroundMusic();
    }
}

int soundIsEnabled(void)
{
    return s_soundEnabled;
}

int soundSetBackgroundMusic(const char *filePath)
{
    char cmd[3 * MAX_PATH];
    int ret;

    closeBackgroundMusicNoLock();
    s_bgmPath[0] = '\0';

    if (filePath == NULL || *filePath == '\0') {
        return 0;
    }

    if (!fileExists(filePath)) {
        return 0;
    }

    snprintf(s_bgmPath, sizeof(s_bgmPath), "%s", filePath);

    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", filePath, BGM_ALIAS);
    ret = mciSendStringA(cmd, NULL, 0, NULL);

    if (ret != 0) {
        snprintf(cmd, sizeof(cmd), "open \"%s\" alias %s", filePath, BGM_ALIAS);
        ret = mciSendStringA(cmd, NULL, 0, NULL);
    }

    if (ret != 0) {
        s_bgmPath[0] = '\0';
        return 0;
    }

    s_bgmLoaded = 1;
    s_bgmPlaying = 0;

    if (s_soundEnabled) {
        soundPlayBackgroundMusic();
    }

    return 1;
}

void soundPlayBackgroundMusic(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded || !s_soundEnabled || s_bgmPlaying) {
        return;
    }

    snprintf(cmd, sizeof(cmd), "play %s repeat", BGM_ALIAS);
    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        s_bgmPlaying = 1;
    }
}

void soundStopBackgroundMusic(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded && s_bgmPlaying == 0) {
        return;
    }

    snprintf(cmd, sizeof(cmd), "stop %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "seek %s to start", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    s_bgmPlaying = 0;
}

int soundHasBackgroundMusic(void)
{
    return s_bgmLoaded != 0;
}

void soundPlayMenuConfirm(void)
{
    static const unsigned f[] = { 740, 980, 1319 };
    static const unsigned d[] = { 22, 18, 30 };
    playSoundOrTone("menu_confirm.wav", f, d, 3);
}

void soundPlayEatBlue(void)
{
    static const unsigned f[] = { 1047, 1319, 1568 };
    static const unsigned d[] = { 24, 24, 26 };
    playSoundOrTone("eat_blue.wav", f, d, 3);
}

void soundPlayEatRed(void)
{
    static const unsigned f[] = { 784, 988, 1319 };
    static const unsigned d[] = { 28, 28, 40 };
    playSoundOrTone("eat_red.wav", f, d, 3);
}

void soundPlayItemCollect(int itemType)
{
    if (itemType == ITEM_TURBO || itemType == ITEM_SHIELD || itemType == ITEM_DOUBLE || itemType == ITEM_GHOST) {
        static const unsigned f[] = { 880, 1175, 1397 };
        static const unsigned d[] = { 25, 25, 30 };
        playSoundOrTone("item_collect_bright.wav", f, d, 3);
    } else if (itemType == ITEM_SLOW) {
        static const unsigned f[] = { 587, 659, 587, 523 };
        static const unsigned d[] = { 22, 22, 22, 28 };
        playSoundOrTone("item_collect_slow.wav", f, d, 4);
    } else if (itemType == ITEM_MAGNET || itemType == ITEM_FREEZE) {
        static const unsigned f[] = { 1047, 1175, 1175, 880 };
        static const unsigned d[] = { 20, 20, 20, 30 };
        playSoundOrTone("item_collect_magic.wav", f, d, 4);
    } else if (itemType == ITEM_SHRINK) {
        static const unsigned f[] = { 392, 330, 262, 220 };
        static const unsigned d[] = { 20, 20, 24, 24 };
        playSoundOrTone("item_collect_shrink.wav", f, d, 4);
    } else {
        static const unsigned f[] = { 1047, 988, 1047 };
        static const unsigned d[] = { 26, 26, 30 };
        playSoundOrTone("item_collect.wav", f, d, 3);
    }
}

void soundPlayDeath(void)
{
    static const unsigned f[] = { 880, 698, 523, 392 };
    static const unsigned d[] = { 30, 30, 40, 45 };
    playSoundOrTone("death.wav", f, d, 4);
}

void soundPlayPause(void)
{
    static const unsigned f[] = { 523, 0, 523 };
    static const unsigned d[] = { 18, 18, 18 };
    playSoundOrTone("pause.wav", f, d, 3);
}

void soundPlayResume(void)
{
    static const unsigned f[] = { 523, 659 };
    static const unsigned d[] = { 20, 20 };
    playSoundOrTone("resume.wav", f, d, 2);
}

void soundPlayShieldBlock(void)
{
    static const unsigned f[] = { 1568, 1568, 1175, 1568 };
    static const unsigned d[] = { 18, 18, 20, 40 };
    playSoundOrTone("shield.wav", f, d, 4);
}

void soundPlayGameOver(void)
{
    static const unsigned f[] = { 523, 440, 392, 330, 262, 196 };
    static const unsigned d[] = { 30, 30, 30, 30, 30, 60 };
    playSoundOrTone("game_over.wav", f, d, 6);
}


/* ============================================================
 *  从 input.cpp 合并
 * ============================================================ */
/* ============================================================
 *  input.cpp - 输入处理模块实现
 *
 *  从 gfx.cpp 提取的键盘/鼠标输入处理逻辑。
 *  依赖 gfx.h 中的 gfxHitDeadButton 进行死亡界面按钮命中检测。
 * ============================================================ */


static int s_windowClosePending = 0;

static void markWindowClosePending(void)
{
    s_windowClosePending = 1;
}

static int isCloseMessage(const MSG *msg)
{
    if (!msg) return 0;

    if (msg->message == WM_CLOSE || msg->message == WM_QUIT
        || msg->message == WM_DESTROY || msg->message == WM_NCDESTROY)
        return 1;

    if (msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_CLOSE))
        return 1;

    return 0;
}

int gfxWindowCloseRequested(void)
{
    MSG msg;

    if (s_windowClosePending) return 1;

    if (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_DESTROY, WM_DESTROY, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_NCDESTROY, WM_NCDESTROY, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE)) {
        if (isCloseMessage(&msg)) {
            markWindowClosePending();
            return 1;
        }
    }

    return 0;
}

int gfxGetKey(void)
{
    ExMessage m;
    /* 仅处理键盘消息，不消费鼠标消息。
     * 鼠标消息留给 pollMirrorEndButton() 等专门处理，避免镜像模式按钮点击被吞掉。 */
    while (peekmessage(&m, EX_KEY, true)) {
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

        if (m.message == WM_KEYDOWN) {
            if (m.vkcode == VK_UP) return KEY_UP;
            if (m.vkcode == VK_DOWN) return KEY_DOWN;
            if (m.vkcode == VK_LEFT) return KEY_LEFT;
            if (m.vkcode == VK_RIGHT) return KEY_RIGHT;
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxGetMenuInput(int *hoverIndex)
{
    ExMessage m;
    int mx, my, hit;

    while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

        if (m.message == WM_MOUSEMOVE) {
            mx = m.x; my = m.y;
            hit = gfxHitMenuButton(mx, my);
            if (hoverIndex) *hoverIndex = hit;
        }
        else if (m.message == WM_LBUTTONDOWN) {
            mx = m.x; my = m.y;
            hit = gfxHitMenuCornerAction(mx, my);
            if (hit == GFX_MENU_ACTION_MUSIC || hit == GFX_MENU_ACTION_IMAGE
                || hit == GFX_MENU_ACTION_CLEAR_IMAGE || hit == GFX_MENU_ACTION_CLEAR_MUSIC) {
                return hit;
            }

            hit = gfxHitMenuButton(mx, my);
            if (hoverIndex) *hoverIndex = hit;
            if (hit > 0) return '0' + hit;
        }
        else if (m.message == WM_KEYDOWN) {
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxGetDeadInput(int *hoverIndex)
{
    ExMessage m;
    int hit;
    static const char keys[3] = { 'R', 'M', 'Q' };

    while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

        if (m.message == WM_MOUSEMOVE) {
            hit = gfxHitDeadButton(m.x, m.y);
            if (hoverIndex) *hoverIndex = hit;
        }
        else if (m.message == WM_LBUTTONDOWN) {
            hit = gfxHitDeadButton(m.x, m.y);
            if (hoverIndex) *hoverIndex = hit;
            if (hit > 0) return keys[hit - 1];
        }
        else if (m.message == WM_KEYDOWN) {
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxIsKeyDown(int vkcode)
{
    return (GetAsyncKeyState(vkcode) & 0x8000) != 0;
}


/* ============================================================
 *  从 gfx.cpp 合并
 * ============================================================ */
/* ============================================================
 *  gfx.cpp - 视图与输入实现
 *
 *  将原先 gfx.h 中全部实现迁移到本文件。
 *  本文件不改变游戏逻辑，仅负责窗口绘制、输入解包与
 *  与 UI 相关的状态展示。
 * ============================================================ */


/* ===================================================
 *  颜色定义（示例风格视觉方案）
 * =================================================== */

/* 背景 */
#define CLR_BG_LIGHT      RGB(56, 111, 179)
#define CLR_BG_DARK       RGB(14, 44, 87)
#define CLR_BG            RGB(26, 74, 133)
#define CLR_PANEL         RGB(33, 86, 150)
#define CLR_PANEL_SOFT    RGB(45, 104, 173)
#define CLR_PANEL_LINE    RGB(97, 153, 206)
#define CLR_PANEL_INNER   RGB(27, 82, 148)
#define CLR_PANEL_DK      RGB(12, 52, 101)
#define CLR_PANEL_GLOW    RGB(117, 191, 255)

/* 棋盘网格 */
#define CLR_GRID_BASE     RGB(24, 116, 224)
#define CLR_GRID_LINE     RGB(56, 146, 238)
#define CLR_GRID_LIGHT    RGB(37, 129, 231)
#define CLR_GRID_DARK     RGB(19, 98, 206)

/* 棋盘边框 */
#define CLR_BOARD_EDGE    RGB(80, 146, 220)
#define CLR_BOARD_EDGE_DN RGB(30, 84, 156)
#define CLR_WALL          RGB(90, 151, 230)
#define CLR_WALL_LINE     RGB(63, 112, 195)

/* 棋盘底色 */
#define CLR_BOARD_BASE    RGB(38, 135, 235)

/* 障碍物 */
#define CLR_OBS           RGB(112, 128, 175)
#define CLR_OBS_CRACK     RGB(84, 102, 145)

/* P1 蛇 */
#define CLR_S1_H          RGB(32, 205, 130)
#define CLR_S1_B1         RGB(28, 170, 95)
#define CLR_S1_B2         RGB(24, 132, 75)
#define CLR_S1_B3         RGB(19, 102, 60)

/* 橙色选项（替代原蓝色） */
#define CLR_S1B_H         RGB(255, 170, 20)
#define CLR_S1B_B1        RGB(255, 132, 20)
#define CLR_S1B_B2        RGB(245, 104, 12)
#define CLR_S1B_B3        RGB(210, 74, 8)

#define CLR_S1P_H         RGB(228, 92, 255)
#define CLR_S1P_B1        RGB(195, 66, 227)
#define CLR_S1P_B2        RGB(168, 41, 197)
#define CLR_S1P_B3        RGB(138, 28, 162)

/* P2 蛇 */
#define CLR_S2_H          RGB(255, 196, 70)
#define CLR_S2_B1         RGB(225, 169, 57)
#define CLR_S2_B2         RGB(195, 141, 48)
#define CLR_S2_B3         RGB(160, 115, 35)

/* AI 蛇（紫色） */
#define CLR_AI_H          RGB(190, 130, 255)
#define CLR_AI_B1         RGB(158, 103, 230)
#define CLR_AI_B2         RGB(128, 84, 198)
#define CLR_AI_B3         RGB(96, 65, 158)

/* 食物 */
#define CLR_BLUE_FOOD     RGB(255, 226, 96)
#define CLR_BLUE_GLOW     RGB(255, 246, 173)
#define CLR_RED_FOOD      RGB(255, 94, 102)
#define CLR_RED_GLOW      RGB(255, 150, 160)

/* 水果配色 */
#define CLR_BLUEBERRY      RGB(72, 92, 200)
#define CLR_BLUEBERRY_HL   RGB(170, 200, 255)
#define CLR_BLUEBERRY_DARK RGB(40, 60, 140)
#define CLR_BLUEBERRY_LEAF RGB(80, 180, 80)
#define CLR_STRAWBERRY     RGB(230, 50, 70)
#define CLR_STRAWBERRY_HL  RGB(255, 170, 180)
#define CLR_STRAWBERRY_DK  RGB(150, 25, 40)
#define CLR_STRAWBERRY_LEAF RGB(70, 190, 80)
#define CLR_STRAWBERRY_SEED RGB(255, 220, 130)

/* UI 多彩点缀 */
#define CLR_ACCENT_CYAN    RGB(94, 220, 230)
#define CLR_ACCENT_LIME    RGB(180, 230, 90)
#define CLR_ACCENT_PINK    RGB(255, 130, 180)
#define CLR_ACCENT_AMBER   RGB(255, 200, 80)
#define CLR_ACCENT_GOLD    RGB(255, 215, 90)

/* 道具 */
#define CLR_TURBO         RGB(255, 214, 60)
#define CLR_TURBO_EDGE    RGB(255, 246, 168)
#define CLR_SHIELD        RGB(84, 197, 255)
#define CLR_SHIELD_EDGE   RGB(171, 230, 255)
#define CLR_SLOW          RGB(200, 119, 255)
#define CLR_SLOW_EDGE     RGB(230, 110, 110)
#define CLR_MAGNET        RGB(255, 131, 107)
#define CLR_MAGNET_EDGE   RGB(255, 188, 168)
#define CLR_FREEZE        RGB(138, 229, 255)
#define CLR_FREEZE_EDGE   RGB(220, 244, 255)
#define CLR_SHRINK        RGB(255, 118, 118)
#define CLR_SHRINK_EDGE   RGB(255, 110, 110)
#define CLR_GHOST         RGB(230, 230, 255)
#define CLR_GHOST_EDGE    RGB(180, 196, 255)
#define CLR_DOUBLE        RGB(255, 214, 89)
#define CLR_DOUBLE_EDGE   RGB(255, 239, 184)
#define CLR_NEG_BORDER    RGB(255, 120, 130)

/* 文字 */
#define CLR_TITLE         RGB(243, 252, 255)
#define CLR_TEXT          RGB(229, 235, 248)
#define CLR_GOLD          RGB(255, 215, 93)
#define CLR_HINT          RGB(168, 185, 217)

/* 按钮边框 */
#define CLR_BORDER        RGB(83, 101, 141)
#define CLR_BORDER_IN     RGB(34, 47, 72)
#define CLR_BORDER_HI     RGB(121, 160, 255)

/* 字体 */
#define FONT_TITLE _T("Microsoft YaHei")
#define FONT_UI    _T("Microsoft YaHei")
#define FONT_MONO  _T("Consolas")
#define FONT_SCALE(sz) (((sz) * 6 / 5) + 1)

/* ===================================================
 *  主题状态 - 仅现代主题
 * =================================================== */

static void drawTextCenter(int x, int y, int w, int h, LPCTSTR text, COLORREF color);
static void drawCheckerboard(void);

/* ===================================================
 *  菜单按钮命中缓存
 * =================================================== */
#define MENU_HIT_MAX 8

typedef struct {
    int x;
    int y;
    int w;
    int h;
} MenuHitRect;

static MenuHitRect s_menuBtns[MENU_HIT_MAX];
static int s_menuBtnCount = 0;

static void resetMenuButtons(void)
{
    s_menuBtnCount = 0;
}

static void addMenuButtonRect(int x, int y, int w, int h, int index)
{
    if (index >= 1 && index <= MENU_HIT_MAX) {
        s_menuBtns[index - 1].x = x;
        s_menuBtns[index - 1].y = y;
        s_menuBtns[index - 1].w = w;
        s_menuBtns[index - 1].h = h;
        if (index > s_menuBtnCount) s_menuBtnCount = index;
    }
}

int gfxHitMenuButton(int mx, int my)
{
    int i;
    for (i = 0; i < s_menuBtnCount; i++) {
        if (mx >= s_menuBtns[i].x && mx <= s_menuBtns[i].x + s_menuBtns[i].w &&
            my >= s_menuBtns[i].y && my <= s_menuBtns[i].y + s_menuBtns[i].h) {
            return i + 1;
        }
    }
    return 0;
}

/* ===================================================
 *  自定义角落按钮（背景图 / 背景音乐）
 * =================================================== */
#define MENU_CORNER_BTN_W       38
#define MENU_CORNER_BTN_H       38
#define MENU_CORNER_MARGIN_X    12
#define MENU_CORNER_MARGIN_Y    12
#define MENU_CORNER_BTN_GAP      8

static int s_menuHasBgImage = 0;
static IMAGE s_menuBgImage;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} MenuCornerButton;

static MenuCornerButton s_menuImageBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuMusicBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuClearImageBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuClearMusicBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};

static int fileExistsInFileSystem(const char *path)
{
    return (path != NULL && *path != '\0' && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES);
}

static int pointInRect(int x, int y, const MenuCornerButton *r)
{
    if (!r) return 0;
    return x >= r->x && x <= r->x + r->w && y >= r->y && y <= r->y + r->h;
}

static void updateCornerButtonRects(void)
{
    s_menuImageBtn.x = MENU_CORNER_MARGIN_X;
    s_menuImageBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuImageBtn.w = MENU_CORNER_BTN_W;
    s_menuImageBtn.h = MENU_CORNER_BTN_H;

    s_menuClearImageBtn.x = s_menuImageBtn.x + MENU_CORNER_BTN_W + MENU_CORNER_BTN_GAP;
    s_menuClearImageBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuClearImageBtn.w = MENU_CORNER_BTN_W;
    s_menuClearImageBtn.h = MENU_CORNER_BTN_H;

    s_menuClearMusicBtn.x = WIN_W - 2 * MENU_CORNER_BTN_W - MENU_CORNER_BTN_GAP - MENU_CORNER_MARGIN_X;
    s_menuClearMusicBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuClearMusicBtn.w = MENU_CORNER_BTN_W;
    s_menuClearMusicBtn.h = MENU_CORNER_BTN_H;

    s_menuMusicBtn.x = WIN_W - MENU_CORNER_BTN_W - MENU_CORNER_MARGIN_X;
    s_menuMusicBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuMusicBtn.w = MENU_CORNER_BTN_W;
    s_menuMusicBtn.h = MENU_CORNER_BTN_H;
}

int gfxHitMenuCornerAction(int mx, int my)
{
    updateCornerButtonRects();
    if (pointInRect(mx, my, &s_menuImageBtn)) {
        return GFX_MENU_ACTION_IMAGE;
    }
    if (pointInRect(mx, my, &s_menuClearImageBtn)) {
        return GFX_MENU_ACTION_CLEAR_IMAGE;
    }
    if (pointInRect(mx, my, &s_menuClearMusicBtn)) {
        return GFX_MENU_ACTION_CLEAR_MUSIC;
    }
    if (pointInRect(mx, my, &s_menuMusicBtn)) {
        return GFX_MENU_ACTION_MUSIC;
    }
    return 0;
}

void gfxSetMenuBackgroundImage(const char *path)
{
    if (path == NULL || *path == '\0') {
        gfxClearMenuBackgroundImage();
        return;
    }

    if (!fileExistsInFileSystem(path)) {
        return;
    }

    loadimage(&s_menuBgImage, path, WIN_W, WIN_H, true);
    s_menuHasBgImage = 1;
}

void gfxClearMenuBackgroundImage(void)
{
    s_menuHasBgImage = 0;
}

int gfxHasMenuBackgroundImage(void)
{
    return s_menuHasBgImage;
}

static void drawMenuCornerButtons(void)
{
    MenuCornerButton imgBtn;
    MenuCornerButton musicBtn;
    MenuCornerButton clearImageBtn;
    MenuCornerButton clearMusicBtn;

    updateCornerButtonRects();
    imgBtn = s_menuImageBtn;
    musicBtn = s_menuMusicBtn;
    clearImageBtn = s_menuClearImageBtn;
    clearMusicBtn = s_menuClearMusicBtn;

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    setbkmode(TRANSPARENT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(imgBtn.x, imgBtn.y, imgBtn.x + imgBtn.w, imgBtn.y + imgBtn.h);

    setfillcolor(s_menuHasBgImage ? CLR_PANEL_GLOW : CLR_HINT);
    solidcircle(imgBtn.x + imgBtn.w / 2, imgBtn.y + imgBtn.h / 2, 10);
    settextcolor(CLR_TEXT);
    drawTextCenter(imgBtn.x, imgBtn.y, imgBtn.w, imgBtn.h, _T("图"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(clearImageBtn.x, clearImageBtn.y, clearImageBtn.x + clearImageBtn.w, clearImageBtn.y + clearImageBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(clearImageBtn.x, clearImageBtn.y, clearImageBtn.w, clearImageBtn.h, _T("清图"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(clearMusicBtn.x, clearMusicBtn.y, clearMusicBtn.x + clearMusicBtn.w, clearMusicBtn.y + clearMusicBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(clearMusicBtn.x, clearMusicBtn.y, clearMusicBtn.w, clearMusicBtn.h, _T("清音"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(musicBtn.x, musicBtn.y, musicBtn.x + musicBtn.w, musicBtn.y + musicBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(musicBtn.x, musicBtn.y, musicBtn.w, musicBtn.h, _T("音"), CLR_TEXT);
}

/* ===================================================
 *  内部绘制辅助
 * =================================================== */

static void drawTextCenter(int x, int y, int w, int h, LPCTSTR text, COLORREF color)
{
    RECT r;
    r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
    setbkmode(TRANSPARENT);
    settextcolor(color);
    drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static COLORREF blendColor(COLORREF src, COLORREF dst, float t)
{
    int sr = GetRValue(src);
    int sg = GetGValue(src);
    int sb = GetBValue(src);
    int dr = GetRValue(dst);
    int dg = GetGValue(dst);
    int db = GetBValue(dst);

    if (t <= 0.0f) return src;
    if (t >= 1.0f) return dst;

    return RGB(
        clampi(sr + (int)((dr - sr) * t), 0, 255),
        clampi(sg + (int)((dg - sg) * t), 0, 255),
        clampi(sb + (int)((db - sb) * t), 0, 255)
    );
}

static COLORREF boardGlowAt(const COLORREF baseColor, int x, int y, int cx, int cy, int maxDist)
{
    float dx = (float)(x - cx);
    float dy = (float)(y - cy);
    float d = sqrtf(dx * dx + dy * dy);
    float k = 1.0f - d / (float)maxDist;

    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    k = 0.05f + 0.12f * (k * k);

    return blendColor(baseColor, RGB(230, 250, 255), k);
}

static void drawCheckerboard(void)
{
    int y;

    for (y = 0; y < WIN_H; y += 8) {
        int top = y * 3 / WIN_H;
        int r = GetRValue(CLR_BG_DARK) - 4 - top;
        int g = GetGValue(CLR_BG_DARK) + top * 2;
        int b = GetBValue(CLR_BG) + top * 3;

        if (r < 0) r = 0;
        if (g > 255) g = 255;
        if (g < 0) g = 0;
        if (b > 255) b = 255;
        if (b < 0) b = 0;

        setfillcolor(RGB(r, g, b));
        solidrectangle(0, y, WIN_W, y + 8);
    }
}

static void drawMapChecker(int ox, int oy, int mapSize)
{
    int x, y;
    int mapPx = mapSize * CELL_PX;
    int cx = ox + mapPx / 2;
    int cy = oy + mapPx / 2;
    int maxDist = (int)(sqrtf((float)(mapPx * mapPx) * 0.5f) + 1);
    COLORREF sepColor;

    for (y = 0; y < mapSize; y++) {
        for (x = 0; x < mapSize; x++) {
            int left = ox + x * CELL_PX;
            int top = oy + y * CELL_PX;
            int cellCx = left + CELL_PX / 2;
            int cellCy = top + CELL_PX / 2;
            COLORREF fill = boardGlowAt(CLR_BOARD_BASE, cellCx, cellCy, cx, cy, maxDist);

            setfillcolor(fill);
            solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
        }
    }

    sepColor = blendColor(CLR_PANEL_GLOW, RGB(8, 28, 68), 0.22f);
    setlinecolor(sepColor);
    setlinestyle(PS_SOLID, 1);

    for (x = 1; x < mapSize; x++) {
        line(ox + x * CELL_PX, oy + 1, ox + x * CELL_PX, oy + mapPx - 1);
    }
    for (y = 1; y < mapSize; y++) {
        line(ox + 1, oy + y * CELL_PX, ox + mapPx - 1, oy + y * CELL_PX);
    }
}

static void drawWoodFrame(int ox, int oy, int mapSize)
{
    int frameThick = 12;
    int x1 = ox - frameThick;
    int y1 = oy - frameThick;
    int x2 = ox + mapSize * CELL_PX + frameThick;
    int y2 = oy + mapSize * CELL_PX + frameThick;

    setfillcolor(CLR_BOARD_EDGE_DN);
    solidrectangle(x1, y1, x2, oy);
    solidrectangle(x1, oy + mapSize * CELL_PX, x2, y2);
    solidrectangle(x1, oy, ox, oy + mapSize * CELL_PX);
    solidrectangle(ox + mapSize * CELL_PX, oy, x2, oy + mapSize * CELL_PX);

    setfillcolor(CLR_BOARD_EDGE);
    solidrectangle(x1 + 2, y1 + 2, x2 - 2, y2 - 2);

    setfillcolor(CLR_PANEL);
    solidrectangle(ox, oy, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);

    setlinecolor(CLR_BOARD_EDGE_DN);
    rectangle(ox - 1, oy - 1, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);
    setlinecolor(CLR_PANEL_LINE);
    rectangle(ox - frameThick, oy - frameThick, x2, y2);
}

static void drawPixelBorder(int x, int y, int w, int h, COLORREF outer, COLORREF inner)
{
    setlinecolor(outer);
    rectangle(x, y, x + w, y + h);
    setlinecolor(inner);
    rectangle(x + 2, y + 2, x + w - 2, y + h - 2);
}

static void drawPixelPanel(int x, int y, int w, int h, COLORREF fill)
{
    setfillcolor(CLR_PANEL_DK);
    solidrectangle(x - 2, y - 2, x + w + 2, y + h + 2);
    setfillcolor(CLR_PANEL_LINE);
    solidrectangle(x, y, x + w, y + h);
    setfillcolor(fill);
    solidrectangle(x + 2, y + 2, x + w - 2, y + h - 2);
}

static void drawPixelSnakeHead(int left, int top, COLORREF headColor, int dir)
{
    int cx, cy;
    int eye1x, eye2x, eye1y, eye2y;
    int eyeLen;

    setfillcolor(headColor);
    solidrectangle(left + 1, top + 1, left + CELL_PX - 1, top + CELL_PX - 1);

    cx = left + CELL_PX / 2;
    cy = top + CELL_PX / 2;
    eyeLen = CELL_PX / 4;

    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 2);

    if (dir == DIR_UP) {
        /* 相对中心线(x=cx)左右对称，朝上时在头部上半区 */
        eye1x = cx - CELL_PX / 6;
        eye2x = cx + CELL_PX / 6;
        eye1y = top + CELL_PX / 3;
        eye2y = eye1y;
        line(eye1x, eye1y, eye1x, eye1y + eyeLen);
        line(eye2x, eye2y, eye2x, eye2y + eyeLen);
    } else if (dir == DIR_DOWN) {
        /* 相对中心线(x=cx)左右对称，朝下时在头部下半区 */
        eye1x = cx - CELL_PX / 6;
        eye2x = cx + CELL_PX / 6;
        eye1y = top + CELL_PX * 2 / 3 - eyeLen;
        eye2y = eye1y;
        line(eye1x, eye1y, eye1x, eye1y + eyeLen);
        line(eye2x, eye2y, eye2x, eye2y + eyeLen);
    } else if (dir == DIR_LEFT) {
        /* 相对中心线(y=cy)上下对称，朝左时眼睛在左前方 */
        eye1y = cy - CELL_PX / 6;
        eye2y = cy + CELL_PX / 6;
        eye1x = left + CELL_PX / 3;
        eye2x = eye1x;
        line(eye1x, eye1y, eye1x + eyeLen, eye1y);
        line(eye2x, eye2y, eye2x + eyeLen, eye2y);
    } else {
        /* 相对中心线(y=cy)上下对称，朝右时眼睛在右前方 */
        eye1y = cy - CELL_PX / 6;
        eye2y = cy + CELL_PX / 6;
        eye1x = left + CELL_PX * 2 / 3 - eyeLen;
        eye2x = eye1x;
        line(eye1x, eye1y, eye1x + eyeLen, eye1y);
        line(eye2x, eye2y, eye2x + eyeLen, eye2y);
    }

    setlinestyle(PS_SOLID, 1);
}

static void drawPixelSnakeBody(int left, int top, COLORREF color)
{
    setfillcolor(color);
    solidrectangle(left + 1, top + 1, left + CELL_PX - 1, top + CELL_PX - 1);
}

static void drawPixelWall(int left, int top)
{
    setfillcolor(CLR_WALL);
    solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
}

static void drawPixelObs(int left, int top)
{
    int pad = 4;
    setfillcolor(CLR_OBS);
    solidrectangle(left + pad, top + pad, left + CELL_PX - pad, top + CELL_PX - pad);
}

static void drawPixelGemFood(int left, int top, COLORREF fill, COLORREF glow, COLORREF ambient)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int outer = CELL_PX / 2 - 2;
    int inner = CELL_PX / 2 - 5;
    int i;
    int radius;
    POINT outPts[4];
    POINT inPts[4];

    for (i = 0; i < 4; i++) {
        radius = outer + 2 + i;
        COLORREF haloColor = blendColor(ambient, glow, 0.78f - i * 0.18f);

        setfillcolor(haloColor);
        setlinecolor(haloColor);
        solidcircle(cx, cy, radius);
    }

    outPts[0].x = cx;
    outPts[0].y = cy - outer;
    outPts[1].x = cx + outer;
    outPts[1].y = cy;
    outPts[2].x = cx;
    outPts[2].y = cy + outer;
    outPts[3].x = cx - outer;
    outPts[3].y = cy;

    inPts[0].x = cx;
    inPts[0].y = cy - inner;
    inPts[1].x = cx + inner;
    inPts[1].y = cy;
    inPts[2].x = cx;
    inPts[2].y = cy + inner;
    inPts[3].x = cx - inner;
    inPts[3].y = cy;

    setlinecolor(blendColor(fill, RGB(255, 255, 255), 0.18f));
    setfillcolor(blendColor(fill, RGB(255, 255, 255), 0.35f));
    solidpolygon(outPts, 4);

    setlinecolor(blendColor(fill, RGB(0, 0, 0), 0.12f));
    setfillcolor(fill);
    solidpolygon(inPts, 4);

    setlinecolor(blendColor(fill, RGB(255, 255, 255), 0.45f));
    line(cx - inner + 2, cy, cx, cy - inner + 3);
    line(cx, cy - inner + 3, cx + inner - 2, cy);
    line(cx + inner - 2, cy, cx, cy + inner - 3);
    line(cx, cy + inner - 3, cx - inner + 2, cy);

    setlinecolor(blendColor(fill, RGB(0, 0, 0), 0.35f));
    line(cx - inner + 2, cy + 2, cx + inner - 2, cy + 2);

    setlinecolor(fill);
    setfillcolor(blendColor(fill, RGB(255, 255, 255), 0.32f));
    POINT spark[3];
    spark[0].x = cx;
    spark[0].y = cy - outer + 2;
    spark[1].x = cx - 1;
    spark[1].y = cy;
    spark[2].x = cx;
    spark[2].y = cy + 2;
    solidpolygon(spark, 3);
}

static void drawPixelBlueFood(int left, int top, COLORREF ambient)
{
    (void)ambient;
    /* 蓝莓：深紫实心圆 + 高光斑 + 顶部一片小绿叶 */
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX * 2 / 3;
    int r = CELL_PX * 2 / 5;

    /* 暗光晕 */
    setfillcolor(CLR_BLUEBERRY_DARK);
    setlinecolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy, r + 1);

    /* 主体 */
    setfillcolor(CLR_BLUEBERRY);
    setlinecolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy, r);

    /* 高光斑（小白点） */
    setfillcolor(CLR_BLUEBERRY_HL);
    setlinecolor(CLR_BLUEBERRY_HL);
    solidcircle(cx - r / 3, cy - r / 3, r / 4);

    /* 顶部小绿叶（三角形） */
    {
        POINT leaf[3];
        leaf[0].x = cx;
        leaf[0].y = cy - r - 1;
        leaf[1].x = cx - r / 2;
        leaf[1].y = cy - r / 4;
        leaf[2].x = cx + r / 2;
        leaf[2].y = cy - r / 4;
        setfillcolor(CLR_BLUEBERRY_LEAF);
        setlinecolor(CLR_BLUEBERRY_LEAF);
        solidpolygon(leaf, 3);
    }

    /* 底部小果蒂（深色） */
    setfillcolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy + r - 1, 2);
}

static void drawPixelRedFood(int left, int top, COLORREF ambient)
{
    (void)ambient;
    /* 草莓：红色三角/倒锥形 + 顶部绿叶 + 黄色籽点 */
    int cx = left + CELL_PX / 2;
    int topY = top + CELL_PX / 3;
    int bottomY = top + CELL_PX - 2;
    int halfW = CELL_PX / 3;
    POINT body[3];

    body[0].x = cx - halfW; body[0].y = topY + 2;
    body[1].x = cx + halfW; body[1].y = topY + 2;
    body[2].x = cx;        body[2].y = bottomY;

    /* 主体深底 */
    POINT bodyShadow[3];
    bodyShadow[0].x = body[0].x - 1; bodyShadow[0].y = body[0].y - 1;
    bodyShadow[1].x = body[1].x + 1; bodyShadow[1].y = body[1].y - 1;
    bodyShadow[2].x = body[2].x;     bodyShadow[2].y = body[2].y + 1;
    setfillcolor(CLR_STRAWBERRY_DK);
    setlinecolor(CLR_STRAWBERRY_DK);
    solidpolygon(bodyShadow, 3);

    /* 主体红 */
    setfillcolor(CLR_STRAWBERRY);
    setlinecolor(CLR_STRAWBERRY_DK);
    solidpolygon(body, 3);

    /* 高光 */
    setfillcolor(CLR_STRAWBERRY_HL);
    setlinecolor(CLR_STRAWBERRY_HL);
    POINT hl[3] = {{cx - halfW/2, topY + 4}, {cx - halfW/4, topY + 4}, {cx - halfW/2, topY + halfW + 2}};
    solidpolygon(hl, 3);

    /* 顶部绿叶（5 片三角形围成） */
    setfillcolor(CLR_STRAWBERRY_LEAF);
    setlinecolor(CLR_STRAWBERRY_LEAF);
    solidcircle(cx, topY + 1, 4);
    /* 旁两片小叶 */
    {
        POINT side[3] = {{cx - 5, topY + 2}, {cx - 1, topY - 2}, {cx - 1, topY + 4}};
        POINT side2[3] = {{cx + 5, topY + 2}, {cx + 1, topY - 2}, {cx + 1, topY + 4}};
        solidpolygon(side, 3);
        solidpolygon(side2, 3);
    }

    /* 黄色籽点（5 颗小黄点散布在主体上） */
    setfillcolor(CLR_STRAWBERRY_SEED);
    setlinecolor(CLR_STRAWBERRY_SEED);
    solidcircle(cx - 3, topY + 9, 1);
    solidcircle(cx + 4, topY + 10, 1);
    solidcircle(cx - 5, topY + 14, 1);
    solidcircle(cx + 2, topY + 15, 1);
    solidcircle(cx + 6, topY + 15, 1);
}

/* 绘制道具 — 彩色圆 + 字母标识 + 区分边框 */
static void drawPixelItem(int left, int top, int itemType)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int r = CELL_PX / 2 - 4;
    COLORREF fill, edge;
    TCHAR symbol[2] = { _T('?'), 0 };

    switch (itemType) {
    case ITEM_TURBO:
        fill = CLR_TURBO; edge = CLR_TURBO_EDGE; symbol[0] = _T('Z'); break;
    case ITEM_SHIELD:
        fill = CLR_SHIELD; edge = CLR_SHIELD_EDGE; symbol[0] = _T('S'); break;
    case ITEM_SLOW:
        fill = CLR_SLOW; edge = CLR_NEG_BORDER;  symbol[0] = _T('-'); break;
    case ITEM_MAGNET:
        fill = CLR_MAGNET; edge = CLR_MAGNET_EDGE; symbol[0] = _T('M'); break;
    case ITEM_FREEZE:
        fill = CLR_FREEZE; edge = CLR_FREEZE_EDGE; symbol[0] = _T('F'); break;
    case ITEM_SHRINK:
        fill = CLR_SHRINK; edge = CLR_NEG_BORDER;  symbol[0] = _T('X'); break;
    case ITEM_GHOST:
        fill = CLR_GHOST; edge = CLR_GHOST_EDGE; symbol[0] = _T('G'); break;
    case ITEM_DOUBLE:
        fill = CLR_DOUBLE; edge = CLR_DOUBLE_EDGE; symbol[0] = _T('2'); break;
    default: return;
    }

    /* 外圈（亮色或红框） */
    setlinecolor(edge);
    setfillcolor(fill);
    solidcircle(cx, cy, r);

    /* 内圈字母 */
    setbkmode(TRANSPARENT);
    settextcolor(itemIsNegative(itemType) ? CLR_NEG_BORDER : RGB(255, 255, 255));
    settextstyle(FONT_SCALE(14), 0, _T("Microsoft YaHei"));
    {
        RECT rect = { left, top, left + CELL_PX, top + CELL_PX };
        drawtext(symbol, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

/* 绘制效果状态指示器（顶部状态栏） */
/* 绘制飘字 */
static void drawFloatTexts(const GameState *g, int ox, int oy)
{
    int i;
    for (i = 0; i < MAX_FLOAT_TEXTS; i++) {
        const FloatText *ft = &g->floatTexts[i];
        if (ft->life > 0.0f) {
            int alpha = (int)(ft->life * 212.0f);  /* 淡出 */
            if (alpha > 255) alpha = 255;
            settextstyle(FONT_SCALE(16), 0, _T("Consolas"));
            setbkmode(TRANSPARENT);
            settextcolor(RGB(
                (GetRValue(ft->color) * alpha) / 255,
                (GetGValue(ft->color) * alpha) / 255,
                (GetBValue(ft->color) * alpha) / 255));
            {
                int px = ox + (int)(ft->x * CELL_PX);
                int py = oy + (int)(ft->y * CELL_PX);
                outtextxy(px, py, ft->text);
            }
        }
    }
}

/* 成就解锁通知 */
static void drawAchNotification(const GameState *g)
{
    if (g->achNewUnlock) {
        int i, y = 80;
        settextstyle(FONT_SCALE(22), 0, _T("Microsoft YaHei"));
        setbkmode(TRANSPARENT);
        for (i = 0; i < ACH_MAX; i++) {
            if (g->achNewUnlock & (1u << i)) {
                TCHAR buf[64];
                _stprintf(buf, _T("成就解锁: %s!"), achName(i));
                settextcolor(CLR_GOLD);
                outtextxy(WIN_W / 2 - 120, y, buf);
                y += 26;
            }
        }
    }
    ((GameState *)g)->achNewUnlock = 0;
}

static void drawEffectIndicators(const GameState *g, int player, int startX, int startY)
{
    TCHAR buf[32];
    int y = startY;
    int count = 0;

    settextstyle(FONT_SCALE(13), 0, FONT_UI);

    if (itemHasEffect(g, player, EFF_TURBO)) {
        settextcolor(CLR_TURBO);
        _stprintf(buf, _T("极速: %.0fs"), itemTimer(g, player, EFF_TURBO));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_SLOW)) {
        settextcolor(CLR_SLOW);
        _stprintf(buf, _T("减速: %.0fs"), itemTimer(g, player, EFF_SLOW));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_MAGNET)) {
        settextcolor(CLR_MAGNET);
        _stprintf(buf, _T("磁铁: %.0fs"), itemTimer(g, player, EFF_MAGNET));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_GHOST)) {
        settextcolor(CLR_GHOST);
        _stprintf(buf, _T("穿墙: %.0fs"), itemTimer(g, player, EFF_GHOST));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_DOUBLE)) {
        settextcolor(CLR_DOUBLE);
        _stprintf(buf, _T("x2: %.0fs"), itemTimer(g, player, EFF_DOUBLE));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_FROZEN)) {
        settextcolor(CLR_FREEZE);
        _stprintf(buf, _T("冻结: %.0fs"), itemTimer(g, player, EFF_FROZEN));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemShieldCount(g, player) > 0) {
        settextcolor(CLR_SHIELD);
        _stprintf(buf, _T("护盾 x%d"), itemShieldCount(g, player));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }

    if (count == 0) {
        settextcolor(CLR_HINT);
        _stprintf(buf, _T("无状态"));
        outtextxy(startX, y, buf);
    }
}

static void drawPixelButton(int index, LPCTSTR text, int hover)
{
    int x = MENU_BTN_X;
    int y = MENU_BTN_START_Y + (index - 1) * MENU_BTN_GAP;
    int w = MENU_BTN_W;
    int h = MENU_BTN_H;

    COLORREF border = hover ? CLR_BORDER_HI : CLR_BORDER;
    COLORREF fill = hover ? CLR_PANEL_SOFT : CLR_PANEL;

    addMenuButtonRect(x, y, w, h, index);
    drawPixelPanel(x, y, w, h, fill);
    setlinecolor(border);
    rectangle(x + 2, y + 2, x + w - 2, y + h - 2);

    /* 左上角色块点缀：5 个按钮各分配一个主题色 */
    {
        COLORREF accent = CLR_ACCENT_CYAN;
        switch (index) {
            case 1: accent = CLR_ACCENT_CYAN;  break;  /* 单人: 青 */
            case 2: accent = CLR_ACCENT_PINK;  break;  /* 双人: 粉 */
            case 3: accent = CLR_ACCENT_AMBER; break;  /* 玩法: 琥珀 */
            case 4: accent = CLR_ACCENT_LIME;  break;  /* 设置: 嫩绿 */
            case 5: accent = CLR_ACCENT_GOLD;  break;  /* 成就: 金 */
            default: break;
        }
        setfillcolor(accent);
        setlinecolor(accent);
        /* hover 时稍大；常态小圆点 */
        solidcircle(x + 14, y + 14, hover ? 7 : 5);
    }

    /* 右下角小三角点缀（hover 时出现） */
    if (hover) {
        POINT tri[3] = {
            { x + w - 16, y + h - 14 },
            { x + w - 6,  y + h - 14 },
            { x + w - 11, y + h - 6 }
        };
        setfillcolor(CLR_PANEL_GLOW);
        setlinecolor(CLR_PANEL_GLOW);
        solidpolygon(tri, 3);
    }

    settextstyle(FONT_SCALE(24), 0, FONT_UI);
    drawTextCenter(x, y, w, h, text, hover ? CLR_PANEL_GLOW : CLR_TEXT);
}

static void mapOrigin(const GameState *g, int *ox, int *oy)
{
    int mapSize = normalizeMapSize(g ? g->mapSize : MAP_SIZE_LARGE);
    int mapPx = mapSize * CELL_PX;

    *ox = (WIN_W - mapPx) / 2;
    *oy = 56 + (WIN_H - 56 - 36 - mapPx) / 2;
}

static int snakeBodyIndex(const Snake *s, int x, int y)
{
    int i;
    for (i = 0; i < s->len; i++) {
        if (s->body[i].x == x && s->body[i].y == y) return i;
    }
    return -1;
}

static int aiBodyIndex(const GameState *g, int x, int y)
{
    int a, j;
    for (a = 0; a < g->aiCount; a++) {
        const AISnake *ai = &g->ai[a];
        for (j = 0; j < ai->len; j++) {
            if (ai->body[j].x == x && ai->body[j].y == y) return j;
        }
    }
    return -1;
}

static COLORREF getSnakeBodyColor(int index, int len,
    COLORREF head, COLORREF b1, COLORREF b2, COLORREF b3)
{
    float ratio;
    int r, gC, bC;
    int r1 = GetRValue(b1), g1 = GetGValue(b1), bb1 = GetBValue(b1);
    int r3 = GetRValue(b3), g3 = GetGValue(b3), bb3 = GetBValue(b3);

    (void)b2;
    if (len <= 1) return head;
    if (len <= 2) return b1;
    ratio = (float)(index - 1) / (float)(len - 2);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    r  = r1  + (int)((r3 - r1) * ratio);
    gC = g1  + (int)((g3 - g1) * ratio);
    bC = bb1 + (int)((bb3 - bb1) * ratio);
    return RGB(r, gC, bC);
}

static COLORREF getP1BodyColor(int index, int len, int snakeColor)
{
    (void)index; (void)len;
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE)   return CLR_S1B_B1;
    if (snakeColor == SNAKE_COLOR_PURPLE) return CLR_S1P_B1;
    return CLR_S1_B1;
}

static COLORREF getP1HeadColor(int snakeColor)
{
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE) return CLR_S1B_H;
    if (snakeColor == SNAKE_COLOR_PURPLE) return CLR_S1P_H;
    return CLR_S1_H;
}

static COLORREF getP2BodyColor(int index, int len)
{
    (void)index; (void)len;
    return CLR_S2_B1;
}

static COLORREF getAIBodyColor(int index, int len)
{
    (void)index; (void)len;
    return CLR_AI_B1;
}

static LPCTSTR colorName(int snakeColor)
{
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE) return _T("橙色");
    if (snakeColor == SNAKE_COLOR_PURPLE) return _T("紫色");
    return _T("绿色");
}

static LPCTSTR mapSizeName(int mapSize)
{
    mapSize = normalizeMapSize(mapSize);
    if (mapSize == MAP_SIZE_SMALL) return _T("小(16x16)");
    if (mapSize == MAP_SIZE_MEDIUM) return _T("中(18x18)");
    return _T("大(20x20)");
}

static LPCTSTR difficultyName(int difficulty)
{
    if (difficulty == 0) return _T("简单");
    if (difficulty == 1) return _T("普通");
    return _T("困难");
}

static void drawTextLeft(int x, int y, LPCTSTR text, COLORREF color)
{
    setbkmode(TRANSPARENT);
    settextcolor(color);
    outtextxy(x, y, text);
}

static void drawOverlayPanel(int x, int y, int w, int h)
{
    drawPixelPanel(x, y, w, h, CLR_PANEL_SOFT);
    setlinecolor(CLR_PANEL_GLOW);
    line(x + 6, y + h + 1, x + w - 6, y + h + 1);
}

static void drawInfoPanel(int x, int y, int w, int h, LPCTSTR title, LPCTSTR content, COLORREF titleColor)
{
    drawPixelPanel(x, y, w, h, CLR_PANEL_INNER);

    if (title && *title) {
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(titleColor);
        outtextxy(x + 12, y + 8, title);
        setlinecolor(CLR_PANEL_LINE);
        line(x + 10, y + 28, x + w - 10, y + 28);
    }

    if (content && *content) {
        settextstyle(FONT_SCALE(14), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        outtextxy(x + 12, y + 48, content);
    }
}

static int s_deadBtnX = 0, s_deadBtnW = 0, s_deadBtnH = 28, s_deadBtnLineH = 32;
static int s_deadBtnY[3] = {0, 0, 0};

static void drawKeyHintsVertical(int px, int pw, int startY, int hoverIndex)
{
    int i;
    COLORREF fg[3] = { RGB(60, 200, 70), RGB(195, 130, 75), RGB(220, 80, 80) };
    LPCTSTR labels[3] = { _T("R 重新开始"), _T("M 返回菜单"), _T("Q 退出游戏") };

    s_deadBtnX = px + 30;
    s_deadBtnW = pw - 60;
    s_deadBtnH = 28;
    s_deadBtnLineH = 32;
    for (i = 0; i < 3; i++) s_deadBtnY[i] = startY + i * s_deadBtnLineH;

    settextstyle(FONT_SCALE(20), 0, _T("Microsoft YaHei"));
    for (i = 0; i < 3; i++) {
        if (hoverIndex == i + 1) {
            setfillcolor(RGB(60, 60, 60));
            solidrectangle(s_deadBtnX, s_deadBtnY[i], s_deadBtnX + s_deadBtnW, s_deadBtnY[i] + s_deadBtnH);
        }
        drawTextCenter(s_deadBtnX, s_deadBtnY[i], s_deadBtnW, s_deadBtnH, labels[i], fg[i]);
    }
}

int gfxHitDeadButton(int mx, int my)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (mx >= s_deadBtnX && mx <= s_deadBtnX + s_deadBtnW &&
            my >= s_deadBtnY[i] && my <= s_deadBtnY[i] + s_deadBtnH) {
            return i + 1;
        }
    }
    return 0;
}

/* 绘制单个地图网格（不含cleardevice和UI文字） */
static void drawMapAt(const GameState *g, int ox, int oy)
{
    int x, y, mapSize;
    int mapPx;
    if (!g) return;

    mapSize = normalizeMapSize(g->mapSize);
    mapPx = mapSize * CELL_PX;

    drawWoodFrame(ox, oy, mapSize);
    drawMapChecker(ox, oy, mapSize);

    {
        int mapRadius = (int)(sqrtf((float)(mapPx * mapPx) * 0.5f) + 1);
        int mapCx = ox + mapPx / 2;
        int mapCy = oy + mapPx / 2;

        for (y = 0; y < mapSize; y++) {
            for (x = 0; x < mapSize; x++) {
                int left = ox + x * CELL_PX;
                int top = oy + y * CELL_PX;
                int cellCx = left + CELL_PX / 2;
                int cellCy = top + CELL_PX / 2;
                int cell = g->grid[y][x];
                COLORREF tileBg = boardGlowAt(CLR_BOARD_BASE, cellCx, cellCy, mapCx, mapCy, mapRadius);

                if (cell == CELL_WALL) {
                    setfillcolor(CLR_WALL);
                    setlinecolor(CLR_WALL_LINE);
                    solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
                    rectangle(left, top, left + CELL_PX, top + CELL_PX);
                } else if (cell == CELL_OBS) {
                    drawPixelObs(left, top);
                } else if (cell == CELL_BLUE) {
                    drawPixelBlueFood(left, top, tileBg);
                } else if (cell == CELL_RED) {
                    drawPixelRedFood(left, top, tileBg);
                } else if (cell == CELL_ITEM) {
                    if (g->config.itemMode == GAMEPLAY_ITEM)
                        drawPixelItem(left, top, g->itemOnField);
                } else if (cell == CELL_SNAKE) {
                    int idx = snakeBodyIndex(&g->snake, x, y);
                    if (idx == 0) drawPixelSnakeHead(left, top, getP1HeadColor(g->config.snakeColor), g->snake.dir);
                    else if (idx > 0) drawPixelSnakeBody(left, top, getP1BodyColor(idx, g->snake.len, g->config.snakeColor));
                } else if (cell == CELL_SNAKE2) {
                    int idx = snakeBodyIndex(&g->snake2, x, y);
                    if (idx == 0) drawPixelSnakeHead(left, top, CLR_S2_H, g->snake2.dir);
                    else if (idx > 0) drawPixelSnakeBody(left, top, getP2BodyColor(idx, g->snake2.len));
                } else if (cell == CELL_AI) {
                    int idx = aiBodyIndex(g, x, y);
                    if (idx == 0) drawPixelSnakeHead(left, top, CLR_AI_H, DIR_UP);
                    else if (idx > 0) drawPixelSnakeBody(left, top, getAIBodyColor(idx, MAX_LEN));
                }
            }
        }
    }
}

static void drawGameContent(const GameState *g)
{
    int ox, oy, mapSize;
    TCHAR buf[128];

    if (!g) return;
    mapSize = normalizeMapSize(g->mapSize);
    mapOrigin(g, &ox, &oy);

    cleardevice();
    drawCheckerboard();

    /* 先画地图，再画计分板，避免地图外框遮住左上角计分板 */
    drawMapAt(g, ox, oy);

    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawInfoPanel(20, 12, 270, 92, _T("P1"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        if (g->gameMode == MODE_DUAL_TIMED) {
            _stprintf(buf, _T("时间: %d:%02d"), (int)(g->matchTimer / 60), (int)g->matchTimer % 60);
            outtextxy(36, 64, buf);
        }

        drawInfoPanel(810, 12, 270, 92, _T("P2"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score2);
        outtextxy(826, 42, buf);
        if (g->gameMode == MODE_DUAL_TIMED) {
            _stprintf(buf, _T("倒计时: %d:%02d"), (int)(g->matchTimer / 60), (int)g->matchTimer % 60);
            outtextxy(826, 64, buf);
        }
    } else {
        drawInfoPanel(20, 12, 320, 92, _T("游戏信息"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        _stprintf(buf, _T("最高: %d"), g->highScore);
        outtextxy(36, 64, buf);
    }

    drawInfoPanel(WIN_W - 190, 12, 180, 92, _T("难度/地图"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("难度: %s"), difficultyName(g->difficulty));
    outtextxy(WIN_W - 174, 42, buf);
    _stprintf(buf, _T("地图: %dx%d"), mapSize, mapSize);
    outtextxy(WIN_W - 174, 64, buf);

    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawEffectIndicators(g, 0, 830, 112);
        drawEffectIndicators(g, 1, 28, 112);
    } else {
        drawEffectIndicators(g, 0, WIN_W - 184, 112);
    }

    if (g->hasRed) {
        int barX = WIN_W - 184;
        int barY = 116;
        int barW = 156;
        int barH = 10;
        int filledW = (int)((float)barW * g->redTimer / (float)RED_TIMER_MAX);

        if (filledW < 0) filledW = 0;
        if (filledW > barW) filledW = barW;
        settextstyle(FONT_SCALE(14), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        outtextxy(barX, barY - 14, _T("红色食物"));
        setfillcolor(CLR_PANEL_DK);
        solidrectangle(barX, barY, barX + barW, barY + barH);
        setfillcolor(CLR_RED_FOOD);
        solidrectangle(barX, barY, barX + filledW, barY + barH);
    }

    settextstyle(FONT_SCALE(15), 0, FONT_UI);
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawTextLeft(20, WIN_H - 28, _T("P1:WASD+J  P2:方向键+0  P:暂停  Q:退出"), CLR_HINT);
    } else if (g->gameMode == MODE_DUAL_MIRROR) {
        /* 镜像模式底部提示由 gfxDrawMirrorGame 绘制 */
    } else if (g->gameMode == MODE_SURVIVAL) {
        if (g->config.itemMode == GAMEPLAY_ITEM)
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  道具:靠近自动拾取  生存中"), CLR_HINT);
        else
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  生存中"), CLR_HINT);
    } else {
        if (g->config.itemMode == GAMEPLAY_ITEM)
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  道具:靠近自动拾取  P:暂停  Q:退出"), CLR_HINT);
        else
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  P:暂停  Q:退出"), CLR_HINT);
    }

    drawFloatTexts(g, ox, oy);
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        int c1 = comboCountForPlayer(g, 0);
        int c2 = comboCountForPlayer(g, 1);
        if (c1 >= 2 || c2 >= 2) {
            settextstyle(FONT_SCALE(20), 0, FONT_UI);
            settextcolor(CLR_GOLD);
            if (c1 >= 2) {
                TCHAR cbuf[32];
                _stprintf(cbuf, _T("P1 COMBO x%d"), c1);
                outtextxy(20, 22, cbuf);
            }
            if (c2 >= 2) {
                TCHAR cbuf[32];
                _stprintf(cbuf, _T("P2 COMBO x%d"), c2);
                outtextxy(WIN_W - 170, 22, cbuf);
            }
        }
    } else if (comboCount(g) >= 2) {
        TCHAR cbuf[32];
        _stprintf(cbuf, _T("COMBO x%d !"), comboCount(g));
        settextstyle(FONT_SCALE(24), 0, FONT_UI);
        settextcolor(CLR_GOLD);
        outtextxy(WIN_W / 2 - 60, 22, cbuf);
    }

    if (g->gameMode == MODE_SURVIVAL) {
        TCHAR sbuf[32];
        int m = (int)g->elapsedTime / 60, s = (int)g->elapsedTime % 60;
        _stprintf(sbuf, _T("生存 %d:%02d"), m, s);
        settextstyle(FONT_SCALE(16), 0, FONT_MONO);
        settextcolor(CLR_PANEL_GLOW);
        outtextxy(WIN_W - 118, 112, sbuf);
    }

    drawAchNotification(g);
}

void gfxInit(void)
{
    initgraph(WIN_W, WIN_H);
    setbkcolor(CLR_BG_DARK);
    cleardevice();
    setfillcolor(CLR_BG_DARK);
    setlinecolor(CLR_BG_DARK);
    solidrectangle(0, 0, WIN_W, WIN_H);
    BeginBatchDraw();
}

void gfxClose(void)
{
    EndBatchDraw();
    closegraph();
}

void gfxDrawMenu(const GameState *g, int menuPage, int hoverIndex)
{
    TCHAR buf[128];

    resetMenuButtons();
    cleardevice();

    if (s_menuHasBgImage) {
        putimage(0, 0, &s_menuBgImage);
    } else {
        drawCheckerboard();
        setfillcolor(CLR_BG_DARK);
        solidrectangle(0, 0, WIN_W, 120);
    }

    drawMenuCornerButtons();

    setfillcolor(CLR_BG_DARK);
    solidrectangle(0, 0, WIN_W, 120);

    drawInfoPanel(160, 20, 780, 86, _T("复古贪吃蛇"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(44), 0, FONT_TITLE);
    settextcolor(CLR_PANEL_GLOW);
    setbkmode(TRANSPARENT);
    drawTextCenter(0, 36, WIN_W, 50, _T("贪吃蛇"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_UI);
    _stprintf(buf, _T("最高分: %d"), g ? g->highScore : 0);
    drawTextCenter(0, 80, WIN_W, 24, buf, CLR_TEXT);

    if (menuPage == MENU_MAIN) {
        drawPixelButton(1, _T("1  单人模式"), hoverIndex == 1);
        drawPixelButton(2, _T("2  双人对战"), hoverIndex == 2);
        drawPixelButton(3, _T("3  玩法说明"), hoverIndex == 3);
        drawPixelButton(4, _T("4  设置"), hoverIndex == 4);
        drawPixelButton(5, _T("5  成就查看"), hoverIndex == 5);

        drawTextCenter(0, 520, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_SINGLE_DIFF) {
        drawTextCenter(0, 150, WIN_W, 30, _T("选择单人玩法"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawPixelButton(4, _T("4  生存模式"), hoverIndex == 4);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_MODE) {
        drawTextCenter(0, 150, WIN_W, 30, _T("双人对战模式"), CLR_TEXT);
        drawPixelButton(1, _T("1  经典对战"), hoverIndex == 1);
        drawPixelButton(2, _T("2  限时赛"), hoverIndex == 2);
        drawPixelButton(3, _T("3  镜像对决"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_MIRROR) {
        drawTextCenter(0, 150, WIN_W, 30, _T("镜像对决 — 选择难度"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("两张相同地图各自独立"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_DIFF) {
        drawTextCenter(0, 150, WIN_W, 30, _T("选择难度"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("P1: WASD  |  P2: 方向键"), CLR_HINT);
    }
    else if (menuPage == MENU_SETTINGS) {
        int itemMode = g ? g->config.itemMode : GAMEPLAY_ITEM;
        int aiEnabled = g ? g->config.aiEnabled : 1;

        drawTextCenter(0, 150, WIN_W, 30, _T("游戏设置"), CLR_TEXT);
        _stprintf(buf, _T("1  蛇颜色: %s"), colorName(g ? g->config.snakeColor : SNAKE_COLOR_GREEN));
        drawPixelButton(1, buf, hoverIndex == 1);
        _stprintf(buf, _T("2  地图: %s"), mapSizeName(g ? g->config.mapSize : MAP_SIZE_LARGE));
        drawPixelButton(2, buf, hoverIndex == 2);
        _stprintf(buf, _T("3  玩法: %s"), itemMode == GAMEPLAY_ITEM ? _T("道具版") : _T("经典版"));
        drawPixelButton(3, buf, hoverIndex == 3);
        _stprintf(buf, _T("4  AI敌人(单人): %s"), aiEnabled ? _T("开启") : _T("关闭"));
        drawPixelButton(4, buf, hoverIndex == 4);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回"), CLR_HINT);
    }
    else if (menuPage == MENU_HOWTOPLAY) {
        drawTextCenter(0, 150, WIN_W, 30, _T("玩法说明"), CLR_PANEL_GLOW);

        /* === 经典模式 vs 道具模式 对比 === */
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        outtextxy(60, 195, _T("经典模式:无道具,纯净贪吃蛇体验;蓝色食物 +10 倍率"));
        outtextxy(60, 215, _T("道具模式:场上随机刷道具,8 种效果改变战局"));

        settextcolor(CLR_ACCENT_AMBER);
        outtextxy(60, 240, _T("── 8 种道具 ──"));

        /* 道具列表 */
        struct { LPCTSTR name; LPCTSTR desc; COLORREF c; } items[] = {
            { _T("极速 Z"), _T("速度×3,持续5秒"),         CLR_TURBO },
            { _T("护盾 S"), _T("免死1次,可叠3层,15秒"),   CLR_SHIELD },
            { _T("减速 -"), _T("速度×1.5,持续4秒"),        CLR_SLOW },
            { _T("磁铁 M"), _T("3格内自动吃蓝食,6秒"),    CLR_MAGNET },
            { _T("冻结 F"), _T("冻结对手3秒(双人)"),       CLR_FREEZE },
            { _T("缩短 X"), _T("蛇尾减3节,即时生效"),     CLR_SHRINK },
            { _T("穿墙 G"), _T("穿过自己身体,6秒"),       CLR_GHOST },
            { _T("双倍 2"), _T("得分×2,持续10秒"),         CLR_DOUBLE },
        };
        int y = 270;
        int i;
        for (i = 0; i < 8; i++) {
            /* 色块 */
            setfillcolor(items[i].c);
            setlinecolor(items[i].c);
            solidcircle(75, y + 6, 5);
            /* 名称 + 描述 */
            settextstyle(FONT_SCALE(14), 0, FONT_UI);
            settextcolor(CLR_TEXT);
            outtextxy(95, y, items[i].name);
            settextcolor(CLR_HINT);
            outtextxy(180, y, items[i].desc);
            y += 22;
        }

        /* AI 敌人特性 */
        y += 8;
        settextcolor(CLR_ACCENT_PINK);
        outtextxy(60, y, _T("── AI 敌人(仅单人模式)──"));
        y += 24;
        settextstyle(FONT_SCALE(13), 0, FONT_UI);
        settextcolor(CLR_HINT);
        outtextxy(60, y, _T("贪心寻路:始终朝最近蓝食物移动 (Manhattan 距离)"));
        y += 18;
        outtextxy(60, y, _T("随机生成:场上最多3条,间隔18-30秒"));
        y += 18;
        outtextxy(60, y, _T("击杀奖励:撞死AI按其长度×蓝食分数得分"));
        y += 18;
        outtextxy(60, y, _T("多人模式下默认关闭,避免和玩家蛇互相干扰"));

        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_ACHIEVEMENTS) {
        int i;
        int unlocked = 0;
        int y = 170;
        TCHAR status[12];

        if (g) {
            for (i = 0; i < ACH_MAX; i++) {
                if (g->achUnlocked & (1u << i)) unlocked++;
            }
        }

        drawTextCenter(0, 150, WIN_W, 30, _T("成就查看"), CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(18), 0, FONT_UI);
        _stprintf(buf, _T("已解锁: %d/%d"), unlocked, ACH_MAX);
        drawTextCenter(0, 178, WIN_W, 24, buf, CLR_GOLD);

        for (i = 0; i < ACH_MAX; i++) {
            COLORREF color = CLR_HINT;
            if (g && (g->achUnlocked & (1u << i))) {
                color = CLR_GOLD;
                _tcscpy(status, _T("[已解锁]"));
            } else {
                _tcscpy(status, _T("[未解锁]"));
            }
            settextstyle(FONT_SCALE(16), 0, FONT_UI);
            settextcolor(color);
            _stprintf(buf, _T("%s %s"), status, achName(i));
            outtextxy(80, y, buf);

            settextstyle(FONT_SCALE(13), 0, FONT_UI);
            if (g && (g->achUnlocked & (1u << i))) settextcolor(CLR_TEXT);
            else settextcolor(RGB(130, 150, 180));
            outtextxy(100, y + 20, achDesc(i));

            y += 36;
        }

        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回"), CLR_HINT);
    }

    FlushBatchDraw();
}

void gfxDrawGame(const GameState *g)
{
    drawGameContent(g);
    FlushBatchDraw();
}

void gfxDrawPause(void)
{
    drawOverlayPanel(WIN_W / 2 - 140, WIN_H / 2 - 90, 280, 180);
    settextstyle(FONT_SCALE(40), 0, FONT_TITLE);
    drawTextCenter(0, WIN_H / 2 - 60, WIN_W, 50, _T("暂停"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(22), 0, FONT_UI);
    drawTextCenter(0, WIN_H / 2 - 2, WIN_W, 30, _T("按 P 继续"), CLR_TEXT);
    drawTextCenter(0, WIN_H / 2 + 28, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);

    FlushBatchDraw();
}

void gfxDrawDeadTitle(const GameState *g, int remainingMs)
{
    int px, py, pw, ph;

    if (g) drawGameContent(g);

    pw = 360;
    ph = 140;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(40), 0, FONT_UI);
    if (g && g->gameMode == MODE_DUAL_TIMED && g->matchTimer <= 0.0f)
        drawTextCenter(px, py + 18, pw, 50, _T("TIME UP"), CLR_GOLD);
    else
        drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), CLR_RED_FOOD);

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    if (remainingMs > 0) {
        TCHAR buf[32];
        _stprintf(buf, _T("缓冲中... %.1fs"), remainingMs / 1000.0f);
        drawTextCenter(px, py + 80, pw, 30, buf, CLR_ACCENT_AMBER);
    } else {
        drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);
    }

    FlushBatchDraw();
}

void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];

    if (g) drawGameContent(g);

    pw = 360;
    ph = 290;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(34), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, _T("游戏结束"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
    _stprintf(buf, _T("SCORE: %d"), score);
    drawTextCenter(px, py + 66, pw, 30, buf, CLR_TEXT);
    _stprintf(buf, _T("HI: %d"), highScore);
    drawTextCenter(px, py + 94, pw, 30, buf, CLR_TEXT);

    if (isNewRecord) {
        settextstyle(FONT_SCALE(20), 0, FONT_UI);
        drawTextCenter(px, py + 124, pw, 28, _T("NEW RECORD!"), CLR_GOLD);
    }

    drawKeyHintsVertical(px, pw, py + 164, hoverIndex);
    FlushBatchDraw();
}

void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || (winner != WIN_P1 && winner != WIN_P2)) title = _T("平局");
    else if (winner == WIN_P1 && score1 > score2) title = _T("P1 获胜");
    else if (winner == WIN_P2 && score2 > score1) title = _T("P2 获胜");
    else if (score1 == score2) title = _T("平局");
    else if (score1 > score2) title = _T("P1 获胜");
    else title = _T("P2 获胜");

    if (g) drawGameContent(g);

    pw = 360;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(32), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, title, CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
    _stprintf(buf, _T("P1: %d    P2: %d"), score1, score2);
    drawTextCenter(px, py + 70, pw, 30, buf, CLR_TEXT);

    drawKeyHintsVertical(px, pw, py + 140, hoverIndex);
    FlushBatchDraw();
}

/* ===================================================
 *  镜像对决渲染
 * =================================================== */

/* 镜像模式立即结算按钮 — 纯函数式坐标计算（避免渲染与输入检测的时序错位） */
static void computeMirrorEndRect(int *x, int *y, int *w, int *h)
{
    int cx = MIRROR_GAP_CX;
    int cy = WIN_H / 2 + 100;
    *w = 128;
    *h = 42;
    *x = cx - *w / 2;
    *y = cy;
}

static void drawMirrorEndButton(void)
{
    int bx, by, bw, bh;
    computeMirrorEndRect(&bx, &by, &bw, &bh);

    setfillcolor(CLR_PANEL);
    solidrectangle(bx, by, bx + bw, by + bh);
    setlinecolor(CLR_BORDER_HI);
    rectangle(bx + 2, by + 2, bx + bw - 2, by + bh - 2);
    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_PANEL_GLOW);
    {
        RECT r = { bx, by, bx + bw, by + bh };
        drawtext(_T("立即结算"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

int gfxHitMirrorEndButton(int mx, int my)
{
    int bx, by, bw, bh;
    computeMirrorEndRect(&bx, &by, &bw, &bh);
    if (mx >= bx && mx <= bx + bw &&
        my >= by && my <= by + bh)
        return 1;
    return 0;
}

void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead)
{
    TCHAR buf[128];

    cleardevice();
    drawCheckerboard();

    if (g1) {
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
    }

    if (g2) {
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
    }

    /* 计分板最后绘制，避免被地图覆盖 */
    drawInfoPanel(20, 12, 260, 92, _T("P1 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P1: %d"), g1 ? g1->score : 0);
    outtextxy(36, 42, buf);
    if (g1) drawEffectIndicators(g1, 0, 30, 62);

    drawInfoPanel(MIRROR_MAP2_X, 12, 260, 92, _T("P2 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P2: %d"), g2 ? g2->score : 0);
    outtextxy(MIRROR_MAP2_X + 16, 42, buf);
    if (g2) drawEffectIndicators(g2, 1, MIRROR_MAP2_X + 20, 62);

    if (p1Dead || p2Dead) {
        settextstyle(FONT_SCALE(24), 0, FONT_UI);
        if (p1Dead) {
            settextcolor(CLR_RED_FOOD);
            outtextxy(MIRROR_MAP1_X + 138, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        if (p2Dead) {
            settextcolor(CLR_RED_FOOD);
            outtextxy(MIRROR_MAP2_X + 138, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        if (p1Dead != p2Dead) {
            drawMirrorEndButton();
        }
    }

    settextstyle(FONT_SCALE(15), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_HINT);
    outtextxy(50, WIN_H - 28, _T("P1:WASD+J"));
    outtextxy(MIRROR_MAP2_X, WIN_H - 28, _T("P2:方向键+0   P暂停  Q退出"));

    if (g1) { drawFloatTexts(g1, MIRROR_MAP1_X, MIRROR_MAP_Y); }
    if (g2) { drawFloatTexts(g2, MIRROR_MAP2_X, MIRROR_MAP_Y); }
    if (g1) drawAchNotification(g1);
    if (g2) drawAchNotification(g2);

    FlushBatchDraw();
}

void gfxDrawMirrorDeadTitle(const GameState *g1, const GameState *g2,
                            int p1Dead, int p2Dead, int remainingMs)
{
    int px, py, pw, ph;

    if (g1 && g2) {
        cleardevice();
        drawCheckerboard();
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
    }

    pw = 360;
    ph = 140;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(40), 0, FONT_UI);
    drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), CLR_RED_FOOD);

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    if (remainingMs > 0) {
        TCHAR buf[32];
        _stprintf(buf, _T("缓冲中... %.1fs"), remainingMs / 1000.0f);
        drawTextCenter(px, py + 80, pw, 30, buf, CLR_ACCENT_AMBER);
    } else {
        drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);
    }

    FlushBatchDraw();
}

void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || score1 == score2)
        title = _T("平局");
    else if (winner == WIN_P1 || score1 > score2)
        title = _T("P1 获胜");
    else
        title = _T("P2 获胜");

    pw = 360;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(32), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, title, CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
    _stprintf(buf, _T("P1: %d    P2: %d"), score1, score2);
    drawTextCenter(px, py + 70, pw, 30, buf, CLR_TEXT);

    drawKeyHintsVertical(px, pw, py + 140, hoverIndex);
    FlushBatchDraw();
}


/* ============================================================
 *  从 main.cpp 合并
 * ============================================================ */
/* ============================================================
 *  main.cpp - 主程序入口
 *  菜单逻辑、主循环
 * ============================================================ */


static void closeHostConsoleIfAny(void)
{
    HWND consoleWnd = GetConsoleWindow();
    if (!consoleWnd) {
        return;
    }

    FreeConsole();
}

/* ===================================================
 *  文件选择对话框
 * =================================================== */
static int pickLocalFile(char *outPath, size_t outSize, const char *title, const char *filter)
{
    OPENFILENAMEA ofn;
    char pathBuffer[MAX_PATH];

    if (!outPath || outSize == 0) return 0;

    memset(pathBuffer, 0, sizeof(pathBuffer));
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = pathBuffer;
    ofn.nMaxFile = (DWORD) sizeof(pathBuffer);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileNameA(&ofn)) {
        return 0;
    }

    snprintf(outPath, outSize, "%s", pathBuffer);
    return 1;
}

static void pickLocalMusicFile(char *outPath, size_t outSize)
{
    static const char filter[] =
        "音频文件 (*.wav;*.mp3;*.ogg;*.flac;*.aac;*.m4a;*.mid;*.wma)\0"
        "*.wav;*.mp3;*.ogg;*.flac;*.aac;*.m4a;*.mid;*.wma\0"
        "所有文件 (*.*)\0"
        "*.*\0";

    pickLocalFile(outPath, outSize, "选择背景音乐", filter);
}

static void pickLocalImageFile(char *outPath, size_t outSize)
{
    static const char filter[] =
        "图片文件 (*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp)\0"
        "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp\0"
        "所有文件 (*.*)\0"
        "*.*\0";

    pickLocalFile(outPath, outSize, "选择背景图片", filter);
}

/* ===================================================
 *  菜单辅助函数
 * =================================================== */

/* 将菜单选择转换为难度值 */
static int menuChoiceToDifficulty(int key)
{
    if (key == '1') return 0;
    if (key == '2') return 1;
    if (key == '3') return 2;
    return -1;
}

/* 循环切换蛇颜色 */
static void cycleSnakeColor(GameConfig *cfg)
{
    cfg->snakeColor = normalizeSnakeColor(cfg->snakeColor);
    cfg->snakeColor = (cfg->snakeColor + 1) % 3;
}

/* 循环切换地图大小 */
static void cycleMapSize(GameConfig *cfg)
{
    cfg->mapSize = normalizeMapSize(cfg->mapSize);
    if (cfg->mapSize == MAP_SIZE_SMALL) cfg->mapSize = MAP_SIZE_MEDIUM;
    else if (cfg->mapSize == MAP_SIZE_MEDIUM) cfg->mapSize = MAP_SIZE_LARGE;
    else cfg->mapSize = MAP_SIZE_SMALL;
}

static void toggleItemMode(GameConfig *cfg)
{
    cfg->itemMode = normalizeItemMode((cfg->itemMode == GAMEPLAY_CLASSIC) ? GAMEPLAY_ITEM : GAMEPLAY_CLASSIC);
}

static void toggleAiEnabled(GameConfig *cfg)
{
    cfg->aiEnabled = normalizeAiEnabled(!cfg->aiEnabled);
}

static void loadMusicFromDisk(GameConfig *cfg)
{
    char filePath[MAX_PATH];

    if (!cfg) return;

    filePath[0] = '\0';
    pickLocalMusicFile(filePath, sizeof(filePath));
    if (filePath[0] != '\0') {
        snprintf(cfg->menuMusicPath, sizeof(cfg->menuMusicPath), "%s", filePath);
        soundSetBackgroundMusic(filePath);
    }
}

static void loadImageFromDisk(GameConfig *cfg)
{
    char filePath[MAX_PATH];

    if (!cfg) return;

    filePath[0] = '\0';
    pickLocalImageFile(filePath, sizeof(filePath));
    if (filePath[0] != '\0') {
        snprintf(cfg->menuBgImagePath, sizeof(cfg->menuBgImagePath), "%s", filePath);
        gfxSetMenuBackgroundImage(filePath);
    }
}

static void clearCustomMenuBgImage(GameConfig *cfg)
{
    if (!cfg) return;

    gfxClearMenuBackgroundImage();
    cfg->menuBgImagePath[0] = '\0';
}

static void clearCustomMenuMusic(GameConfig *cfg)
{
    if (!cfg) return;

    soundStopBackgroundMusic();
    cfg->menuMusicPath[0] = '\0';
}

static int isSoloMode(int gameMode)
{
    return gameMode == MODE_SINGLE || gameMode == MODE_SURVIVAL;
}

static int pollMirrorEndButton(void)
{
    ExMessage em;
    int hit = 0;
    while (peekmessage(&em, EX_MOUSE, true)) {
        if (em.message == WM_LBUTTONDOWN &&
            gfxHitMirrorEndButton(em.x, em.y)) {
            hit = 1;
        }
    }
    return hit;
}

/* 暂停恢复时重置各模式移动时钟，避免长时间停顿导致一次性多步 */
static void resetMoveClocks(GameState *g, GameState *g2, DWORD now)
{
    if (!g) return;

    if (isSoloMode(g->gameMode)) {
        g->lastTickP1 = now;
    } else if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        g->lastTickP1 = now;
        g->lastTickP2 = now;
    } else if (g->gameMode == MODE_DUAL_MIRROR) {
        g->lastTickP1 = now;
        if (g2) g2->lastTickP1 = now;
    }
}

/* ===================================================
 *  主游戏循环
 * =================================================== */
static int runGame(void)
{
    GameState g;
    GameState g2;               /* 镜像对决 P2 */
    GameConfig cfg;
    int menuPage = MENU_MAIN;
    int hoverIndex = 0;
    int running = 1;
    int dualMode = MODE_DUAL;   /* 双人模式：经典/限时 */
    int singleMode = MODE_SINGLE; /* 单人模式：经典/生存 */
    int mirrorP1Dead = 0, mirrorP2Dead = 0;
    int mirrorEndRequested = 0;
    DWORD lastTick;
    DWORD lastDualTick;

    memset(&g, 0, sizeof(g));
    memset(&g2, 0, sizeof(g2));
    srand((unsigned)time(NULL));

    /* 加载配置 */
    int achBlueTotal = 0, achAIKills = 0, achShieldBlocks = 0;

    loadRecordConfig(&g.highScore, &cfg);
    g.achUnlocked = achLoad();
    achLoadState(&achBlueTotal, &achAIKills, &achShieldBlocks);
    g.achBlueTotal = achBlueTotal;
    g.achAIKills = achAIKills;
    g.achShieldBlocks = achShieldBlocks;
    cfg.snakeColor = normalizeSnakeColor(cfg.snakeColor);
    cfg.mapSize = normalizeMapSize(cfg.mapSize);
    cfg.itemMode = normalizeItemMode(cfg.itemMode);
    cfg.aiEnabled = normalizeAiEnabled(cfg.aiEnabled);
    g.config = cfg;
    g.mapSize = cfg.mapSize;
    g.gameState = STATE_MENU;

    /* 初始化图形系统 */
    gfxInit();
    soundInit();

    if (cfg.menuBgImagePath[0] != '\0') {
        gfxSetMenuBackgroundImage(cfg.menuBgImagePath);
    }
    if (cfg.menuMusicPath[0] != '\0') {
        soundSetBackgroundMusic(cfg.menuMusicPath);
    }

    lastTick = GetTickCount();
    lastDualTick = lastTick;

    while (running) {
        if (gfxWindowCloseRequested()) {
            running = 0;
            break;
        }

        /* ==================== 菜单状态 ==================== */
        if (g.gameState == STATE_MENU) {
            int key;
            gfxDrawMenu(&g, menuPage, hoverIndex);
            key = gfxGetMenuInput(&hoverIndex);

            if (key == 'q') key = 'Q';
            if (key == 'm') key = 'M';

            if (gfxWindowCloseRequested()) {
                running = 0;
            }
            else if (key == GFX_MENU_ACTION_MUSIC) {
                loadMusicFromDisk(&cfg);
            }
            else if (key == GFX_MENU_ACTION_IMAGE) {
                loadImageFromDisk(&cfg);
            }
            else if (key == GFX_MENU_ACTION_CLEAR_IMAGE) {
                clearCustomMenuBgImage(&cfg);
            }
            else if (key == GFX_MENU_ACTION_CLEAR_MUSIC) {
                clearCustomMenuMusic(&cfg);
            }
            else if (menuPage == MENU_MAIN) {
                if (key == '1') { singleMode = MODE_SINGLE; menuPage = MENU_SINGLE_DIFF; hoverIndex = 0; }
                else if (key == '2') { menuPage = MENU_DUAL_MODE; hoverIndex = 0; }
                else if (key == '3') { menuPage = MENU_HOWTOPLAY; hoverIndex = 0; }
                else if (key == '4') { menuPage = MENU_SETTINGS; hoverIndex = 0; }
                else if (key == '5') { menuPage = MENU_ACHIEVEMENTS; hoverIndex = 0; }
                else if (key == 'Q') running = 0;
            }
            else if (menuPage == MENU_SINGLE_DIFF) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0 || key == '4') {
                    singleMode = (key == '4') ? MODE_SURVIVAL : MODE_SINGLE;
                    if (singleMode == MODE_SURVIVAL)
                        diff = 1;
                    soundPlayMenuConfirm();
                    gameInit(&g, diff, &cfg);
                    g.gameMode = singleMode;
                    lastTick = GetTickCount();
                    lastDualTick = lastTick;
                    g.lastTickP1 = lastTick;
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_MODE) {
                if (key == '1') {
                    dualMode = MODE_DUAL;
                    menuPage = MENU_DUAL_DIFF; hoverIndex = 0;
                } else if (key == '2') {
                    dualMode = MODE_DUAL_TIMED;
                    menuPage = MENU_DUAL_DIFF; hoverIndex = 0;
                } else if (key == '3') {
                    menuPage = MENU_DUAL_MIRROR; hoverIndex = 0;
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN; hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_MIRROR) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0) {
                    unsigned seed = (unsigned)time(NULL);
                    GameConfig duelCfg = cfg;
                    duelCfg.aiEnabled = 0;
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    soundPlayMenuConfirm();
                    srand(seed); gameInit(&g, diff, &duelCfg);
                    srand(seed); gameInit(&g2, diff, &duelCfg);
                    g.gameMode = MODE_DUAL_MIRROR;
                    g2.gameMode = MODE_DUAL_MIRROR;
                    g.lastTickP1 = GetTickCount();
                    g2.lastTickP1 = GetTickCount();
                    lastTick = g.lastTickP1;
                    lastDualTick = lastTick;
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_DUAL_MODE; hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_DIFF) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0) {
                    soundPlayMenuConfirm();
                    gameInitDual(&g, diff, dualMode, &cfg);
                    lastTick = GetTickCount();
                    lastDualTick = lastTick;
                    g.lastTickP1 = lastTick;
                    g.lastTickP2 = lastTick;
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_DUAL_MODE;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_SETTINGS) {
                if (key == '1') {
                    cycleSnakeColor(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '2') {
                    cycleMapSize(&cfg);
                    g.config = cfg;
                    g.mapSize = cfg.mapSize;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '3') {
                    toggleItemMode(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '4') {
                    toggleAiEnabled(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            } else if (menuPage == MENU_ACHIEVEMENTS) {
                if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_HOWTOPLAY) {
                if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
        }
        /* ==================== 游戏进行中 ==================== */
        else if (g.gameState == STATE_PLAYING) {
            int key;
            DWORD now = GetTickCount();

            if (g.gameMode == MODE_DUAL_MIRROR &&
                mirrorP1Dead != mirrorP2Dead &&
                pollMirrorEndButton()) {
                mirrorEndRequested = 1;
            }

            key = gfxGetKey();

            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';

            if (gfxWindowCloseRequested()) {
                running = 0;
            }

            if (key == 'P' && running) {
                soundPlayPause();
                DWORD pauseNow = GetTickCount();
                g.gameState = STATE_PAUSED;
                lastTick = pauseNow;
                resetMoveClocks(&g, &g2, pauseNow);
            } else if (key == 'Q' && running) {
                running = 0;
            } else if (running) {
                /* 单人：WASD 和方向键都控制 P1
                 * 双人：WASD 控制 P1，方向键控制 P2 */
                if (isSoloMode(g.gameMode)) {
                    if (key == 'w' || key == 'W' || key == KEY_UP) gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S' || key == KEY_DOWN) gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A' || key == KEY_LEFT) gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D' || key == KEY_RIGHT) gameSetDir(&g.snake, DIR_RIGHT);
                } else if (g.gameMode == MODE_DUAL_MIRROR) {
                    /* 镜像：WASD→P1(g), 方向键→P2(g2) */
                    if (key == 'w' || key == 'W') gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S') gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A') gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D') gameSetDir(&g.snake, DIR_RIGHT);
                    if (key == KEY_UP) gameSetDir(&g2.snake, DIR_UP);
                    if (key == KEY_DOWN) gameSetDir(&g2.snake, DIR_DOWN);
                    if (key == KEY_LEFT) gameSetDir(&g2.snake, DIR_LEFT);
                    if (key == KEY_RIGHT) gameSetDir(&g2.snake, DIR_RIGHT);
                } else {
                    if (key == 'w' || key == 'W') gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S') gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A') gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D') gameSetDir(&g.snake, DIR_RIGHT);
                    if (key == KEY_UP) gameSetDir(&g.snake2, DIR_UP);
                    if (key == KEY_DOWN) gameSetDir(&g.snake2, DIR_DOWN);
                    if (key == KEY_LEFT) gameSetDir(&g.snake2, DIR_LEFT);
                    if (key == KEY_RIGHT) gameSetDir(&g.snake2, DIR_RIGHT);
                }

                if (g.gameMode == MODE_DUAL_MIRROR) {
                    /* ---- 镜像对决：独立 tick，不使用全局 canMove/dt/lastTick ---- */
                    int p1Ready = !mirrorP1Dead &&
                        (now - g.lastTickP1 >= (DWORD)g.speed);
                    int p2Ready = !mirrorP2Dead &&
                        (now - g2.lastTickP1 >= (DWORD)g2.speed);

                    if (p1Ready || p2Ready || mirrorEndRequested) {
                        if (p1Ready) {
                            float dt1 = (float)(now - g.lastTickP1) / 1000.0f;
                            gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                            if (g.config.itemMode == GAMEPLAY_ITEM)
                                itemUpdate(&g, dt1);
                            comboUpdate(&g, dt1);
                            floatTextUpdate(&g, dt1);
                            g.elapsedTime += dt1;
                            g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                            if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                            movingObsUpdate(&g, dt1);
                            if (g.config.aiEnabled)
                                aiUpdate(&g, dt1);
                            gameUpdateRedTimer(&g, dt1);
                            if (gameMove(&g) == 0) { achCheckAll(&g, g.gameMode); mirrorP1Dead = 1; }
                            g.lastTickP1 = now;
                        }
                        if (p2Ready) {
                            float dt2 = (float)(now - g2.lastTickP1) / 1000.0f;
                            /* 镜像 P2 的提速键通过 p1Boost 参数传入：
                             * gameApplySpeed 中 p1Boost→g->speed，P2 实际走 g2.snake/g2.speed */
                            gameApplySpeed(&g2, gfxIsKeyDown(VK_BOOST_P2), 0);
                            if (g2.config.itemMode == GAMEPLAY_ITEM)
                                itemUpdate(&g2, dt2);
                            comboUpdate(&g2, dt2);
                            floatTextUpdate(&g2, dt2);
                            g2.elapsedTime += dt2;
                            g2.diffFactor = 1.0f + g2.elapsedTime / 120.0f;
                            if (g2.diffFactor > 3.0f) g2.diffFactor = 3.0f;
                            movingObsUpdate(&g2, dt2);
                            if (g2.config.aiEnabled)
                                aiUpdate(&g2, dt2);
                            gameUpdateRedTimer(&g2, dt2);
                            if (gameMove(&g2) == 0) { achCheckAll(&g2, g2.gameMode); mirrorP2Dead = 1; }
                            g2.lastTickP1 = now;
                        }
                        /* 检测"立即结算"按钮点击（鼠标消息不再被 gfxGetKey 消费，可正常检测） */
                        if (mirrorP1Dead != mirrorP2Dead && pollMirrorEndButton())
                            mirrorEndRequested = 1;
                        if ((mirrorP1Dead && mirrorP2Dead) || mirrorEndRequested) {
                            soundPlayGameOver();
                            if (g.score > g2.score) g.winner = WIN_P1;
                            else if (g2.score > g.score) g.winner = WIN_P2;
                            else g.winner = WIN_DRAW;
                            if (g.winner != WIN_DRAW)
                                achUnlock(&g, ACH_MIRRORWIN);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    }
                } else {
                    if (isSoloMode(g.gameMode)) {
                        float dt = (float)(now - lastTick) / 1000.0f;
                        int moveReady;

                        lastTick = now;

                        gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                        moveReady = (now - g.lastTickP1 >= (DWORD)g.speed);
                        if (g.config.itemMode == GAMEPLAY_ITEM)
                            itemUpdate(&g, dt);
                        comboUpdate(&g, dt);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        if (g.config.aiEnabled)
                            aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);

                        if (moveReady) {
                            if (gameMove(&g) == 0) {
                                soundPlayGameOver();
                                achCheckAll(&g, g.gameMode);
                                int isNewRecord = g.score > g.highScore;
                                if (isNewRecord) {
                                    g.highScore = g.score;
                                    saveRecordConfig(g.highScore, &cfg);
                                }
                                g.deadTick = now;
                                g.gameState = STATE_DEAD_TITLE;
                            }
                            g.lastTickP1 = now;
                        }
                    } else {
                        int ret = 1;
                        float dt = (float)(now - lastDualTick) / 1000.0f;
                        int p1Ready = (now - g.lastTickP1 >= (DWORD)g.speed);
                        int p2Ready = (now - g.lastTickP2 >= (DWORD)g.speed2);

                        gameApplySpeed(&g,
                                      gfxIsKeyDown(VK_BOOST_P1),
                                      gfxIsKeyDown(VK_BOOST_P2));
                        if (g.config.itemMode == GAMEPLAY_ITEM)
                            itemUpdate(&g, dt);
                        comboUpdateForPlayer(&g, dt, 0);
                        comboUpdateForPlayer(&g, dt, 1);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        if (g.config.aiEnabled)
                            aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);

                        if (p1Ready || p2Ready) {
                            ret = gameMoveDual(&g, p1Ready, p2Ready);
                            if (p1Ready) g.lastTickP1 = now;
                            if (p2Ready) g.lastTickP2 = now;
                        }

                        if (ret == 0) {
                            soundPlayGameOver();
                            achCheckAll(&g, g.gameMode);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        } else if (g.gameMode == MODE_DUAL_TIMED &&
                                   !gameUpdateTimer(&g, dt)) {
                            /* 时间到，比分高者胜 */
                            soundPlayGameOver();
                            if (g.score > g.score2) g.winner = WIN_P1;
                            else if (g.score2 > g.score) g.winner = WIN_P2;
                            else g.winner = WIN_DRAW;
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }

                        lastDualTick = now;
                    }
                }
            }

            if (running) {
                if (g.gameMode == MODE_DUAL_MIRROR)
                    gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
                else
                    gfxDrawGame(&g);
            }
        }
        /* ==================== 暂停 ==================== */
        else if (g.gameState == STATE_PAUSED) {
            int key = gfxGetKey();
            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else {
                if (g.gameMode == MODE_DUAL_MIRROR) {
                    gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
                } else {
                    gfxDrawGame(&g);
                }
                gfxDrawPause();

                if (key == 'P') {
                    soundPlayResume();
                    DWORD resumeNow = GetTickCount();
                    g.gameState = STATE_PLAYING;
                    lastTick = resumeNow;
                    lastDualTick = resumeNow;
                    resetMoveClocks(&g, &g2, resumeNow);
                } else if (key == 'Q') {
                    running = 0;
                }
            }
        }
        /* ==================== 死亡标题 ==================== */
        else if (g.gameState == STATE_DEAD_TITLE) {
            int key = gfxGetKey();
            DWORD now = GetTickCount();
            int elapsed = (int)(now - g.deadTick);
            int remaining = 1000 - elapsed;
            if (remaining < 0) remaining = 0;

            if (key == 'q') key = 'Q';
            if (g.gameMode == MODE_DUAL_MIRROR) {
                gfxDrawMirrorDeadTitle(&g, &g2, mirrorP1Dead, mirrorP2Dead, remaining);
            } else {
                gfxDrawDeadTitle(&g, remaining);
            }

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else if (key == 'Q') {
                running = 0;
            } else if (remaining > 0) {
                /* 缓冲期内：忽略任意键转移（仅允许 Q 退出） */
                ;
            } else if (key != 0) {
                g.gameState = STATE_DEAD;
                hoverIndex = 0;
            }
        }
        /* ==================== 结算画面 ==================== */
        else if (g.gameState == STATE_DEAD) {
            int key = gfxGetDeadInput(&hoverIndex);
            if (key == 'q') key = 'Q';
            if (key == 'r') key = 'R';
            if (key == 'm') key = 'M';

            if (g.gameMode == MODE_DUAL_MIRROR) {
                int w;
                if (g.score > g2.score) w = WIN_P1;
                else if (g2.score > g.score) w = WIN_P2;
                else w = WIN_DRAW;
                gfxDrawMirrorOver(g.score, g2.score, w, hoverIndex);
            } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                gfxDrawDualOver(&g, g.winner, g.score, g.score2, hoverIndex);
            else
                gfxDrawGameOver(&g, g.score, g.highScore, g.score >= g.highScore && g.score > 0, hoverIndex);

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else if (key == 'R') {
                soundPlayMenuConfirm();
                if (g.gameMode == MODE_DUAL_MIRROR) {
                    unsigned seed = (unsigned)time(NULL);
                    GameConfig duelCfg = cfg;
                    duelCfg.aiEnabled = 0;
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    srand(seed); gameInit(&g, g.difficulty, &duelCfg);
                    srand(seed); gameInit(&g2, g2.difficulty, &duelCfg);
                    g.gameMode = MODE_DUAL_MIRROR;
                    g2.gameMode = MODE_DUAL_MIRROR;
                    g.lastTickP1 = g2.lastTickP1 = GetTickCount();
                    lastTick = g.lastTickP1;
                    lastDualTick = lastTick;
                } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                    gameInitDual(&g, g.difficulty, g.gameMode, &cfg);
                else {
                    int restartMode = g.gameMode;
                    gameInit(&g, g.difficulty, &cfg);
                    if (restartMode == MODE_SURVIVAL)
                        g.gameMode = MODE_SURVIVAL;
                }
                lastTick = GetTickCount();
                lastDualTick = lastTick;
                g.lastTickP1 = lastTick;
                if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED) {
                    g.lastTickP2 = lastTick;
                }
            } else if (key == 'M') {
                g.gameState = STATE_MENU;
                menuPage = MENU_MAIN;
                hoverIndex = 0;
            } else if (key == 'Q') {
                running = 0;
            }
        }

        Sleep(16);
    }

    saveRecordConfig(g.highScore, &cfg);
    gfxClose();
    closeHostConsoleIfAny();
    return 0;
}

/* ===================================================
 *  程序入口
 * =================================================== */
int main(void)
{
    return runGame();
}
