/* ============================================================
 *  record.h - 记录持久化接口
 *
 *  负责：
 *  - 读取 record.txt 中的最高分和用户配置
 *  - 保存最高分和用户配置到 record.txt
 * ============================================================ */
#ifndef RECORD_H
#define RECORD_H

#include "snake.h"

/* 从 record.txt 读取最高分和配置 */
void loadRecordConfig(int *highScore, GameConfig *cfg);

/* 保存最高分和配置到 record.txt */
void saveRecordConfig(int highScore, const GameConfig *cfg);

#endif /* RECORD_H */
