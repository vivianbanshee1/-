/* ============================================================
 *  record.h - 记录持久化接口
 *
 *  负责：
 *  - 读取 record.txt 中的最高分与基础配置
 *  - 保存最高分与基础配置到 record.txt
 * ============================================================ */
#ifndef RECORD_H
#define RECORD_H

#include "snake.h"

/* 从 record.txt 读取最高分和配置 */
void loadRecordConfig(int *highScore, GameConfig *cfg);

/* 保存最高分和配置到 record.txt */
void saveRecordConfig(int highScore, const GameConfig *cfg);

/* 从 record.txt 读取成就数据，返回1=找到成就字段，0=未找到 */
int loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks);

/* 保存成就数据到 record.txt，同时保留最高分和配置 */
void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);

#endif /* RECORD_H */
