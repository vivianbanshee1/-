/* ============================================================
 *  item.cpp - 道具系统实现
 *
 *  管理道具生成、收集、效果定时与过期。
 * ============================================================ */
#include "item.h"
#include "achievement.h"
#include "sound.h"

#include <stdlib.h>
#include <string.h>

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
    int maxType = (g->gameMode == MODE_SINGLE) ? ITEM_MAX - 1 : ITEM_MAX;

    /* 随机选取类型（排除冻结，如果是单人） */
    type = 1 + rand() % maxType;
    if (type == ITEM_FREEZE && g->gameMode == MODE_SINGLE)
        type = ITEM_TURBO;  /* 单人模式不生成冻结，用极速替代 */

    /* 随机找空格 */
    tries = g->mapSize * g->mapSize * 2;
    while (tries-- > 0) {
        x = 1 + rand() % (g->mapSize - 2);
        y = 1 + rand() % (g->mapSize - 2);
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
        g->grid[g->itemPos.y][g->itemPos.x] = CELL_EMPTY;
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
