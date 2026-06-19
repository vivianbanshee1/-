/* ============================================================
 *  movingobs.cpp - 移动障碍实现
 * ============================================================ */
#include "movingobs.h"

#include <stdlib.h>

static const int dirDX[4] = { 0, 0, -1, 1 };
static const int dirDY[4] = { -1, 1, 0, 0 };

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
                int nx = g->obsMoving[i].x + dirDX[g->obsMovingDir[i]];
                int ny = g->obsMoving[i].y + dirDY[g->obsMovingDir[i]];
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
