/* ============================================================
 *  gfx.cpp - 视图与输入实现
 *
 *  将原先 gfx.h 中全部实现迁移到本文件。
 *  本文件不改变游戏逻辑，仅负责窗口绘制、输入解包与
 *  与 UI 相关的状态展示。
 * ============================================================ */
#include "gfx.h"
#include "item.h"
#include "ai.h"
#include "combo.h"
#include "floattext.h"
#include "achievement.h"

#include <easyx.h>
#include <stdio.h>
#include <tchar.h>

/* ===================================================
 *  颜色定义（复古像素风）
 * =================================================== */

/* 背景 */
#define CLR_BG_LIGHT   RGB(55, 55, 55)
#define CLR_BG_DARK    RGB(40, 40, 40)
#define CLR_BG         RGB(45, 45, 45)

/* 棋盘网格 */
#define CLR_GRID_BASE  RGB(60, 60, 60)
#define CLR_GRID_LINE  RGB(75, 75, 75)
#define CLR_GRID_LIGHT RGB(70, 70, 70)
#define CLR_GRID_DARK  RGB(55, 55, 55)

/* 木质边框 */
#define CLR_WOOD       RGB(160, 95, 50)
#define CLR_WOOD_LIGHT RGB(195, 130, 75)
#define CLR_WOOD_DARK  RGB(110, 65, 30)
#define CLR_WALL       RGB(160, 95, 50)
#define CLR_WALL_LINE  RGB(110, 65, 30)

/* 障碍物 */
#define CLR_OBS        RGB(150, 150, 150)
#define CLR_OBS_CRACK  RGB(120, 120, 120)

/* P1 蛇 */
#define CLR_S1_H       RGB(20, 130, 35)
#define CLR_S1_B1      RGB(40, 180, 55)
#define CLR_S1_B2      RGB(80, 210, 95)
#define CLR_S1_B3      RGB(140, 240, 160)

#define CLR_S1B_H      RGB(30, 100, 180)
#define CLR_S1B_B1     RGB(50, 140, 220)
#define CLR_S1B_B2     RGB(95, 175, 240)
#define CLR_S1B_B3     RGB(150, 210, 255)

#define CLR_S1P_H      RGB(150, 30, 30)
#define CLR_S1P_B1     RGB(200, 60, 60)
#define CLR_S1P_B2     RGB(225, 100, 100)
#define CLR_S1P_B3     RGB(240, 150, 150)

/* P2 蛇 */
#define CLR_S2_H       RGB(180, 130, 20)
#define CLR_S2_B1      RGB(220, 170, 50)
#define CLR_S2_B2      RGB(240, 200, 90)
#define CLR_S2_B3      RGB(255, 230, 130)

/* AI 蛇（紫色） */
#define CLR_AI_H       RGB(130, 30, 180)
#define CLR_AI_B1      RGB(160, 50, 210)
#define CLR_AI_B2      RGB(185, 90, 230)
#define CLR_AI_B3      RGB(210, 140, 245)

/* 食物 */
#define CLR_BLUE_FOOD  RGB(80, 150, 230)
#define CLR_RED_FOOD   RGB(230, 70, 70)

/* 道具 */
#define CLR_TURBO      RGB(255, 220, 0)
#define CLR_TURBO_EDGE RGB(255, 255, 150)
#define CLR_SHIELD     RGB(60, 140, 230)
#define CLR_SHIELD_EDGE RGB(100, 200, 255)
#define CLR_SLOW       RGB(160, 60, 200)
#define CLR_SLOW_EDGE  RGB(220, 80, 80)   /* 警告红框 */
#define CLR_MAGNET     RGB(220, 70, 60)
#define CLR_MAGNET_EDGE RGB(180, 40, 30)
#define CLR_FREEZE     RGB(100, 200, 230)
#define CLR_FREEZE_EDGE RGB(200, 240, 255)
#define CLR_SHRINK     RGB(180, 50, 50)
#define CLR_SHRINK_EDGE RGB(220, 80, 80)  /* 警告红框 */
#define CLR_GHOST      RGB(230, 230, 250)
#define CLR_GHOST_EDGE RGB(150, 180, 220)
#define CLR_DOUBLE     RGB(255, 200, 0)
#define CLR_DOUBLE_EDGE RGB(255, 240, 150)
#define CLR_NEG_BORDER RGB(220, 60, 60)

/* 文字 */
#define CLR_TITLE      RGB(255, 255, 255)
#define CLR_TEXT       RGB(230, 230, 230)
#define CLR_GOLD       RGB(255, 200, 0)
#define CLR_HINT       RGB(180, 180, 180)

/* 菜单边框 */
#define CLR_BORDER     RGB(160, 95, 50)
#define CLR_BORDER_IN  RGB(110, 65, 30)
#define CLR_BORDER_HI  RGB(195, 130, 75)

/* ===================================================
 *  内部绘制辅助
 * =================================================== */

