/* ============================================================
 *  combo.h - 连击系统接口
 *
 *  短时间内连续吃食物触发连击，额外加分。
 *  3秒窗口内每吃一个 +1 combo，乘数 = 1 + combo×0.5
 * ============================================================ */
#ifndef COMBO_H
#define COMBO_H

#include "snake.h"

#define COMBO_WINDOW 3.0f

/* 每帧更新连击冷却（超时归零） */
void comboUpdate(GameState *g, float dt);

/* 吃到食物时调用，返回实际应得分数（含连击倍率） */
int  comboOnEat(GameState *g, int baseScore);

/* 获取当前连击数（gfx用） */
int  comboCount(const GameState *g);

/* 获取本局最高连击 */
int  comboMax(const GameState *g);

#endif
