/* ============================================================
 *  snake.cpp - 游戏模型层实现
 *
 *  把原先 snake.h 中的全部实现移动到本文件，形成
 *  逻辑模块与接口声明分离的结构。
 * ============================================================ */
#include "snake.h"
#include "food.h"
#include "record.h"
#include "item.h"
#include "ai.h"
#include "combo.h"
#include "floattext.h"
#include "movingobs.h"
#include "achievement.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/* 初始化网格，设置边框墙 */
static void setupGrid(GameState *g)
{
    int x, y;
    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            if (x < g->mapSize && y < g->mapSize &&
                (x == 0 || y == 0 || x == g->mapSize - 1 || y == g->mapSize - 1))
                g->grid[y][x] = CELL_WALL;
            else
                g->grid[y][x] = CELL_EMPTY;
        }
    }
}

/* 随机放置障碍物 */
static void placeObstacles(GameState *g)
{
    int i;
    for (i = 0; i < g->obsCount && i < MAX_OBS; i++) {
        int x, y;
        do {
            x = 1 + rand() % (g->mapSize - 2);
            y = 1 + rand() % (g->mapSize - 2);
        } while (g->grid[y][x] != CELL_EMPTY);
        g->obs[i].x = x;
        g->obs[i].y = y;
        g->grid[y][x] = CELL_OBS;
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
    int cx;

    savedCfg.snakeColor = cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN;
    savedCfg.mapSize = cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE;
    savedCfg.itemMode = cfg ? normalizeItemMode(cfg->itemMode) : GAMEPLAY_ITEM;
    savedCfg.aiEnabled = cfg ? normalizeAiEnabled(cfg->aiEnabled) : 1;

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

    cx = g->mapSize / 2;
    g->snake.len = 3;
    g->snake.dir = DIR_UP;
    g->snake.lastDir = DIR_UP;
    g->snake.body[0].x = cx; g->snake.body[0].y = cx;
    g->snake.body[1].x = cx; g->snake.body[1].y = cx + 1;
    g->snake.body[2].x = cx; g->snake.body[2].y = cx + 2;
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

    /* 限时赛设置初始时长 */
    if (mode == MODE_DUAL_TIMED) {
        if (difficulty == 0) g->matchTimer = (float)TIMED_EASY;
        else if (difficulty == 1) g->matchTimer = (float)TIMED_NORMAL;
        else g->matchTimer = (float)TIMED_HARD;
    }

    applyDualDifficulty(g, difficulty);
    setupGrid(g);

    y = g->mapSize / 2;
    x1 = g->mapSize / 4;
    x2 = g->mapSize * 3 / 4;

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

    nextPosition(&g->snake, &nx, &ny);
    if (nx < 0 || ny < 0 || nx >= g->mapSize || ny >= g->mapSize) {
        if (itemConsumeShield(g, 0)) return 1;
        return 0;
    }

    cell = g->grid[ny][nx];

    /* 道具收集 */
    if (g->config.itemMode == GAMEPLAY_ITEM && cell == CELL_ITEM) {
        itemCollect(g, 0, g->itemOnField);
        cell = CELL_EMPTY;
    }

    /* 碰撞判定（穿墙效果允许穿过自己的身体） */
    if (cell == CELL_WALL || cell == CELL_OBS || cell == CELL_AI || cell == CELL_SNAKE2) {
        if (itemConsumeShield(g, 0)) return 1;
        return 0;
    }
    if (cell == CELL_SNAKE && !hasGhost) {
        if (itemConsumeShield(g, 0)) return 1;
        return 0;
    }

    eatBlue = (cell == CELL_BLUE);
    eatRed = (cell == CELL_RED && g->hasRed);
    grow = eatBlue || eatRed;

    if (!grow) {
        Position tail = g->snake.body[g->snake.len - 1];
        if (g->grid[tail.y][tail.x] == CELL_SNAKE)
            g->grid[tail.y][tail.x] = CELL_EMPTY;
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
    if (itemHasEffect(g, 0, EFF_MAGNET)) {
        int dx, dy;
        for (dy = -MAGNET_RANGE; dy <= MAGNET_RANGE; dy++) {
            for (dx = -MAGNET_RANGE; dx <= MAGNET_RANGE; dx++) {
                int mx = nx + dx, my = ny + dy;
                int dist = abs(dx) + abs(dy);
                if (dist > 0 && dist <= MAGNET_RANGE &&
                    mx > 0 && my > 0 && mx < g->mapSize - 1 && my < g->mapSize - 1 &&
                    g->grid[my][mx] == CELL_BLUE) {
                    int base = (BLUE_BASE * g->mult / 10) * mult;
                    int act = comboOnEat(g, base);
                    g->score += act;
                    g->blueCount++;
                    achOnEatBlue(g);
                    g->grid[my][mx] = CELL_EMPTY;
                    gamePlaceBlueFood(g);
                    if (g->blueCount % RED_TRIGGER == 0 && !g->hasRed)
                        gamePlaceRedFood(g);
                    achCheckAll(g, g->gameMode);
                    return 2;
                }
            }
        }
    }

    return 1;
}

/* 双人移动：0=有人死(看winner) 1=正常
 * moveP1/moveP2 指示本次哪些蛇需要移动，支持各自独立速度 */
int gameMoveDual(GameState *g, int moveP1, int moveP2)
{
    int nx1, ny1, nx2, ny2, c1, c2;
    int dead1 = 0, dead2 = 0;
    int eat1 = 0, eat2 = 0, eatRed1 = 0, eatRed2 = 0;
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

    /* 只计算需要移动的蛇的下一步位置 */
    if (moveP1) {
        nextPosition(&g->snake, &nx1, &ny1);
        peek1 = (nx1 < 0 || ny1 < 0 || nx1 >= g->mapSize || ny1 >= g->mapSize)
                ? CELL_WALL : g->grid[ny1][nx1];
        /* 道具收集 */
        if (g->config.itemMode == GAMEPLAY_ITEM && peek1 == CELL_ITEM) {
            itemCollect(g, 0, g->itemOnField);
            g->grid[ny1][nx1] = CELL_EMPTY;
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
        /* 道具收集 */
        if (g->config.itemMode == GAMEPLAY_ITEM && peek2 == CELL_ITEM) {
            itemCollect(g, 1, g->itemOnField);
            g->grid[ny2][nx2] = CELL_EMPTY;
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

    /* 抹除即将移开的尾巴（仅移动的蛇，且不吃食物时） */
    if (moveP1 && !(eat1 || eatRed1) && g->grid[tail1.y][tail1.x] == CELL_SNAKE)
        g->grid[tail1.y][tail1.x] = CELL_EMPTY;
    if (moveP2 && !(eat2 || eatRed2) && g->grid[tail2.y][tail2.x] == CELL_SNAKE2)
        g->grid[tail2.y][tail2.x] = CELL_EMPTY;

    /* 去尾后重新检查目标格 */
    if (moveP1) {
        c1 = (nx1 < 0 || ny1 < 0 || nx1 >= g->mapSize || ny1 >= g->mapSize)
           ? CELL_WALL : g->grid[ny1][nx1];
        if (c1 == CELL_WALL || c1 == CELL_OBS || c1 == CELL_AI || c1 == CELL_SNAKE2) {
            if (itemConsumeShield(g, 0)) moveP1 = 0; else dead1 = 1;
        } else if (c1 == CELL_SNAKE && !ghost1) {
            if (itemConsumeShield(g, 0)) moveP1 = 0; else dead1 = 1;
        }
    }
    if (moveP2) {
        c2 = (nx2 < 0 || ny2 < 0 || nx2 >= g->mapSize || ny2 >= g->mapSize)
           ? CELL_WALL : g->grid[ny2][nx2];
        if (c2 == CELL_WALL || c2 == CELL_OBS || c2 == CELL_AI || c2 == CELL_SNAKE) {
            if (itemConsumeShield(g, 1)) moveP2 = 0; else dead2 = 1;
        } else if (c2 == CELL_SNAKE2 && !ghost2) {
            if (itemConsumeShield(g, 1)) moveP2 = 0; else dead2 = 1;
        }
    }

    if (dead1 || dead2) {
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
        int act = comboOnEat(g, BLUE_BASE * mult1);
        g->score += act; achOnEatBlue(g);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx1, (float)ny1, b, RGB(230,230,230), 1.2f); }
    }
    if (eat2) {
        /* Note: combo system is shared (P1's combo tracking). For dual,
           we give P2 the same combo benefit using P1's combo state */
        int act = comboOnEat(g, BLUE_BASE * mult2);
        g->score2 += act; achOnEatBlue(g);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx2, (float)ny2, b, RGB(230,230,230), 1.2f); }
    }
    if (eatRed1) {
        int base = calcRedScore(g) * mult1;
        int act = comboOnEat(g, base);
        g->score += act; g->hasRed = 0;
        if (base >= RED_BASE * mult1) achUnlock(g, ACH_RED50);
        { TCHAR b[16]; _stprintf(b, _T("+%d"), act);
          floatTextAdd(g, (float)nx1, (float)ny1, b, RGB(230,70,70), 1.2f); }
    }
    if (eatRed2) {
        int base = calcRedScore(g) * mult2;
        int act = comboOnEat(g, base);
        g->score2 += act; g->hasRed = 0;
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