static void drawTextCenter(int x, int y, int w, int h, LPCTSTR text, COLORREF color)
{
    RECT r;
    r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
    setbkmode(TRANSPARENT);
    settextcolor(color);
    drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void drawCheckerboard(void)
{
    setfillcolor(CLR_BG);
    setlinecolor(CLR_BG);
    solidrectangle(0, 0, WIN_W + 1, WIN_H + 1);
}

static void drawMapChecker(int ox, int oy, int mapSize)
{
    int x, y;
    setfillcolor(CLR_GRID_BASE);
    solidrectangle(ox, oy, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);

    setlinecolor(CLR_GRID_LINE);
    setlinestyle(PS_SOLID, 1);
    for (x = 1; x < mapSize; x++)
        line(ox + x * CELL_PX, oy, ox + x * CELL_PX, oy + mapSize * CELL_PX);
    for (y = 1; y < mapSize; y++)
        line(ox, oy + y * CELL_PX, ox + mapSize * CELL_PX, oy + y * CELL_PX);
}

static void drawWoodFrame(int ox, int oy, int mapSize)
{
    int frameThick = 14;
    int x1 = ox - frameThick;
    int y1 = oy - frameThick;
    int x2 = ox + mapSize * CELL_PX + frameThick;
    int y2 = oy + mapSize * CELL_PX + frameThick;

    setfillcolor(CLR_WOOD);
    solidrectangle(x1, y1, x2, oy);
    solidrectangle(x1, oy + mapSize * CELL_PX, x2, y2);
    solidrectangle(x1, oy, ox, oy + mapSize * CELL_PX);
    solidrectangle(ox + mapSize * CELL_PX, oy, x2, oy + mapSize * CELL_PX);

    setlinecolor(CLR_WOOD_LIGHT);
    setlinestyle(PS_SOLID, 1);
    line(x1, y1, x2 - 1, y1);
    line(x1, y1, x1, y2 - 1);

    setlinecolor(CLR_WOOD_DARK);
    line(x1, y2 - 1, x2 - 1, y2 - 1);
    line(x2 - 1, y1, x2 - 1, y2 - 1);

    setlinecolor(CLR_WOOD_DARK);
    rectangle(ox - 1, oy - 1, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);
}

static void drawPixelBorder(int x, int y, int w, int h, COLORREF outer, COLORREF inner)
{
    setlinecolor(outer);
    rectangle(x, y, x + w, y + h);
    setlinecolor(inner);
    rectangle(x + 2, y + 2, x + w - 2, y + h - 2);
}

static void drawPixelPanel(int x, int y, int w, int h, COLORREF fill)
{
    setfillcolor(fill);
    solidrectangle(x + 3, y + 3, x + w - 3, y + h - 3);
    drawPixelBorder(x, y, w, h, CLR_BORDER, CLR_BORDER_IN);
}

static void drawPixelSnakeHead(int left, int top, COLORREF headColor, int dir)
{
    (void)dir;
    setfillcolor(headColor);
    solidrectangle(left + 1, top + 1, left + CELL_PX - 1, top + CELL_PX - 1);
}

static void drawPixelSnakeBody(int left, int top, COLORREF color)
{
    setfillcolor(color);
    solidrectangle(left + 1, top + 1, left + CELL_PX - 1, top + CELL_PX - 1);
}

static void drawPixelWall(int left, int top)
{
    setfillcolor(CLR_WALL);
    solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
}

static void drawPixelObs(int left, int top)
{
    int pad = 4;
    setfillcolor(CLR_OBS);
    solidrectangle(left + pad, top + pad, left + CELL_PX - pad, top + CELL_PX - pad);
}

static void drawPixelBlueFood(int left, int top)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int r = CELL_PX / 2 - 5;
    setfillcolor(CLR_BLUE_FOOD);
    setlinecolor(CLR_BLUE_FOOD);
    solidcircle(cx, cy, r);
}

static void drawPixelRedFood(int left, int top)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int r = CELL_PX / 2 - 5;
    setfillcolor(CLR_RED_FOOD);
    setlinecolor(CLR_RED_FOOD);
    solidcircle(cx, cy, r);
}

