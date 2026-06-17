/* ============================================================
 *  main.cpp - 主程序入口
 *  菜单逻辑、主循环
 * ============================================================ */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <easyx.h>

#include "snake.h"
#include "food.h"
#include "record.h"
#include "item.h"
#include "ai.h"
#include "combo.h"
#include "floattext.h"
#include "movingobs.h"
#include "achievement.h"
#include "gfx.h"
#include "input.h"

/* ===================================================
 *  菜单辅助函数
 * =================================================== */

/* 将菜单选择转换为难度值 */
static int menuChoiceToDifficulty(int key)
{
    if (key == '1') return 0;
    if (key == '2') return 1;
    if (key == '3') return 2;
    return -1;
}

/* 循环切换蛇颜色 */
static void cycleSnakeColor(GameConfig *cfg)
{
    cfg->snakeColor = normalizeSnakeColor(cfg->snakeColor);
    cfg->snakeColor = (cfg->snakeColor + 1) % 3;
}

/* 循环切换地图大小 */
static void cycleMapSize(GameConfig *cfg)
{
    cfg->mapSize = normalizeMapSize(cfg->mapSize);
    if (cfg->mapSize == MAP_SIZE_SMALL) cfg->mapSize = MAP_SIZE_MEDIUM;
    else if (cfg->mapSize == MAP_SIZE_MEDIUM) cfg->mapSize = MAP_SIZE_LARGE;
    else cfg->mapSize = MAP_SIZE_SMALL;
}

/* 暂停恢复时重置各模式移动时钟，避免长时间停顿导致一次性多步 */
static void resetMoveClocks(GameState *g, GameState *g2, DWORD now)
{
    if (!g) return;

    if (g->gameMode == MODE_SINGLE || g->gameMode == MODE_SURVIVAL) {
        g->lastTickP1 = now;
    } else if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        g->lastTickP1 = now;
        g->lastTickP2 = now;
    } else if (g->gameMode == MODE_DUAL_MIRROR) {
        g->lastTickP1 = now;
        if (g2) g2->lastTickP1 = now;
    }
}

/* ===================================================
 *  主游戏循环
 * =================================================== */
