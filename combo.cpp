/* ============================================================
 *  combo.cpp - 连击系统实现
 * ============================================================ */
#include "combo.h"

void comboUpdate(GameState *g, float dt)
{
    if (g->comboCount > 0) {
        g->comboCD -= dt;
        if (g->comboCD <= 0.0f) {
            g->comboCD = 0.0f;
            g->comboCount = 0;
        }
    }
}

int comboOnEat(GameState *g, int baseScore)
{
    if (g->comboCD > 0.0f) {
        g->comboCount++;
    } else {
        g->comboCount = 1;
    }
    g->comboCD = COMBO_WINDOW;

    if (g->comboCount > g->maxCombo)
        g->maxCombo = g->comboCount;

    return (int)(baseScore * (1.0f + (g->comboCount - 1) * 0.5f));
}

int comboCount(const GameState *g) { return g->comboCount; }
int comboMax(const GameState *g)   { return g->maxCombo; }