/* 绘制道具 — 彩色圆 + 字母标识 + 区分边框 */
static void drawPixelItem(int left, int top, int itemType)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int r = CELL_PX / 2 - 4;
    COLORREF fill, edge;
    TCHAR symbol[2] = { _T('?'), 0 };

    switch (itemType) {
    case ITEM_TURBO:
        fill = CLR_TURBO; edge = CLR_TURBO_EDGE; symbol[0] = _T('Z'); break;
    case ITEM_SHIELD:
        fill = CLR_SHIELD; edge = CLR_SHIELD_EDGE; symbol[0] = _T('S'); break;
    case ITEM_SLOW:
        fill = CLR_SLOW; edge = CLR_NEG_BORDER;  symbol[0] = _T('-'); break;
    case ITEM_MAGNET:
        fill = CLR_MAGNET; edge = CLR_MAGNET_EDGE; symbol[0] = _T('M'); break;
    case ITEM_FREEZE:
        fill = CLR_FREEZE; edge = CLR_FREEZE_EDGE; symbol[0] = _T('F'); break;
    case ITEM_SHRINK:
        fill = CLR_SHRINK; edge = CLR_NEG_BORDER;  symbol[0] = _T('X'); break;
    case ITEM_GHOST:
        fill = CLR_GHOST; edge = CLR_GHOST_EDGE; symbol[0] = _T('G'); break;
    case ITEM_DOUBLE:
        fill = CLR_DOUBLE; edge = CLR_DOUBLE_EDGE; symbol[0] = _T('2'); break;
    default: return;
    }

    /* 外圈（亮色或红框） */
    setlinecolor(edge);
    setfillcolor(fill);
    solidcircle(cx, cy, r);

    /* 内圈字母 */
    setbkmode(TRANSPARENT);
    settextcolor(itemIsNegative(itemType) ? CLR_NEG_BORDER : RGB(255, 255, 255));
    settextstyle(14, 0, _T("Microsoft YaHei"));
    {
        RECT rect = { left, top, left + CELL_PX, top + CELL_PX };
        drawtext(symbol, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

/* 绘制效果状态指示器（顶部状态栏） */
/* 绘制飘字 */
static void drawFloatTexts(const GameState *g, int ox, int oy)
{
    int i;
    for (i = 0; i < MAX_FLOAT_TEXTS; i++) {
        const FloatText *ft = &g->floatTexts[i];
        if (ft->life > 0.0f) {
            int alpha = (int)(ft->life * 212.0f);  /* 淡出 */
            if (alpha > 255) alpha = 255;
            settextstyle(16, 0, _T("Consolas"));
            setbkmode(TRANSPARENT);
            settextcolor(RGB(
                (GetRValue(ft->color) * alpha) / 255,
                (GetGValue(ft->color) * alpha) / 255,
                (GetBValue(ft->color) * alpha) / 255));
            {
                int px = ox + (int)(ft->x * CELL_PX);
                int py = oy + (int)(ft->y * CELL_PX);
                outtextxy(px, py, ft->text);
            }
        }
    }
}

/* 成就解锁通知 */
static void drawAchNotification(const GameState *g)
{
    if (g->achNewUnlock) {
        int i, y = 80;
        settextstyle(22, 0, _T("Microsoft YaHei"));
        setbkmode(TRANSPARENT);
        for (i = 0; i < ACH_MAX; i++) {
            if (g->achNewUnlock & (1u << i)) {
                TCHAR buf[64];
                _stprintf(buf, _T("🏆 成就解锁: %s!"), achName(i));
                settextcolor(CLR_GOLD);
                outtextxy(WIN_W / 2 - 120, y, buf);
                y += 26;
            }
        }
    }
}

static void drawEffectIndicators(const GameState *g, int player, int startX, int startY)
{
    TCHAR buf[32];
    int y = startY;

    settextstyle(14, 0, _T("Microsoft YaHei"));

    if (itemHasEffect(g, player, EFF_TURBO)) {
        settextcolor(CLR_TURBO);
        _stprintf(buf, _T("⚡极速 %.0fs"), itemTimer(g, player, EFF_TURBO));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemShieldCount(g, player) > 0) {
        settextcolor(CLR_SHIELD);
        _stprintf(buf, _T("🛡 护盾 x%d"), itemShieldCount(g, player));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemHasEffect(g, player, EFF_SLOW)) {
        settextcolor(CLR_SLOW);
        _stprintf(buf, _T("🐌减速 %.0fs"), itemTimer(g, player, EFF_SLOW));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemHasEffect(g, player, EFF_MAGNET)) {
        settextcolor(CLR_MAGNET);
        _stprintf(buf, _T("🧲磁铁 %.0fs"), itemTimer(g, player, EFF_MAGNET));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemHasEffect(g, player, EFF_GHOST)) {
        settextcolor(CLR_GHOST_EDGE);
        _stprintf(buf, _T("👻穿墙 %.0fs"), itemTimer(g, player, EFF_GHOST));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemHasEffect(g, player, EFF_DOUBLE)) {
        settextcolor(CLR_DOUBLE);
        _stprintf(buf, _T("x2得分 %.0fs"), itemTimer(g, player, EFF_DOUBLE));
        outtextxy(startX, y, buf);
        y += 20;
    }
    if (itemHasEffect(g, player, EFF_FROZEN)) {
        settextcolor(CLR_FREEZE);
        _stprintf(buf, _T("❄冻结! %.0fs"), itemTimer(g, player, EFF_FROZEN));
        outtextxy(startX, y, buf);
    }
}

static void drawPixelButton(int index, LPCTSTR text, int hover)
{
    int x = WIN_W / 2 - 130;
    int y = 200 + index * 55;
    int w = 260;
    int h = 50;

    COLORREF bgColor = hover ? RGB(95, 95, 95) : RGB(70, 70, 70);
    setfillcolor(bgColor);
    solidrectangle(x + 3, y + 3, x + w - 3, y + h - 3);

    setfillcolor(hover ? RGB(120, 120, 120) : RGB(95, 95, 95));
    solidrectangle(x + 4, y + 4, x + w - 4, y + 9);

    if (hover) drawPixelBorder(x, y, w, h, CLR_BORDER_HI, RGB(140, 140, 140));
    else drawPixelBorder(x, y, w, h, CLR_BORDER, CLR_BORDER_IN);

    settextstyle(22, 0, _T("Microsoft YaHei"));
    drawTextCenter(x, y, w, h, text, hover ? RGB(255, 255, 255) : CLR_TEXT);
}

static void mapOrigin(const GameState *g, int *ox, int *oy)
{
    int mapSize = normalizeMapSize(g->mapSize);
    int mapPx = mapSize * CELL_PX;
    *ox = (WIN_W - mapPx) / 2;
    *oy = 56 + (WIN_H - 56 - 36 - mapPx) / 2;
}

static int snakeBodyIndex(const Snake *s, int x, int y)
{
    int i;
    for (i = 0; i < s->len; i++) {
        if (s->body[i].x == x && s->body[i].y == y) return i;
    }
    return -1;
}

static int aiBodyIndex(const GameState *g, int x, int y)
{
    int a, j;
    for (a = 0; a < g->aiCount; a++) {
        const AISnake *ai = &g->ai[a];
        for (j = 0; j < ai->len; j++) {
            if (ai->body[j].x == x && ai->body[j].y == y) return j;
        }
    }
    return -1;
}

static COLORREF getSnakeBodyColor(int index, int len,
    COLORREF head, COLORREF b1, COLORREF b2, COLORREF b3)
{
    float ratio;
    int r, gC, bC;
    int r1 = GetRValue(b1), g1 = GetGValue(b1), bb1 = GetBValue(b1);
    int r3 = GetRValue(b3), g3 = GetGValue(b3), bb3 = GetBValue(b3);

    (void)b2;
    if (len <= 1) return head;
    if (len <= 2) return b1;
    ratio = (float)(index - 1) / (float)(len - 2);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    r  = r1  + (int)((r3 - r1) * ratio);
    gC = g1  + (int)((g3 - g1) * ratio);
    bC = bb1 + (int)((bb3 - bb1) * ratio);
    return RGB(r, gC, bC);
}

static COLORREF getP1BodyColor(int index, int len, int snakeColor)
{
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE)
        return getSnakeBodyColor(index, len, CLR_S1B_H, CLR_S1B_B1, CLR_S1B_B2, CLR_S1B_B3);
    if (snakeColor == SNAKE_COLOR_PURPLE)
        return getSnakeBodyColor(index, len, CLR_S1P_H, CLR_S1P_B1, CLR_S1P_B2, CLR_S1P_B3);
    return getSnakeBodyColor(index, len, CLR_S1_H, CLR_S1_B1, CLR_S1_B2, CLR_S1_B3);
}

static COLORREF getP1HeadColor(int snakeColor)
{
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE) return CLR_S1B_H;
    if (snakeColor == SNAKE_COLOR_PURPLE) return CLR_S1P_H;
    return CLR_S1_H;
}