static int runGame(void)
{
    GameState g;
    GameState g2;               /* 镜像对决 P2 */
    GameConfig cfg;
    int menuPage = MENU_MAIN;
    int hoverIndex = 0;
    int running = 1;
    int dualMode = MODE_DUAL;   /* 双人模式：经典/限时 */
    int singleMode = MODE_SINGLE; /* 单人模式：经典/生存 */
    int mirrorP1Dead = 0, mirrorP2Dead = 0;
    int mirrorEndRequested = 0;
    DWORD lastTick;

    memset(&g, 0, sizeof(g));
    memset(&g2, 0, sizeof(g2));
    srand((unsigned)time(NULL));

    /* 加载配置 */
    int achBlueTotal = 0, achAIKills = 0, achShieldBlocks = 0;

    loadRecordConfig(&g.highScore, &cfg);
    g.achUnlocked = achLoad();
    achLoadState(&achBlueTotal, &achAIKills, &achShieldBlocks);
    g.achBlueTotal = achBlueTotal;
    g.achAIKills = achAIKills;
    g.achShieldBlocks = achShieldBlocks;
    cfg.snakeColor = normalizeSnakeColor(cfg.snakeColor);
    cfg.mapSize = normalizeMapSize(cfg.mapSize);
    g.config = cfg;
    g.mapSize = cfg.mapSize;
    g.gameState = STATE_MENU;

    /* 初始化图形系统 */
    gfxInit();
    lastTick = GetTickCount();

    while (running) {
        /* ==================== 菜单状态 ==================== */
        if (g.gameState == STATE_MENU) {
            int key;
            gfxDrawMenu(&g, menuPage, hoverIndex);
            key = gfxGetMenuInput(&hoverIndex);

            if (key == 'q') key = 'Q';
            if (key == 'm') key = 'M';

            if (menuPage == MENU_MAIN) {
                if (key == '1') { singleMode = MODE_SINGLE; menuPage = MENU_SINGLE_DIFF; hoverIndex = 0; }
                else if (key == '2') { menuPage = MENU_DUAL_MODE; hoverIndex = 0; }
                else if (key == '3') { singleMode = MODE_SURVIVAL; menuPage = MENU_SINGLE_DIFF; hoverIndex = 0; }
                else if (key == '4') { menuPage = MENU_SETTINGS; hoverIndex = 0; }
                else if (key == 'Q') running = 0;
            }
            else if (menuPage == MENU_SINGLE_DIFF) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0) {
                    gameInit(&g, diff, &cfg);
                    if (singleMode == MODE_SURVIVAL) g.gameMode = MODE_SURVIVAL;
                    lastTick = GetTickCount();
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_MODE) {
                if (key == '1') {
                    dualMode = MODE_DUAL;
                    menuPage = MENU_DUAL_DIFF; hoverIndex = 0;
                } else if (key == '2') {
                    dualMode = MODE_DUAL_TIMED;
                    menuPage = MENU_DUAL_DIFF; hoverIndex = 0;
                } else if (key == '3') {
                    menuPage = MENU_DUAL_MIRROR; hoverIndex = 0;
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN; hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_MIRROR) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0) {
                    unsigned seed = (unsigned)time(NULL);
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    srand(seed); gameInit(&g, diff, &cfg);
                    srand(seed); gameInit(&g2, diff, &cfg);
                    g.gameMode = MODE_DUAL_MIRROR;
                    g2.gameMode = MODE_DUAL_MIRROR;
                    g.lastTickP1 = GetTickCount();
                    g2.lastTickP1 = GetTickCount();
                    lastTick = GetTickCount();
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_DUAL_MODE; hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_DUAL_DIFF) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0) {
                    gameInitDual(&g, diff, dualMode, &cfg);
                    lastTick = GetTickCount();
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_DUAL_MODE;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_SETTINGS) {
                if (key == '1') {
                    cycleSnakeColor(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '2') {
                    cycleMapSize(&cfg);
                    g.config = cfg;
                    g.mapSize = cfg.mapSize;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '3' || key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
        }
        /* ==================== 游戏进行中 ==================== */
        else if (g.gameState == STATE_PLAYING) {
            int key = gfxGetKey();
            DWORD now = GetTickCount();

            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';

            if (key == 'P') {
                DWORD pauseNow = GetTickCount();
                g.gameState = STATE_PAUSED;
                lastTick = pauseNow;
                resetMoveClocks(&g, &g2, pauseNow);
            } else if (key == 'Q') {
                running = 0;
            } else {
                /* 单人：WASD 和方向键都控制 P1
                 * 双人：WASD 控制 P1，方向键控制 P2 */
                if (g.gameMode == MODE_SINGLE) {
                    if (key == 'w' || key == 'W' || key == KEY_UP) gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S' || key == KEY_DOWN) gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A' || key == KEY_LEFT) gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D' || key == KEY_RIGHT) gameSetDir(&g.snake, DIR_RIGHT);
                } else if (g.gameMode == MODE_DUAL_MIRROR) {
                    /* 镜像：WASD→P1(g), 方向键→P2(g2) */
                    if (key == 'w' || key == 'W') gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S') gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A') gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D') gameSetDir(&g.snake, DIR_RIGHT);
                    if (key == KEY_UP) gameSetDir(&g2.snake, DIR_UP);
                    if (key == KEY_DOWN) gameSetDir(&g2.snake, DIR_DOWN);
                    if (key == KEY_LEFT) gameSetDir(&g2.snake, DIR_LEFT);
                    if (key == KEY_RIGHT) gameSetDir(&g2.snake, DIR_RIGHT);
                } else {
                    if (key == 'w' || key == 'W') gameSetDir(&g.snake, DIR_UP);
                    if (key == 's' || key == 'S') gameSetDir(&g.snake, DIR_DOWN);
                    if (key == 'a' || key == 'A') gameSetDir(&g.snake, DIR_LEFT);
                    if (key == 'd' || key == 'D') gameSetDir(&g.snake, DIR_RIGHT);
                    if (key == KEY_UP) gameSetDir(&g.snake2, DIR_UP);
                    if (key == KEY_DOWN) gameSetDir(&g.snake2, DIR_DOWN);
                    if (key == KEY_LEFT) gameSetDir(&g.snake2, DIR_LEFT);
                    if (key == KEY_RIGHT) gameSetDir(&g.snake2, DIR_RIGHT);
                }

                {
                    int canMove;
                    if (g.gameMode == MODE_SINGLE)
                        canMove = (now - lastTick >= (DWORD)g.speed);
                    else if (g.gameMode == MODE_DUAL_MIRROR)
                        canMove = 1;  /* 镜像总是检查 tick */
                    else
                        canMove = (now - g.lastTickP1 >= (DWORD)g.speed)
                               || (now - g.lastTickP2 >= (DWORD)g.speed2);

                    if (canMove) {
                        int ret;
                        float dt = (float)(now - lastTick) / 1000.0f;
                        lastTick = now;

                        if (g.gameMode == MODE_SINGLE) {
                        gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                        itemUpdate(&g, dt);
                        comboUpdate(&g, dt);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);
                        ret = gameMove(&g);
                        if (ret == 0) {
                            achCheckAll(&g, g.gameMode);
                            int isNewRecord = g.score > g.highScore;
                            if (isNewRecord) {
                                g.highScore = g.score;
                                saveRecordConfig(g.highScore, &cfg);
                            }
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    } else if (g.gameMode == MODE_DUAL_MIRROR) {
                        /* 镜像对决：各自独立 tick */
                        int p1Ready = !mirrorP1Dead &&
                            (now - g.lastTickP1 >= (DWORD)g.speed);
                        int p2Ready = !mirrorP2Dead &&
                            (now - g2.lastTickP1 >= (DWORD)g2.speed);

                        if (p1Ready) {
                            gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                            itemUpdate(&g, dt);
                            comboUpdate(&g, dt);
                            floatTextUpdate(&g, dt);
                            g.elapsedTime += dt;
                            g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                            if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                            movingObsUpdate(&g, dt);
                            aiUpdate(&g, dt);
                            gameUpdateRedTimer(&g, dt);
                            if (gameMove(&g) == 0) { achCheckAll(&g, g.gameMode); mirrorP1Dead = 1; }
                            g.lastTickP1 = now;
                        }
                        if (p2Ready) {
                            gameApplySpeed(&g2, 0, gfxIsKeyDown(VK_BOOST_P2));
                            itemUpdate(&g2, dt);
                            comboUpdate(&g2, dt);
                            floatTextUpdate(&g2, dt);
                            g2.elapsedTime += dt;
                            g2.diffFactor = 1.0f + g2.elapsedTime / 120.0f;
                            if (g2.diffFactor > 3.0f) g2.diffFactor = 3.0f;
                            movingObsUpdate(&g2, dt);
                            aiUpdate(&g2, dt);
                            gameUpdateRedTimer(&g2, dt);
                            if (gameMove(&g2) == 0) { achCheckAll(&g2, g2.gameMode); mirrorP2Dead = 1; }
                            g2.lastTickP1 = now;
                        }
                        /* 检测"立即结算"按钮点击 */
                        if (mirrorP1Dead != mirrorP2Dead) {
                            ExMessage em;
                            while (peekmessage(&em, EX_MOUSE, true)) {
                                if (em.message == WM_LBUTTONDOWN &&
                                    gfxHitMirrorEndButton(em.x, em.y))
                                    mirrorEndRequested = 1;
                            }
                        }
                        if ((mirrorP1Dead && mirrorP2Dead) || mirrorEndRequested) {
                            if (g.score > g2.score) g.winner = WIN_P1;
                            else if (g2.score > g.score) g.winner = WIN_P2;
                            else g.winner = WIN_DRAW;
                            if (g.winner != WIN_DRAW)
                                achUnlock(&g, ACH_MIRRORWIN);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    } else {
                        int p1Ready = (now - g.lastTickP1 >= (DWORD)g.speed);
                        int p2Ready = (now - g.lastTickP2 >= (DWORD)g.speed2);
                        gameApplySpeed(&g,
                            gfxIsKeyDown(VK_BOOST_P1),
                            gfxIsKeyDown(VK_BOOST_P2));
                        itemUpdate(&g, dt);
                        comboUpdate(&g, dt);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);
                        ret = gameMoveDual(&g, p1Ready, p2Ready);
                        if (p1Ready) g.lastTickP1 = now;
                        if (p2Ready) g.lastTickP2 = now;
                        if (ret == 0) {
                            achCheckAll(&g, g.gameMode);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        } else if (g.gameMode == MODE_DUAL_TIMED &&
                                   !gameUpdateTimer(&g, dt)) {
                            /* 时间到，比分高者胜 */
                            if (g.score > g.score2) g.winner = WIN_P1;
                            else if (g.score2 > g.score) g.winner = WIN_P2;
                            else g.winner = WIN_DRAW;
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    }
                }
                }
            }
            if (g.gameMode == MODE_DUAL_MIRROR)
                gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
            else
                gfxDrawGame(&g);
        }
        /* ==================== 暂停 ==================== */
        else if (g.gameState == STATE_PAUSED) {
            int key = gfxGetKey();
            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';
            if (g.gameMode == MODE_DUAL_MIRROR) {
                gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
            } else {
                gfxDrawGame(&g);
            }
            gfxDrawPause();
            if (key == 'P') {
                DWORD resumeNow = GetTickCount();
                g.gameState = STATE_PLAYING;
                lastTick = resumeNow;
                resetMoveClocks(&g, &g2, resumeNow);
            } else if (key == 'Q') {
                running = 0;
            }
        }
        /* ==================== 死亡标题 ==================== */
        else if (g.gameState == STATE_DEAD_TITLE) {
            int key = gfxGetKey();
            if (key == 'q') key = 'Q';
            if (g.gameMode == MODE_DUAL_MIRROR) {
                gfxDrawMirrorDeadTitle(&g, &g2, mirrorP1Dead, mirrorP2Dead);
            } else {
                gfxDrawDeadTitle(&g);
            }
            if (key == 'Q') {
                running = 0;
            } else if (key != 0) {
                g.gameState = STATE_DEAD;
                hoverIndex = 0;
            }
        }
        /* ==================== 结算画面 ==================== */
        else if (g.gameState == STATE_DEAD) {
            int key = gfxGetDeadInput(&hoverIndex);
            if (key == 'q') key = 'Q';
            if (key == 'r') key = 'R';
            if (key == 'm') key = 'M';

            if (g.gameMode == MODE_DUAL_MIRROR) {
                int w;
                if (g.score > g2.score) w = WIN_P1;
                else if (g2.score > g.score) w = WIN_P2;
                else w = WIN_DRAW;
                gfxDrawMirrorOver(g.score, g2.score, w, hoverIndex);
            } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                gfxDrawDualOver(&g, g.winner, g.score, g.score2, hoverIndex);
            else
                gfxDrawGameOver(&g, g.score, g.highScore, g.score == g.highScore, hoverIndex);

            if (key == 'R') {
                if (g.gameMode == MODE_DUAL_MIRROR) {
                    unsigned seed = (unsigned)time(NULL);
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    srand(seed); gameInit(&g, g.difficulty, &cfg);
                    srand(seed); gameInit(&g2, g2.difficulty, &cfg);
                    g.gameMode = MODE_DUAL_MIRROR;
                    g2.gameMode = MODE_DUAL_MIRROR;
                    g.lastTickP1 = g2.lastTickP1 = GetTickCount();
                    lastTick = GetTickCount();
                } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                    gameInitDual(&g, g.difficulty, g.gameMode, &cfg);
                else gameInit(&g, g.difficulty, &cfg);
                lastTick = GetTickCount();
            } else if (key == 'M') {
                g.gameState = STATE_MENU;
                menuPage = MENU_MAIN;
                hoverIndex = 0;
            } else if (key == 'Q') {
                running = 0;
            }
        }

        Sleep(16);
    }

    gfxClose();
    return 0;
}

/* ===================================================
 *  程序入口
 * =================================================== */
int main(void)
{
    return runGame();
}
