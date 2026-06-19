/* ============================================================
 *  main.cpp - 主程序入口
 *  菜单逻辑、主循环
 * ============================================================ */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <commdlg.h>
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
#include "sound.h"

static void closeHostConsoleIfAny(void)
{
    HWND consoleWnd = GetConsoleWindow();
    if (!consoleWnd) {
        return;
    }

    FreeConsole();
}

/* ===================================================
 *  文件选择对话框
 * =================================================== */
static int pickLocalFile(char *outPath, size_t outSize, const char *title, const char *filter)
{
    OPENFILENAMEA ofn;
    char pathBuffer[MAX_PATH];

    if (!outPath || outSize == 0) return 0;

    memset(pathBuffer, 0, sizeof(pathBuffer));
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = pathBuffer;
    ofn.nMaxFile = (DWORD) sizeof(pathBuffer);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (!GetOpenFileNameA(&ofn)) {
        return 0;
    }

    snprintf(outPath, outSize, "%s", pathBuffer);
    return 1;
}

static void pickLocalMusicFile(char *outPath, size_t outSize)
{
    static const char filter[] =
        "音频文件 (*.wav;*.mp3;*.ogg;*.flac;*.aac;*.m4a;*.mid;*.wma)\0"
        "*.wav;*.mp3;*.ogg;*.flac;*.aac;*.m4a;*.mid;*.wma\0"
        "所有文件 (*.*)\0"
        "*.*\0";

    pickLocalFile(outPath, outSize, "选择背景音乐", filter);
}

static void pickLocalImageFile(char *outPath, size_t outSize)
{
    static const char filter[] =
        "图片文件 (*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp)\0"
        "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp\0"
        "所有文件 (*.*)\0"
        "*.*\0";

    pickLocalFile(outPath, outSize, "选择背景图片", filter);
}

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

static void toggleItemMode(GameConfig *cfg)
{
    cfg->itemMode = normalizeItemMode((cfg->itemMode == GAMEPLAY_CLASSIC) ? GAMEPLAY_ITEM : GAMEPLAY_CLASSIC);
}

static void toggleAiEnabled(GameConfig *cfg)
{
    cfg->aiEnabled = normalizeAiEnabled(!cfg->aiEnabled);
}

static void loadMusicFromDisk(GameConfig *cfg)
{
    char filePath[MAX_PATH];

    if (!cfg) return;

    filePath[0] = '\0';
    pickLocalMusicFile(filePath, sizeof(filePath));
    if (filePath[0] != '\0') {
        snprintf(cfg->menuMusicPath, sizeof(cfg->menuMusicPath), "%s", filePath);
        soundSetBackgroundMusic(filePath);
    }
}

static void loadImageFromDisk(GameConfig *cfg)
{
    char filePath[MAX_PATH];

    if (!cfg) return;

    filePath[0] = '\0';
    pickLocalImageFile(filePath, sizeof(filePath));
    if (filePath[0] != '\0') {
        snprintf(cfg->menuBgImagePath, sizeof(cfg->menuBgImagePath), "%s", filePath);
        gfxSetMenuBackgroundImage(filePath);
    }
}

static void clearCustomMenuBgImage(GameConfig *cfg)
{
    if (!cfg) return;

    gfxClearMenuBackgroundImage();
    cfg->menuBgImagePath[0] = '\0';
}

static void clearCustomMenuMusic(GameConfig *cfg)
{
    if (!cfg) return;

    soundStopBackgroundMusic();
    cfg->menuMusicPath[0] = '\0';
}

static int isSoloMode(int gameMode)
{
    return gameMode == MODE_SINGLE || gameMode == MODE_SURVIVAL;
}

static int pollMirrorEndButton(void)
{
    ExMessage em;
    int hit = 0;
    while (peekmessage(&em, EX_MOUSE, true)) {
        if (em.message == WM_LBUTTONDOWN &&
            gfxHitMirrorEndButton(em.x, em.y)) {
            hit = 1;
        }
    }
    return hit;
}