static COLORREF getP2BodyColor(int index, int len)
{
    return getSnakeBodyColor(index, len, CLR_S2_H, CLR_S2_B1, CLR_S2_B2, CLR_S2_B3);
}

static COLORREF getAIBodyColor(int index, int len)
{
    return getSnakeBodyColor(index, len, CLR_AI_H, CLR_AI_B1, CLR_AI_B2, CLR_AI_B3);
}

static LPCTSTR colorName(int snakeColor)
{
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE) return _T("蓝色");
    if (snakeColor == SNAKE_COLOR_PURPLE) return _T("红色");
    return _T("绿色");
}

static LPCTSTR mapSizeName(int mapSize)
{
    mapSize = normalizeMapSize(mapSize);
    if (mapSize == MAP_SIZE_SMALL) return _T("小(16x16)");
    if (mapSize == MAP_SIZE_MEDIUM) return _T("中(18x18)");
    return _T("大(20x20)");
}

static LPCTSTR difficultyName(int difficulty)
{
    if (difficulty == 0) return _T("简单");
    if (difficulty == 1) return _T("普通");
    return _T("困难");
}

static void drawTextLeft(int x, int y, LPCTSTR text, COLORREF color)
{
    setbkmode(TRANSPARENT);
    settextcolor(color);
    outtextxy(x, y, text);
}

