/* ============================================================
 *  gfx.h - 视图层接口（模块化重构）
 *
 *  负责：
 *  - 窗口参数与 UI 常量声明
 *  - 渲染与输入函数接口声明
 *
 *  实现代码已迁移到 gfx.cpp
 * ============================================================ */
#ifndef GFX_H
#define GFX_H

#include "snake.h"
#include <tchar.h>

/* ===================================================
 *  窗口与布局
 * =================================================== */
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
#define MIRROR_GAP_CX   550  /* 中间间隙中心 X */

/* ===================================================
 *  键盘按键码（统一入口）
 * =================================================== */
#define KEY_UP        0x101
#define KEY_DOWN      0x102
#define KEY_LEFT      0x103
#define KEY_RIGHT     0x104

/* ===================================================
 *  菜单自定义动作返回值
 * =================================================== */
#define GFX_MENU_ACTION_MUSIC        0x1001
#define GFX_MENU_ACTION_IMAGE        0x1002
#define GFX_MENU_ACTION_CLEAR_IMAGE  0x1003
#define GFX_MENU_ACTION_CLEAR_MUSIC  0x1004

/* ===================================================
 *  按钮命中检测
 * =================================================== */
int gfxHitDeadButton(int mx, int my);
int gfxHitMirrorEndButton(int mx, int my);
int gfxHitMenuCornerAction(int mx, int my);

/* ===================================================
 *  图形系统初始化 / 销毁
 * =================================================== */
void gfxInit(void);
void gfxClose(void);

/* ===================================================
 *  各状态画面渲染
 * =================================================== */
void gfxDrawMenu(const GameState *g, int menuPage, int hoverIndex);
void gfxDrawGame(const GameState *g);
void gfxDrawPause(void);
void gfxDrawDeadTitle(const GameState *g, int remainingMs);
void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex);
void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex);

/* 镜像对决渲染 */
void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead);
void gfxDrawMirrorDeadTitle(const GameState *g1, const GameState *g2,
                            int p1Dead, int p2Dead, int remainingMs);
void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex);

/* 菜单按钮命中检测 */
int gfxHitMenuButton(int mx, int my);

/* 背景图设置 */
void gfxSetMenuBackgroundImage(const char *path);
void gfxClearMenuBackgroundImage(void);
int  gfxHasMenuBackgroundImage(void);

#endif /* GFX_H */