/* 暂停恢复时重置各模式移动时钟，避免长时间停顿导致一次性多步 */
static void resetMoveClocks(GameState *g, GameState *g2, DWORD now)
{
    if (!g) return;

    if (isSoloMode(g->gameMode)) {
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
    cfg.itemMode = normalizeItemMode(cfg.itemMode);
    cfg.aiEnabled = normalizeAiEnabled(cfg.aiEnabled);
    g.config = cfg;
    g.mapSize = cfg.mapSize;
    g.gameState = STATE_MENU;

    /* 初始化图形系统 */
    gfxInit();
    soundInit();

    if (cfg.menuBgImagePath[0] != '\0') {
        gfxSetMenuBackgroundImage(cfg.menuBgImagePath);
    }
    if (cfg.menuMusicPath[0] != '\0') {
        soundSetBackgroundMusic(cfg.menuMusicPath);
    }

    lastTick = GetTickCount();

    while (running) {
        if (gfxWindowCloseRequested()) {
            running = 0;
            break;
        }

        /* ==================== 菜单状态 ==================== */
        if (g.gameState == STATE_MENU) {
            int key;
            gfxDrawMenu(&g, menuPage, hoverIndex);
            key = gfxGetMenuInput(&hoverIndex);

            if (key == 'q') key = 'Q';
            if (key == 'm') key = 'M';

            if (gfxWindowCloseRequested()) {
                running = 0;
            }
            else if (key == GFX_MENU_ACTION_MUSIC) {
                loadMusicFromDisk(&cfg);
            }
            else if (key == GFX_MENU_ACTION_IMAGE) {
                loadImageFromDisk(&cfg);
            }
            else if (key == GFX_MENU_ACTION_CLEAR_IMAGE) {
                clearCustomMenuBgImage(&cfg);
            }
            else if (key == GFX_MENU_ACTION_CLEAR_MUSIC) {
                clearCustomMenuMusic(&cfg);
            }
            else if (menuPage == MENU_MAIN) {
                if (key == '1') { singleMode = MODE_SINGLE; menuPage = MENU_SINGLE_DIFF; hoverIndex = 0; }
                else if (key == '2') { menuPage = MENU_DUAL_MODE; hoverIndex = 0; }
                else if (key == '3') { menuPage = MENU_HOWTOPLAY; hoverIndex = 0; }
                else if (key == '4') { menuPage = MENU_SETTINGS; hoverIndex = 0; }
                else if (key == '5') { menuPage = MENU_ACHIEVEMENTS; hoverIndex = 0; }
                else if (key == 'Q') running = 0;
            }
            else if (menuPage == MENU_SINGLE_DIFF) {
                int diff = menuChoiceToDifficulty(key);
                if (diff >= 0 || key == '4') {
                    singleMode = (key == '4') ? MODE_SURVIVAL : MODE_SINGLE;
                    if (singleMode == MODE_SURVIVAL)
                        diff = 1;
                    soundPlayMenuConfirm();
                    gameInit(&g, diff, &cfg);
                    g.gameMode = singleMode;
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
                    GameConfig duelCfg = cfg;
                    duelCfg.aiEnabled = 0;
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    soundPlayMenuConfirm();
                    srand(seed); gameInit(&g, diff, &duelCfg);
                    srand(seed); gameInit(&g2, diff, &duelCfg);
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
                    soundPlayMenuConfirm();
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
                } else if (key == '3') {
                    toggleItemMode(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == '4') {
                    toggleAiEnabled(&cfg);
                    g.config = cfg;
                    saveRecordConfig(g.highScore, &cfg);
                } else if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            } else if (menuPage == MENU_ACHIEVEMENTS) {
                if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
            else if (menuPage == MENU_HOWTOPLAY) {
                if (key == 'M' || key == 27) {
                    menuPage = MENU_MAIN;
                    hoverIndex = 0;
                } else if (key == 'Q') {
                    running = 0;
                }
            }
        }
        /* ==================== 游戏进行中 ==================== */
        else if (g.gameState == STATE_PLAYING) {
            int key;
            DWORD now = GetTickCount();

            if (g.gameMode == MODE_DUAL_MIRROR &&
                mirrorP1Dead != mirrorP2Dead &&
                pollMirrorEndButton()) {
                mirrorEndRequested = 1;
            }

            key = gfxGetKey();

            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';

            if (gfxWindowCloseRequested()) {
                running = 0;
            }

            if (key == 'P' && running) {
                soundPlayPause();
                DWORD pauseNow = GetTickCount();
                g.gameState = STATE_PAUSED;
                lastTick = pauseNow;
                resetMoveClocks(&g, &g2, pauseNow);
            } else if (key == 'Q' && running) {
                running = 0;
            } else if (running) {
                /* 单人：WASD 和方向键都控制 P1
                 * 双人：WASD 控制 P1，方向键控制 P2 */
                if (isSoloMode(g.gameMode)) {
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

                if (g.gameMode == MODE_DUAL_MIRROR) {
                    /* ---- 镜像对决：独立 tick，不使用全局 canMove/dt/lastTick ---- */
                    int p1Ready = !mirrorP1Dead &&
                        (now - g.lastTickP1 >= (DWORD)g.speed);
                    int p2Ready = !mirrorP2Dead &&
                        (now - g2.lastTickP1 >= (DWORD)g2.speed);

                    if (p1Ready || p2Ready || mirrorEndRequested) {
                        if (p1Ready) {
                            float dt1 = (float)(now - g.lastTickP1) / 1000.0f;
                            gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                            if (g.config.itemMode == GAMEPLAY_ITEM)
                                itemUpdate(&g, dt1);
                            comboUpdate(&g, dt1);
                            floatTextUpdate(&g, dt1);
                            g.elapsedTime += dt1;
                            g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                            if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                            movingObsUpdate(&g, dt1);
                            if (g.config.aiEnabled)
                                aiUpdate(&g, dt1);
                            gameUpdateRedTimer(&g, dt1);
                            if (gameMove(&g) == 0) { achCheckAll(&g, g.gameMode); mirrorP1Dead = 1; }
                            g.lastTickP1 = now;
                        }
                        if (p2Ready) {
                            float dt2 = (float)(now - g2.lastTickP1) / 1000.0f;
                            /* 镜像 P2 的提速键通过 p1Boost 参数传入：
                             * gameApplySpeed 中 p1Boost→g->speed，P2 实际走 g2.snake/g2.speed */
                            gameApplySpeed(&g2, gfxIsKeyDown(VK_BOOST_P2), 0);
                            if (g2.config.itemMode == GAMEPLAY_ITEM)
                                itemUpdate(&g2, dt2);
                            comboUpdate(&g2, dt2);
                            floatTextUpdate(&g2, dt2);
                            g2.elapsedTime += dt2;
                            g2.diffFactor = 1.0f + g2.elapsedTime / 120.0f;
                            if (g2.diffFactor > 3.0f) g2.diffFactor = 3.0f;
                            movingObsUpdate(&g2, dt2);
                            if (g2.config.aiEnabled)
                                aiUpdate(&g2, dt2);
                            gameUpdateRedTimer(&g2, dt2);
                            if (gameMove(&g2) == 0) { achCheckAll(&g2, g2.gameMode); mirrorP2Dead = 1; }
                            g2.lastTickP1 = now;
                        }
                        /* 检测"立即结算"按钮点击（鼠标消息不再被 gfxGetKey 消费，可正常检测） */
                        if (mirrorP1Dead != mirrorP2Dead && pollMirrorEndButton())
                            mirrorEndRequested = 1;
                        if ((mirrorP1Dead && mirrorP2Dead) || mirrorEndRequested) {
                            soundPlayGameOver();
                            if (g.score > g2.score) g.winner = WIN_P1;
                            else if (g2.score > g.score) g.winner = WIN_P2;
                            else g.winner = WIN_DRAW;
                            if (g.winner != WIN_DRAW)
                                achUnlock(&g, ACH_MIRRORWIN);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    }
                } else {
                    /* ---- 单人 / 双人经典：使用全局 canMove/dt/lastTick ---- */
                    int canMove;
                    if (isSoloMode(g.gameMode))
                        canMove = (now - lastTick >= (DWORD)g.speed);
                    else
                        canMove = (now - g.lastTickP1 >= (DWORD)g.speed)
                               || (now - g.lastTickP2 >= (DWORD)g.speed2);

                    if (canMove) {
                        int ret;
                        float dt = (float)(now - lastTick) / 1000.0f;
                        lastTick = now;

                        if (isSoloMode(g.gameMode)) {
                        gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                        if (g.config.itemMode == GAMEPLAY_ITEM)
                            itemUpdate(&g, dt);
                        comboUpdate(&g, dt);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        if (g.config.aiEnabled)
                            aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);
                        ret = gameMove(&g);
                        if (ret == 0) {
                            soundPlayGameOver();
                            achCheckAll(&g, g.gameMode);
                            int isNewRecord = g.score > g.highScore;
                            if (isNewRecord) {
                                g.highScore = g.score;
                                saveRecordConfig(g.highScore, &cfg);
                            }
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        }
                    } else {
                        int p1Ready = (now - g.lastTickP1 >= (DWORD)g.speed);
                        int p2Ready = (now - g.lastTickP2 >= (DWORD)g.speed2);
                        gameApplySpeed(&g,
                            gfxIsKeyDown(VK_BOOST_P1),
                            gfxIsKeyDown(VK_BOOST_P2));
                        if (g.config.itemMode == GAMEPLAY_ITEM)
                            itemUpdate(&g, dt);
                        comboUpdate(&g, dt);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        if (g.config.aiEnabled)
                            aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);
                        ret = gameMoveDual(&g, p1Ready, p2Ready);
                        if (p1Ready) g.lastTickP1 = now;
                        if (p2Ready) g.lastTickP2 = now;
                        if (ret == 0) {
                            soundPlayGameOver();
                            achCheckAll(&g, g.gameMode);
                            g.deadTick = now;
                            g.gameState = STATE_DEAD_TITLE;
                        } else if (g.gameMode == MODE_DUAL_TIMED &&
                                   !gameUpdateTimer(&g, dt)) {
                            /* 时间到，比分高者胜 */
                            soundPlayGameOver();
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

            if (running) {
                if (g.gameMode == MODE_DUAL_MIRROR)
                    gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
                else
                    gfxDrawGame(&g);
            }
        }
        /* ==================== 暂停 ==================== */
        else if (g.gameState == STATE_PAUSED) {
            int key = gfxGetKey();
            if (key == 'q') key = 'Q';
            if (key == 'p') key = 'P';

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else {
                if (g.gameMode == MODE_DUAL_MIRROR) {
                    gfxDrawMirrorGame(&g, &g2, mirrorP1Dead, mirrorP2Dead);
                } else {
                    gfxDrawGame(&g);
                }
                gfxDrawPause();

                if (key == 'P') {
                    soundPlayResume();
                    DWORD resumeNow = GetTickCount();
                    g.gameState = STATE_PLAYING;
                    lastTick = resumeNow;
                    resetMoveClocks(&g, &g2, resumeNow);
                } else if (key == 'Q') {
                    running = 0;
                }
            }
        }
        /* ==================== 死亡标题 ==================== */
        else if (g.gameState == STATE_DEAD_TITLE) {
            int key = gfxGetKey();
            DWORD now = GetTickCount();
            int elapsed = (int)(now - g.deadTick);
            int remaining = 1000 - elapsed;
            if (remaining < 0) remaining = 0;

            if (key == 'q') key = 'Q';
            if (g.gameMode == MODE_DUAL_MIRROR) {
                gfxDrawMirrorDeadTitle(&g, &g2, mirrorP1Dead, mirrorP2Dead, remaining);
            } else {
                gfxDrawDeadTitle(&g, remaining);
            }

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else if (key == 'Q') {
                running = 0;
            } else if (remaining > 0) {
                /* 缓冲期内：忽略任意键转移（仅允许 Q 退出） */
                ;
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
                gfxDrawGameOver(&g, g.score, g.highScore, g.score >= g.highScore && g.score > 0, hoverIndex);

            if (gfxWindowCloseRequested()) {
                running = 0;
            } else if (key == 'R') {
                soundPlayMenuConfirm();
                if (g.gameMode == MODE_DUAL_MIRROR) {
                    unsigned seed = (unsigned)time(NULL);
                    GameConfig duelCfg = cfg;
                    duelCfg.aiEnabled = 0;
                    mirrorP1Dead = 0; mirrorP2Dead = 0;
                    mirrorEndRequested = 0;
                    srand(seed); gameInit(&g, g.difficulty, &duelCfg);
                    srand(seed); gameInit(&g2, g2.difficulty, &duelCfg);
                    g.gameMode = MODE_DUAL_MIRROR;
                    g2.gameMode = MODE_DUAL_MIRROR;
                    g.lastTickP1 = g2.lastTickP1 = GetTickCount();
                    lastTick = GetTickCount();
                } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                    gameInitDual(&g, g.difficulty, g.gameMode, &cfg);
                else {
                    int restartMode = g.gameMode;
                    gameInit(&g, g.difficulty, &cfg);
                    if (restartMode == MODE_SURVIVAL)
                        g.gameMode = MODE_SURVIVAL;
                }
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

    saveRecordConfig(g.highScore, &cfg);
    gfxClose();
    closeHostConsoleIfAny();
    return 0;
}

/* ===================================================
 *  程序入口
 * =================================================== */
int main(void)
{
    return runGame();
}