static void drawOverlayPanel(int x, int y, int w, int h)
{
    setfillcolor(CLR_WOOD);
    solidrectangle(x - 3, y - 3, x + w + 3, y + h + 3);
    setfillcolor(CLR_WOOD_LIGHT);
    solidrectangle(x - 3, y - 3, x + w + 3, y - 1);
    solidrectangle(x - 3, y - 3, x - 1, y + h + 3);
    setfillcolor(CLR_WOOD_DARK);
    solidrectangle(x - 3, y + h + 1, x + w + 3, y + h + 3);
    solidrectangle(x + w + 1, y - 3, x + w + 3, y + h + 3);
    setfillcolor(RGB(35, 35, 35));
    solidrectangle(x, y, x + w, y + h);
}

static int s_deadBtnX = 0, s_deadBtnW = 0, s_deadBtnH = 28, s_deadBtnLineH = 32;
static int s_deadBtnY[3] = {0, 0, 0};

static void drawKeyHintsVertical(int px, int pw, int startY, int hoverIndex)
{
    int i;
    COLORREF fg[3] = { RGB(60, 200, 70), RGB(195, 130, 75), RGB(220, 80, 80) };
    LPCTSTR labels[3] = { _T("R 重新开始"), _T("M 返回菜单"), _T("Q 退出游戏") };

    s_deadBtnX = px + 30;
    s_deadBtnW = pw - 60;
    s_deadBtnH = 28;
    s_deadBtnLineH = 32;
    for (i = 0; i < 3; i++) s_deadBtnY[i] = startY + i * s_deadBtnLineH;

    settextstyle(20, 0, _T("Microsoft YaHei"));
    for (i = 0; i < 3; i++) {
        if (hoverIndex == i + 1) {
            setfillcolor(RGB(60, 60, 60));
            solidrectangle(s_deadBtnX, s_deadBtnY[i], s_deadBtnX + s_deadBtnW, s_deadBtnY[i] + s_deadBtnH);
        }
        drawTextCenter(s_deadBtnX, s_deadBtnY[i], s_deadBtnW, s_deadBtnH, labels[i], fg[i]);
    }
}

int gfxHitDeadButton(int mx, int my)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (mx >= s_deadBtnX && mx <= s_deadBtnX + s_deadBtnW &&
            my >= s_deadBtnY[i] && my <= s_deadBtnY[i] + s_deadBtnH) {
            return i + 1;
        }
    }
    return 0;
}

/* 绘制单个地图网格（不含cleardevice和UI文字） */
static void drawMapAt(const GameState *g, int ox, int oy)
{
    int x, y, mapSize;
    mapSize = normalizeMapSize(g->mapSize);

    drawWoodFrame(ox, oy, mapSize);
    drawMapChecker(ox, oy, mapSize);

    for (y = 0; y < mapSize; y++) {
        for (x = 0; x < mapSize; x++) {
            int left = ox + x * CELL_PX;
            int top = oy + y * CELL_PX;
            int cell = g->grid[y][x];

            if (cell == CELL_WALL) {
                setfillcolor(CLR_WOOD);
                solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
            } else if (cell == CELL_OBS) {
                drawPixelObs(left, top);
            } else if (cell == CELL_BLUE) {
                drawPixelBlueFood(left, top);
            } else if (cell == CELL_RED) {
                drawPixelRedFood(left, top);
            } else if (cell == CELL_SNAKE) {
                int idx = snakeBodyIndex(&g->snake, x, y);
                if (idx == 0) drawPixelSnakeHead(left, top, getP1HeadColor(g->config.snakeColor), g->snake.dir);
                else if (idx > 0) drawPixelSnakeBody(left, top, getP1BodyColor(idx, g->snake.len, g->config.snakeColor));
            } else if (cell == CELL_SNAKE2) {
                int idx = snakeBodyIndex(&g->snake2, x, y);
                if (idx == 0) drawPixelSnakeHead(left, top, CLR_S2_H, g->snake2.dir);
                else if (idx > 0) drawPixelSnakeBody(left, top, getP2BodyColor(idx, g->snake2.len));
            } else if (cell == CELL_AI) {
                int idx = aiBodyIndex(g, x, y);
                if (idx == 0) drawPixelSnakeHead(left, top, CLR_AI_H, DIR_UP);
                else if (idx > 0) drawPixelSnakeBody(left, top, getAIBodyColor(idx, MAX_LEN));
            }
        }
    }

    /* 绘制场上道具 */
    if (g->itemOnField != ITEM_NONE) {
        int ileft = ox + g->itemPos.x * CELL_PX;
        int itop = oy + g->itemPos.y * CELL_PX;
        drawPixelItem(ileft, itop, g->itemOnField);
    }
}

