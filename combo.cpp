/* ============================================================
 *  combo.cpp - 连击系统实现
 * ============================================================ */
#include "combo.h"

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
