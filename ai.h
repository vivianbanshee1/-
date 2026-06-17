/* ============================================================
 *  ai.h - AI 敌人蛇接口
 *
 *  负责：
 *  - AI 蛇的生成、移动、寻路、死亡处理
 *  - 与游戏网格的交互
 * ============================================================ */
#ifndef AI_H
#define AI_H

#include "snake.h"

/* 初始化 AI 系统 */
void aiInit(GameState *g);

/* 每帧更新（生成计时 + 移动） */
void aiUpdate(GameState *g, float dt);

/* 将所有 AI 蛇身体标记到网格上 */
void aiMarkAll(GameState *g);

/* 杀死某条 AI 蛇并清理其在网格上的标记 */
void aiKill(GameState *g, int index);

/* 对玩家的计分奖励（杀死 AI 时玩家获得的分数） */
int  aiScoreReward(int aiLen, int mult);

#endif /* AI_H */
