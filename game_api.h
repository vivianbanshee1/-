/* ============================================================
 *  game_api.h - 合并后的跨模块 API 汇总（用于单 cpp 结构）
 *
 *  该头文件承载“原先分散在多个模块头文件中的公开接口”：
 *  - 食物/道具/AI/移动障碍
 *  - 连击/飘字/成就
 *  - 记录/音效/输入/渲染
 *
 *  运行时所有实现集中在 snake.cpp 中。
 * ============================================================ */
#ifndef GAME_API_H
#define GAME_API_H

#include "snake.h"

#include <tchar.h>

/* ===================================================
 *  食物模块
 * =================================================== */
void gamePlaceBlueFood(GameState *g);
void gamePlaceRedFood(GameState *g);
void gameUpdateRedTimer(GameState *g, float dt);
int  calcRedScore(const GameState *g);

/* ===================================================
 *  道具模块
 * =================================================== */
void itemUpdate(GameState *g, float dt);
void itemCollect(GameState *g, int player, int itemType);
int itemHasEffect(const GameState *g, int player, int eff);
float itemTimer(const GameState *g, int player, int eff);
int itemShieldCount(const GameState *g, int player);
int itemIsNegative(int itemType);
void itemRemove(GameState *g);
int itemConsumeShield(GameState *g, int player);

/* 道具范围（原 item.h） */
#define MAGNET_RANGE 3

/* ===================================================
 *  AI 模块
 * =================================================== */
void aiInit(GameState *g);
void aiUpdate(GameState *g, float dt);
void aiMarkAll(GameState *g);
void aiKill(GameState *g, int index);
int  aiScoreReward(int aiLen, int mult);

/* ===================================================
 *  移动障碍模块
 * =================================================== */
#define MOVING_OBS_SPAWN  20.0f

void movingObsInit(GameState *g);
void movingObsUpdate(GameState *g, float dt);

/* ===================================================
 *  连击模块
 * =================================================== */
#define COMBO_WINDOW 3.0f

void comboUpdate(GameState *g, float dt);
void comboUpdateForPlayer(GameState *g, float dt, int player);
int  comboOnEat(GameState *g, int baseScore);
int  comboOnEatForPlayer(GameState *g, int baseScore, int player);
int  comboCount(const GameState *g);
int  comboCountForPlayer(const GameState *g, int player);
int  comboMax(const GameState *g);
int  comboMaxForPlayer(const GameState *g, int player);

/* ===================================================
 *  飘字模块
 * =================================================== */
void floatTextAdd(GameState *g, float x, float y,
                  LPCTSTR text, COLORREF color, float life);
void floatTextUpdate(GameState *g, float dt);
void floatTextClearAll(GameState *g);

/* ===================================================
 *  成就模块
 * =================================================== */
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

unsigned achLoad(void);
void achLoadState(int *blueTotal, int *aiKills, int *shieldBlocks);
void achSaveState(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);
int achUnlock(GameState *g, int achId);
int achCheckAll(GameState *g, int gameMode);
LPCTSTR achName(int achId);
LPCTSTR achDesc(int achId);
void achOnEatBlue(GameState *g);
void achOnKillAI(GameState *g);
void achOnShieldBlock(GameState *g);

/* ===================================================
 *  存档模块
 * =================================================== */
void loadRecordConfig(int *highScore, GameConfig *cfg);
void saveRecordConfig(int highScore, const GameConfig *cfg);
int  loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks);
void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks);

/* ===================================================
 *  音效模块
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
 *  渲染模块
 * =================================================== */

/* 窗口与布局 */
#define CELL_PX     24      /* 每格像素 */
#define WIN_W       1100    /* 窗口宽（镜像对决扩宽） */
#define WIN_H       600     /* 窗口高 */

/* ===== 主界面布局 ===== */
#define MAP_X       60      /* 地图左上角X（原单图） */
#define MAP_Y       80      /* 地图左上角Y（原单图） */

/* ===== 菜单按钮布局（与 input.cpp 共用） ===== */
#define MENU_BTN_W        320
#define MENU_BTN_H         56
#define MENU_BTN_GAP        62
#define MENU_BTN_START_Y   206
#define MENU_BTN_X      (WIN_W / 2 - MENU_BTN_W / 2)

/* 镜像对决布局 */
#define MIRROR_MAP1_X   50   /* P1 地图 X */
#define MIRROR_MAP2_X   570  /* P2 地图 X */
#define MIRROR_MAP_Y    56   /* 双地图 Y */
#define MIRROR_GAP_CX   550  /* 中心 X */

/* 按键常量 */
#define KEY_UP        0x101
#define KEY_DOWN      0x102
#define KEY_LEFT      0x103
#define KEY_RIGHT     0x104

/* 菜单自定义动作 */
#define GFX_MENU_ACTION_MUSIC        0x1001
#define GFX_MENU_ACTION_IMAGE        0x1002
#define GFX_MENU_ACTION_CLEAR_IMAGE  0x1003
#define GFX_MENU_ACTION_CLEAR_MUSIC  0x1004

/* 菜单按钮命中 */
int gfxHitDeadButton(int mx, int my);
int gfxHitMirrorEndButton(int mx, int my);
int gfxHitMenuCornerAction(int mx, int my);

void gfxInit(void);
void gfxClose(void);

void gfxDrawMenu(const GameState *g, int menuPage, int hoverIndex);
void gfxDrawGame(const GameState *g);
void gfxDrawPause(void);
void gfxDrawDeadTitle(const GameState *g, int remainingMs);
void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex);
void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex);

void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead);
void gfxDrawMirrorDeadTitle(const GameState *g1, const GameState *g2,
                            int p1Dead, int p2Dead, int remainingMs);
void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex);

int gfxHitMenuButton(int mx, int my);

void gfxSetMenuBackgroundImage(const char *path);
void gfxClearMenuBackgroundImage(void);
int  gfxHasMenuBackgroundImage(void);

/* ===================================================
 *  输入模块
 * =================================================== */
#define VK_BOOST_P1  'J'
#define VK_BOOST_P2  '0'

int gfxIsKeyDown(int vkcode);
int gfxGetKey(void);
int gfxGetMenuInput(int *hoverIndex);
int gfxGetDeadInput(int *hoverIndex);
int gfxWindowCloseRequested(void);

#endif /* GAME_API_H */
