/* ============================================================
 *  item.h - 道具系统接口
 *
 *  负责：
 *  - 道具定时生成、存活倒计时、自动消失
 *  - 道具收集时触发对应效果
 *  - 效果定时器更新、过期清除
 *  - 效果查询（供其他模块判断蛇当前状态）
 * ============================================================ */
#ifndef ITEM_H
#define ITEM_H

#include "snake.h"

/* 每帧更新道具系统（生成、计时、过期） */
void itemUpdate(GameState *g, float dt);

/* 玩家收集到道具时触发效果 */
void itemCollect(GameState *g, int player, int itemType);

/* 查询玩家某个效果是否正在生效 */
int itemHasEffect(const GameState *g, int player, int eff);

/* 查询玩家某个效果的剩余时间 */
float itemTimer(const GameState *g, int player, int eff);

/* 查询玩家护盾层数 */
int itemShieldCount(const GameState *g, int player);

/* 判断道具类型是否为负面道具 */
int itemIsNegative(int itemType);

/* 移除场上道具 */
void itemRemove(GameState *g);

/* 消耗一层护盾（返回0=无护盾可消耗 1=消耗成功） */
int itemConsumeShield(GameState *g, int player);

/* 磁铁吸附范围 */
#define MAGNET_RANGE 3

#endif /* ITEM_H */
