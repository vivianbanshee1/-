/* ============================================================
 *  snake_render.h - 渲染与主流程模块接口
 *
 *  负责：
 *  - 窗口/布局常量
 *  - EasyX 绘制函数声明
 *  - 菜单/游戏/死亡界面渲染
 *  - 镜像对决双地图渲染
 *  - 按键常量与菜单动作常量
 *
 *  对应实现文件：snake_render.cpp
 * ============================================================ */
#ifndef SNAKE_RENDER_H
#define SNAKE_RENDER_H

#include "snake_logic.h"

/* ===================================================
 *  窗口与布局
 * =================================================== */
#define CELL_PX     24      /* 每格像素 */
#define WIN_W       1100    /* 窗口宽（镜像对决扩宽） */
#define WIN_H       600     /* 窗口高 */

/* ===== 主界面布局 ===== */
#define MAP_X       60      /* 地图左上角X（原单图） */
#define MAP_Y       80      /* 地图左上角Y（原单图） */

/* ===== 菜单按钮布局（与输入模块共用） ===== */
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

/* ===================================================
 *  按键常量
 * =================================================== */
#define KEY_UP        0x101
#define KEY_DOWN      0x102
#define KEY_LEFT      0x103
#define KEY_RIGHT     0x104

/* ===================================================
 *  菜单自定义动作
 * =================================================== */
#define GFX_MENU_ACTION_MUSIC        0x1001
#define GFX_MENU_ACTION_IMAGE        0x1002
#define GFX_MENU_ACTION_CLEAR_IMAGE  0x1003
#define GFX_MENU_ACTION_CLEAR_MUSIC  0x1004

/* ===================================================
 *  菜单按钮命中检测
 * =================================================== */
int gfxHitDeadButton(int mx, int my);
int gfxHitMirrorEndButton(int mx, int my);
int gfxHitMenuCornerAction(int mx, int my);
int gfxHitMenuButton(int mx, int my);

/* ===================================================
 *  窗口初始化与关闭
 * =================================================== */
void gfxInit(void);
void gfxClose(void);

/* ===================================================
 *  菜单与 HUD 渲染
 * =================================================== */
void gfxDrawMenu(const GameState *g, int menuPage, int hoverIndex);
void gfxDrawGame(const GameState *g);
void gfxDrawPause(void);

/* ===================================================
 *  死亡/结算界面
 * =================================================== */
void gfxDrawDeadTitle(const GameState *g, int remainingMs);
void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex);
void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex);

/* ===================================================
 *  镜像对决渲染
 * =================================================== */
void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead);
void gfxDrawMirrorDeadTitle(const GameState *g1, const GameState *g2,
                            int p1Dead, int p2Dead, int remainingMs);
void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex);

/* ===================================================
 *  菜单背景图片
 * =================================================== */
void gfxSetMenuBackgroundImage(const char *path);
void gfxClearMenuBackgroundImage(void);
int  gfxHasMenuBackgroundImage(void);

#endif /* SNAKE_RENDER_H */
