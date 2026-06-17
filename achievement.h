/* ============================================================
 *  achievement.h - 成就系统接口
 *
 *  10种成就，位掩码持久化。跨局累积。
 *  解锁时返回成就名称供 gfx 显示通知。
 * ============================================================ */
#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include "snake.h"

#define ACH_MAX 10

/* 成就 ID */
#define ACH_FIRST100    0   /* 首次得分>100 */
#define ACH_BLUE100     1   /* 累计吃100个蓝食物 */
#define ACH_BLUE20      2   /* 单局吃20个蓝食物 */
#define ACH_COMBO5      3   /* 达成5连击 */
#define ACH_KILLAI10    4   /* 累计杀死10条AI */
#define ACH_SHIELD10    5   /* 累计用护盾挡10次 */
#define ACH_SPEED200    6   /* 60秒内得200分 */
#define ACH_SURVIVE180  7   /* 生存模式活过180秒 */
#define ACH_MIRRORWIN   8   /* 镜像对决中获胜 */
#define ACH_RED50       9   /* 吃到满分红食物(50分) */

/* 从 record.txt 加载成就位掩码 */
unsigned achLoad(void);

/* 加载累计统计：蓝食物总数、AI击杀总数、护盾拦截总数 */
void achLoadState(int *blueTotal, int *aiKills, int *shieldBlocks);

/* 保存成就位掩码和累计统计 */
void achSaveState(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);

/* 尝试解锁成就，返回0=已解锁，1=刚解锁 */
int achUnlock(GameState *g, int achId);

/* 检测本局各种成就条件并尝试解锁，返回新解锁数 */
int achCheckAll(GameState *g, int gameMode);

/* 获取成就名称 */
LPCTSTR achName(int achId);

/* 获取成就描述 */
LPCTSTR achDesc(int achId);

/* 本局蓝食物计数+1 */
void achOnEatBlue(GameState *g);

/* 本局AI杀计数+1 */
void achOnKillAI(GameState *g);

/* 本局护盾挡+1 */
void achOnShieldBlock(GameState *g);

#endif
