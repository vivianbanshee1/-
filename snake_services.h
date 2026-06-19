/* ============================================================
 *  snake_services.h - 系统服务模块接口
 *
 *  负责：
 *  - 持久化配置/成就读写
 *  - 音效播放与背景音乐
 *  - 键盘/鼠标输入处理
 *
 *  对应实现文件：snake_services.cpp
 * ============================================================ */
#ifndef SNAKE_SERVICES_H
#define SNAKE_SERVICES_H

#include "snake_render.h"

/* ===================================================
 *  ===== 第一部分：存档模块 =====
 * =================================================== */
void loadRecordConfig(int *highScore, GameConfig *cfg);
void saveRecordConfig(int highScore, const GameConfig *cfg);
int  loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks);
void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);

/* ===================================================
 *  ===== 第二部分：音效模块 =====
 * =================================================== */
void soundInit(void);
void soundSetEnabled(int enabled);
int  soundIsEnabled(void);

void soundPlayMenuConfirm(void);
void soundPlayEatBlue(void);
void soundPlayEatRed(void);
void soundPlayItemCollect(int itemType);
void soundPlayDeath(void);
void soundPlayPause(void);
void soundPlayResume(void);
void soundPlayShieldBlock(void);
void soundPlayGameOver(void);

int  soundSetBackgroundMusic(const char *filePath);
void soundPlayBackgroundMusic(void);
void soundStopBackgroundMusic(void);
int  soundHasBackgroundMusic(void);

/* ===================================================
 *  ===== 第三部分：输入模块 =====
 * =================================================== */
#define VK_BOOST_P1  'J'
#define VK_BOOST_P2  '0'

int gfxIsKeyDown(int vkcode);
int gfxGetKey(void);
int gfxGetMenuInput(int *hoverIndex);
int gfxGetDeadInput(int *hoverIndex);
int gfxWindowCloseRequested(void);

#endif /* SNAKE_SERVICES_H */
