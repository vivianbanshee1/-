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
#include "ai.h"
#include "food.h"
#include "achievement.h"

#include <stdlib.h>

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

/* 格子是否可通行（对AI而言：空格、蓝食物、红食物、道具均可） */
static int isPassable(int cell)
{
    return cell == CELL_EMPTY || cell == CELL_BLUE ||
           cell == CELL_RED || cell == CELL_ITEM;
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
        int cell;

        /* 不能反向 */
        if (d == opp) continue;

        /* 边界检查 */
        if (nx <= 0 || ny <= 0 || nx >= g->mapSize - 1 || ny >= g->mapSize - 1)
            continue;

        cell = g->grid[ny][nx];

        /* 可通过格子 */
        if (!isPassable(cell)) continue;

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
        if (x <= 0 || y <= 0 || x >= g->mapSize - 1 || y >= g->mapSize - 1)
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
                int sx = 2 + rand() % (g->mapSize - 4);
                int sy = 2 + rand() % (g->mapSize - 4);
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
        if (nx <= 0 || ny <= 0 || nx >= g->mapSize - 1 || ny >= g->mapSize - 1) {
            aiKill(g, i); continue; /* 不递增 i */
        }

        cell = g->grid[ny][nx];

        /* 撞障碍/墙/AI身 → 死亡 */
        if (cell == CELL_WALL || cell == CELL_OBS || cell == CELL_AI) {
            aiKill(g, i); continue;
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
            Position tail = a->body[a->len - 1];
            if (g->grid[tail.y][tail.x] == CELL_AI)
                g->grid[tail.y][tail.x] = CELL_EMPTY;
            {
                int k;
                int newLen = a->len + 1;
                if (newLen > MAX_LEN) newLen = MAX_LEN;
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
