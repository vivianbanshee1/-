/* ============================================================
 *  food.h - 食物子系统接口
 *
 *  负责：
 *  - 蓝色食物（永久）放置
 *  - 红色食物（限时）放置、计时与消灭
 *  - 红色食物得分计算
 * ============================================================ */
#ifndef FOOD_H
#define FOOD_H

#include "snake.h"

/* 放置蓝色食物 — 避开红色食物位置 */
void gamePlaceBlueFood(GameState *g);

/* 放置红色食物 — 避开蓝色食物位置 */
void gamePlaceRedFood(GameState *g);

/* 红色食物计时器更新（每帧调用） */
void gameUpdateRedTimer(GameState *g, float dt);

/* 计算红色食物当前得分（随时间递减） */
int  calcRedScore(const GameState *g);

#endif /* FOOD_H */
