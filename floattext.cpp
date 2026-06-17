/* ============================================================
 *  floattext.cpp - 飘字系统实现
 * ============================================================ */
#include "floattext.h"

#include <string.h>

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