static void drawGameContent(const GameState *g)
{
    int ox, oy, mapSize;
    TCHAR buf[128];

    if (!g) return;
    mapSize = normalizeMapSize(g->mapSize);
    mapOrigin(g, &ox, &oy);

    cleardevice();
    drawCheckerboard();

    settextstyle(18, 0, _T("Microsoft YaHei"));
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        _stprintf(buf, _T("P1: %d"), g->score);
        drawTextLeft(20, 24, buf, CLR_TEXT);
        _stprintf(buf, _T("P2: %d"), g->score2);
        drawTextLeft(140, 24, buf, CLR_TEXT);
        drawTextLeft(280, 24, difficultyName(g->difficulty), CLR_TEXT);
        if (g->gameMode == MODE_DUAL_TIMED) {
            int mins = (int)g->matchTimer / 60;
            int secs = (int)g->matchTimer % 60;
            _stprintf(buf, _T("⏱ %d:%02d"), mins, secs);
            drawTextLeft(WIN_W - 100, 24, buf,
                g->matchTimer <= 10.0f ? CLR_RED_FOOD : CLR_GOLD);
        }
    } else {
        _stprintf(buf, _T("得分: %d"), g->score);
        drawTextLeft(20, 24, buf, CLR_TEXT);
        _stprintf(buf, _T("最高: %d"), g->highScore);
        drawTextLeft(140, 24, buf, CLR_TEXT);
        drawTextLeft(280, 24, difficultyName(g->difficulty), CLR_TEXT);
    }

    drawMapAt(g, ox, oy);

    /* 效果指示器 */
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawEffectIndicators(g, 0, WIN_W - 160, 24);
        drawEffectIndicators(g, 1, 20, 56);
    } else {
        drawEffectIndicators(g, 0, WIN_W - 160, 24);
    }

    if (g->hasRed) {
        int barX = WIN_W - 140;
        int barY = 28;
        int barW = 120;
        int barH = 10;
        int filledW = (int)((float)barW * g->redTimer / (float)RED_TIMER_MAX);
        if (filledW < 0) filledW = 0;
        setfillcolor(RGB(70, 70, 70));
        solidrectangle(barX, barY, barX + barW, barY + barH);
        setfillcolor(CLR_RED_FOOD);
        solidrectangle(barX, barY, barX + filledW, barY + barH);
    }

    settextstyle(15, 0, _T("Microsoft YaHei"));
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawTextLeft(20, WIN_H - 28, _T("P1:WASD+J  P2:方向键+0  捡道具   P暂停  Q退出"), CLR_HINT);
    } else if (g->gameMode == MODE_DUAL_MIRROR) {
        /* 镜像模式底部提示由 gfxDrawMirrorGame 绘制 */
    } else if (g->gameMode == MODE_SURVIVAL) {
        drawTextLeft(20, WIN_H - 28, _T("WASD移动   J加速   捡道具   生存中..."), CLR_HINT);
    } else {
        drawTextLeft(20, WIN_H - 28, _T("WASD移动   J加速   捡道具   P暂停   Q退出"), CLR_HINT);
    }

    /* 飘字 */
    drawFloatTexts(g, ox, oy);
    /* 连击显示 */
    if (comboCount(g) >= 2) {
        TCHAR cbuf[32];
        _stprintf(cbuf, _T("COMBO x%d!"), comboCount(g));
        settextstyle(22, 0, _T("Microsoft YaHei"));
        settextcolor(CLR_GOLD);
        outtextxy(WIN_W/2 - 50, 50, cbuf);
    }
    /* 生存时间 */
    if (g->gameMode == MODE_SURVIVAL) {
        TCHAR sbuf[32];
        int m = (int)g->elapsedTime / 60, s = (int)g->elapsedTime % 60;
        _stprintf(sbuf, _T("⏱ %d:%02d"), m, s);
        settextstyle(18, 0, _T("Consolas"));
        settextcolor(CLR_TEXT);
        outtextxy(WIN_W - 110, 50, sbuf);
    }
    /* 成就通知 */
    drawAchNotification(g);
}

void gfxInit(void)
{
    initgraph(WIN_W, WIN_H);
    setbkcolor(CLR_BG);
    cleardevice();
    setfillcolor(CLR_BG);
    setlinecolor(CLR_BG);
    solidrectangle(0, 0, WIN_W + 1, WIN_H + 1);
    BeginBatchDraw();
}

void gfxClose(void)
{
    EndBatchDraw();
    closegraph();
}

