/* ============================================================
 *  food.cpp - 食物子系统实现
 *
 *  从 snake.cpp 提取的食物放置、计时与计分逻辑。
 * ============================================================ */
#include "food.h"

#include <stdlib.h>

/* ===================================================
 *  内部辅助
 * =================================================== */

/* 在网格中找一个空格子（避开已有食物） */
static int randomEmptyCell(GameState *g, Position *p)
{
    int tries = g->mapSize * g->mapSize * 2;
    while (tries-- > 0) {
        int x = 1 + rand() % (g->mapSize - 2);
        int y = 1 + rand() % (g->mapSize - 2);
        if (g->grid[y][x] == CELL_EMPTY) {
            p->x = x;
            p->y = y;
            return 1;
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
        int x = 1 + rand() % (g->mapSize - 2);
        int y = 1 + rand() % (g->mapSize - 2);
        if (g->grid[y][x] == CELL_EMPTY &&
            !(g->hasRed && x == g->redFood.x && y == g->redFood.y)) {
            g->blueFood.x = x;
            g->blueFood.y = y;
            g->grid[y][x] = CELL_BLUE;
            return;
        }
    }

    if (randomEmptyCell(g, &p)) {
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
        int x = 1 + rand() % (g->mapSize - 2);
        int y = 1 + rand() % (g->mapSize - 2);
        if (g->grid[y][x] == CELL_EMPTY && !(x == g->blueFood.x && y == g->blueFood.y)) {
            g->redFood.x = x;
            g->redFood.y = y;
            g->grid[y][x] = CELL_RED;
            g->hasRed = 1;
            g->redTimer = (float)RED_TIMER_MAX;
            return;
        }
    }

    if (randomEmptyCell(g, &p)) {
        if (!(p.x == g->blueFood.x && p.y == g->blueFood.y)) {
            g->redFood = p;
            g->grid[p.y][p.x] = CELL_RED;
            g->hasRed = 1;
            g->redTimer = (float)RED_TIMER_MAX;
        }
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
