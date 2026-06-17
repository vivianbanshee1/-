/* ============================================================
 *  movingobs.h - 移动障碍接口
 *
 *  障碍在场地上缓慢漂移，撞墙反弹。
 *  每20秒尝试生成一个（上限5个）。
 * ============================================================ */
#ifndef MOVINGOBS_H
#define MOVINGOBS_H

#include "snake.h"

#define MOVING_OBS_SPAWN  20.0f   /* 生成间隔（秒） */

/* 初始化 */
void movingObsInit(GameState *g);

/* 每帧更新（生成+移动） */
void movingObsUpdate(GameState *g, float dt);

#endif