void gfxDrawMenu(const GameState *g, int menuPage, int hoverIndex)
{
    TCHAR buf[128];

    cleardevice();
    drawCheckerboard();

    settextstyle(60, 0, _T("Microsoft YaHei"));
    drawTextCenter(0, 40, WIN_W, 80, _T("贪吃蛇"), CLR_GOLD);

    setlinecolor(CLR_BORDER);
    line(100, 130, WIN_W - 100, 130);

    settextstyle(18, 0, _T("Microsoft YaHei"));

    if (menuPage == MENU_MAIN) {
        _stprintf(buf, _T("最高分: %d"), g ? g->highScore : 0);
        drawTextCenter(0, 150, WIN_W, 30, buf, CLR_GOLD);

        drawPixelButton(1, _T("1  单人模式"), hoverIndex == 1);
        drawPixelButton(2, _T("2  双人对战"), hoverIndex == 2);
        drawPixelButton(3, _T("3  生存模式"), hoverIndex == 3);
        drawPixelButton(4, _T("4  设置"), hoverIndex == 4);

        drawTextCenter(0, 520, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_SINGLE_DIFF) {
        drawTextCenter(0, 150, WIN_W, 30,
            (g && g->gameMode == MODE_SURVIVAL) ? _T("生存模式 — 选择难度") : _T("选择难度"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_MODE) {
        drawTextCenter(0, 150, WIN_W, 30, _T("双人对战模式"), CLR_TEXT);
        drawPixelButton(1, _T("1  经典对战"), hoverIndex == 1);
        drawPixelButton(2, _T("2  限时赛"), hoverIndex == 2);
        drawPixelButton(3, _T("3  镜像对决"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_MIRROR) {
        drawTextCenter(0, 150, WIN_W, 30, _T("镜像对决 — 选择难度"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("两张相同地图各自独立"), CLR_HINT);
    }
    else if (menuPage == MENU_DUAL_DIFF) {
        drawTextCenter(0, 150, WIN_W, 30, _T("选择难度"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("P1: WASD  |  P2: 方向键"), CLR_HINT);
    }
    else {
        drawTextCenter(0, 150, WIN_W, 30, _T("游戏设置"), CLR_TEXT);
        _stprintf(buf, _T("1  蛇颜色: %s"), colorName(g ? g->config.snakeColor : SNAKE_COLOR_GREEN));
        drawPixelButton(1, buf, hoverIndex == 1);
        _stprintf(buf, _T("2  地图: %s"), mapSizeName(g ? g->config.mapSize : MAP_SIZE_LARGE));
        drawPixelButton(2, buf, hoverIndex == 2);
        drawPixelButton(3, _T("3  返回"), hoverIndex == 3);
        drawTextCenter(0, 520, WIN_W, 30, _T("自动保存"), CLR_HINT);
    }

    FlushBatchDraw();
}

void gfxDrawGame(const GameState *g)
{
    drawGameContent(g);
    FlushBatchDraw();
}

void gfxDrawPause(void)
{
    drawPixelPanel(WIN_W / 2 - 120, WIN_H / 2 - 80, 240, 160, RGB(30, 30, 30));

    settextstyle(36, 0, _T("Microsoft YaHei"));
    drawTextCenter(0, WIN_H / 2 - 55, WIN_W, 45, _T("PAUSED"), CLR_TITLE);

    settextstyle(18, 0, _T("Microsoft YaHei"));
    drawTextCenter(0, WIN_H / 2 + 10, WIN_W, 30, _T("按 P 继续"), CLR_TEXT);
    drawTextCenter(0, WIN_H / 2 + 40, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);

    FlushBatchDraw();
}

void gfxDrawDeadTitle(const GameState *g)
{
    int px, py, pw, ph;

    if (g) drawGameContent(g);

    pw = 360;
    ph = 140;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(40, 0, _T("Microsoft YaHei"));
    if (g && g->gameMode == MODE_DUAL_TIMED && g->matchTimer <= 0.0f)
        drawTextCenter(px, py + 18, pw, 50, _T("TIME UP!"), CLR_GOLD);
    else
        drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), RGB(230, 70, 70));

    settextstyle(18, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);

    FlushBatchDraw();
}

void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];

    if (g) drawGameContent(g);

    pw = 340;
    ph = 290;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(30, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 14, pw, 40, _T("游戏结束"), CLR_TITLE);

    settextstyle(20, 0, _T("Consolas"));
    _stprintf(buf, _T("SCORE: %d"), score);
    drawTextCenter(px, py + 60, pw, 30, buf, CLR_TEXT);
    _stprintf(buf, _T("HI:    %d"), highScore);
    drawTextCenter(px, py + 90, pw, 30, buf, CLR_TEXT);

    if (isNewRecord) {
        settextstyle(20, 0, _T("Microsoft YaHei"));
        drawTextCenter(px, py + 124, pw, 28, _T("NEW RECORD!"), CLR_GOLD);
    }

    drawKeyHintsVertical(px, pw, py + 160, hoverIndex);
    FlushBatchDraw();
}

void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || (winner != WIN_P1 && winner != WIN_P2)) title = _T("平局!");
    else if (winner == WIN_P1 && score1 > score2) title = _T("P1 获胜!");
    else if (winner == WIN_P2 && score2 > score1) title = _T("P2 获胜!");
    else if (score1 == score2) title = _T("平局!");
    else if (score1 > score2) title = _T("P1 获胜!");
    else title = _T("P2 获胜!");

    if (g) drawGameContent(g);

    pw = 340;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(30, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 14, pw, 40, title, CLR_TITLE);

    settextstyle(20, 0, _T("Consolas"));
    _stprintf(buf, _T("P1: %d    P2: %d"), score1, score2);
    drawTextCenter(px, py + 70, pw, 30, buf, CLR_TEXT);

    drawKeyHintsVertical(px, pw, py + 140, hoverIndex);
    FlushBatchDraw();
}

/* ===================================================
 *  镜像对决渲染
 * =================================================== */

/* 镜像模式中间按钮状态 */
static int s_mirrorBtnX = 0, s_mirrorBtnY = 0, s_mirrorBtnW = 120, s_mirrorBtnH = 40;

static void drawMirrorEndButton(void)
{
    int cx = MIRROR_GAP_CX;
    int cy = WIN_H / 2 + 100;
    s_mirrorBtnX = cx - s_mirrorBtnW / 2;
    s_mirrorBtnY = cy;
    s_mirrorBtnW = 120;
    s_mirrorBtnH = 40;

    setfillcolor(RGB(200, 60, 60));
    solidrectangle(s_mirrorBtnX, s_mirrorBtnY,
                   s_mirrorBtnX + s_mirrorBtnW, s_mirrorBtnY + s_mirrorBtnH);
    settextstyle(18, 0, _T("Microsoft YaHei"));
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    {
        RECT r = { s_mirrorBtnX, s_mirrorBtnY,
                   s_mirrorBtnX + s_mirrorBtnW, s_mirrorBtnY + s_mirrorBtnH };
        drawtext(_T("立即结算"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

int gfxHitMirrorEndButton(int mx, int my)
{
    if (mx >= s_mirrorBtnX && mx <= s_mirrorBtnX + s_mirrorBtnW &&
        my >= s_mirrorBtnY && my <= s_mirrorBtnY + s_mirrorBtnH)
        return 1;
    return 0;
}

void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead)
{
    TCHAR buf[128];

    cleardevice();
    drawCheckerboard();

    /* P1 分数 */
    settextstyle(20, 0, _T("Microsoft YaHei"));
    _stprintf(buf, _T("P1: %d"), g1 ? g1->score : 0);
    settextcolor(CLR_TEXT);
    outtextxy(60, 20, buf);

    /* P2 分数 */
    _stprintf(buf, _T("P2: %d"), g2 ? g2->score : 0);
    outtextxy(MIRROR_MAP2_X + 20, 20, buf);

    /* P1 地图 */
    if (g1) {
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
        drawEffectIndicators(g1, 0, 60, 42);
    }

    /* P2 地图 */
    if (g2) {
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
        drawEffectIndicators(g2, 1, MIRROR_MAP2_X + 20, 42);
    }

    /* 死亡提示 + 结算按钮 */
    if (p1Dead || p2Dead) {
        settextstyle(24, 0, _T("Microsoft YaHei"));
        if (p1Dead) {
            settextcolor(RGB(230, 70, 70));
            outtextxy(MIRROR_MAP1_X + 140, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        if (p2Dead) {
            settextcolor(RGB(230, 70, 70));
            outtextxy(MIRROR_MAP2_X + 140, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        /* 只有一方死亡时显示结算按钮 */
        if (p1Dead != p2Dead) {
            drawMirrorEndButton();
        }
    }

    /* 底部提示 */
    settextstyle(15, 0, _T("Microsoft YaHei"));
    settextcolor(CLR_HINT);
    outtextxy(50, WIN_H - 28, _T("P1:WASD+J"));
    outtextxy(MIRROR_MAP2_X, WIN_H - 28, _T("P2:方向键+0   P暂停  Q退出"));

    /* 飘字 + 连击 + 成就 */
    if (g1) { drawFloatTexts(g1, MIRROR_MAP1_X, MIRROR_MAP_Y); }
    if (g2) { drawFloatTexts(g2, MIRROR_MAP2_X, MIRROR_MAP_Y); }
    if (g1) drawAchNotification(g1);
    if (g2) drawAchNotification(g2);

    FlushBatchDraw();
}

void gfxDrawMirrorDeadTitle(const GameState *g1, const GameState *g2,
                            int p1Dead, int p2Dead)
{
    int px, py, pw, ph;

    if (g1 && g2) {
        cleardevice();
        drawCheckerboard();
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
    }

    pw = 360;
    ph = 140;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(40, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), RGB(230, 70, 70));

    settextstyle(18, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);

    FlushBatchDraw();
}

void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || score1 == score2)
        title = _T("平局!");
    else if (winner == WIN_P1 || score1 > score2)
        title = _T("P1 获胜!");
    else
        title = _T("P2 获胜!");

    pw = 340;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(30, 0, _T("Microsoft YaHei"));
    drawTextCenter(px, py + 14, pw, 40, title, CLR_TITLE);

    settextstyle(20, 0, _T("Consolas"));
    _stprintf(buf, _T("P1: %d    P2: %d"), score1, score2);
    drawTextCenter(px, py + 70, pw, 30, buf, CLR_TEXT);

    drawKeyHintsVertical(px, pw, py + 140, hoverIndex);
    FlushBatchDraw();
}
