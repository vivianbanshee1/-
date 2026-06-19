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

/* 每帧更新连击冷却（超时归零）
 * 默认更新 P1（单人模式 / 双人模式默认用于 P1）。 */
void comboUpdate(GameState *g, float dt);

/* 按玩家更新连击冷却（player: 0=P1, 1=P2） */
void comboUpdateForPlayer(GameState *g, float dt, int player);

/* 吃到食物时调用，返回实际应得分数（含连击倍率）
 * 默认按 P1（单人模式 / 双人模式默认用于 P1）。 */
int  comboOnEat(GameState *g, int baseScore);

/* 按玩家吃食物时返回实际得分（含连击倍率） */
int  comboOnEatForPlayer(GameState *g, int baseScore, int player);

/* 获取当前连击数（gfx用）
 * 默认返回 P1。 */
int  comboCount(const GameState *g);

/* 按玩家获取当前连击数 */
int  comboCountForPlayer(const GameState *g, int player);

/* 获取本局最高连击 */
int  comboMax(const GameState *g);

/* 按玩家获取本局最高连击 */
int  comboMaxForPlayer(const GameState *g, int player);

#endif
