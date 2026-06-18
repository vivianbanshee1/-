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
#include <math.h>
#include <stdio.h>
#include <tchar.h>

/* ===================================================
 *  颜色定义（示例风格视觉方案）
 * =================================================== */

/* 背景 */
#define CLR_BG_LIGHT      RGB(56, 111, 179)
#define CLR_BG_DARK       RGB(14, 44, 87)
#define CLR_BG            RGB(26, 74, 133)
#define CLR_PANEL         RGB(33, 86, 150)
#define CLR_PANEL_SOFT    RGB(45, 104, 173)
#define CLR_PANEL_LINE    RGB(97, 153, 206)
#define CLR_PANEL_INNER   RGB(27, 82, 148)
#define CLR_PANEL_DK      RGB(12, 52, 101)
#define CLR_PANEL_GLOW    RGB(117, 191, 255)

/* 棋盘网格 */
#define CLR_GRID_BASE     RGB(24, 116, 224)
#define CLR_GRID_LINE     RGB(56, 146, 238)
#define CLR_GRID_LIGHT    RGB(37, 129, 231)
#define CLR_GRID_DARK     RGB(19, 98, 206)

/* 棋盘边框 */
#define CLR_BOARD_EDGE    RGB(80, 146, 220)
#define CLR_BOARD_EDGE_DN RGB(30, 84, 156)
#define CLR_WALL          RGB(90, 151, 230)
#define CLR_WALL_LINE     RGB(63, 112, 195)

/* 棋盘底色 */
#define CLR_BOARD_BASE    RGB(38, 135, 235)

/* 障碍物 */
#define CLR_OBS           RGB(112, 128, 175)
#define CLR_OBS_CRACK     RGB(84, 102, 145)

/* P1 蛇 */
#define CLR_S1_H          RGB(32, 205, 130)
#define CLR_S1_B1         RGB(28, 170, 95)
#define CLR_S1_B2         RGB(24, 132, 75)
#define CLR_S1_B3         RGB(19, 102, 60)

/* 橙色选项（替代原蓝色） */
#define CLR_S1B_H         RGB(255, 170, 20)
#define CLR_S1B_B1        RGB(255, 132, 20)
#define CLR_S1B_B2        RGB(245, 104, 12)
#define CLR_S1B_B3        RGB(210, 74, 8)

#define CLR_S1P_H         RGB(228, 92, 255)
#define CLR_S1P_B1        RGB(195, 66, 227)
#define CLR_S1P_B2        RGB(168, 41, 197)
#define CLR_S1P_B3        RGB(138, 28, 162)

/* P2 蛇 */
#define CLR_S2_H          RGB(255, 196, 70)
#define CLR_S2_B1         RGB(225, 169, 57)
#define CLR_S2_B2         RGB(195, 141, 48)
#define CLR_S2_B3         RGB(160, 115, 35)

/* AI 蛇（紫色） */
#define CLR_AI_H          RGB(190, 130, 255)
#define CLR_AI_B1         RGB(158, 103, 230)
#define CLR_AI_B2         RGB(128, 84, 198)
#define CLR_AI_B3         RGB(96, 65, 158)

/* 食物 */
#define CLR_BLUE_FOOD     RGB(255, 226, 96)
#define CLR_BLUE_GLOW     RGB(255, 246, 173)
#define CLR_RED_FOOD      RGB(255, 94, 102)
#define CLR_RED_GLOW      RGB(255, 150, 160)

/* 道具 */
#define CLR_TURBO         RGB(255, 214, 60)
#define CLR_TURBO_EDGE    RGB(255, 246, 168)
#define CLR_SHIELD        RGB(84, 197, 255)
#define CLR_SHIELD_EDGE   RGB(171, 230, 255)
#define CLR_SLOW          RGB(200, 119, 255)
#define CLR_SLOW_EDGE     RGB(230, 110, 110)
#define CLR_MAGNET        RGB(255, 131, 107)
#define CLR_MAGNET_EDGE   RGB(255, 188, 168)
#define CLR_FREEZE        RGB(138, 229, 255)
#define CLR_FREEZE_EDGE   RGB(220, 244, 255)
#define CLR_SHRINK        RGB(255, 118, 118)
#define CLR_SHRINK_EDGE   RGB(255, 110, 110)
#define CLR_GHOST         RGB(230, 230, 255)
#define CLR_GHOST_EDGE    RGB(180, 196, 255)
#define CLR_DOUBLE        RGB(255, 214, 89)
#define CLR_DOUBLE_EDGE   RGB(255, 239, 184)
#define CLR_NEG_BORDER    RGB(255, 120, 130)

/* 文字 */
#define CLR_TITLE         RGB(243, 252, 255)
#define CLR_TEXT          RGB(229, 235, 248)
#define CLR_GOLD          RGB(255, 215, 93)
#define CLR_HINT          RGB(168, 185, 217)

/* 按钮边框 */
#define CLR_BORDER        RGB(83, 101, 141)
#define CLR_BORDER_IN     RGB(34, 47, 72)
#define CLR_BORDER_HI     RGB(121, 160, 255)

/* 字体 */
#define FONT_TITLE _T("Microsoft YaHei")
#define FONT_UI    _T("Microsoft YaHei")
#define FONT_MONO  _T("Consolas")
#define FONT_SCALE(sz) (((sz) * 6 / 5) + 1)

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

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static COLORREF blendColor(COLORREF src, COLORREF dst, float t)
{
    int sr = GetRValue(src);
    int sg = GetGValue(src);
    int sb = GetBValue(src);
    int dr = GetRValue(dst);
    int dg = GetGValue(dst);
    int db = GetBValue(dst);

    if (t <= 0.0f) return src;
    if (t >= 1.0f) return dst;

    return RGB(
        clampi(sr + (int)((dr - sr) * t), 0, 255),
        clampi(sg + (int)((dg - sg) * t), 0, 255),
        clampi(sb + (int)((db - sb) * t), 0, 255)
    );
}

static COLORREF boardGlowAt(const COLORREF baseColor, int x, int y, int cx, int cy, int maxDist)
{
    float dx = (float)(x - cx);
    float dy = (float)(y - cy);
    float d = sqrtf(dx * dx + dy * dy);
    float k = 1.0f - d / (float)maxDist;

    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    k = 0.05f + 0.12f * (k * k);

    return blendColor(baseColor, RGB(230, 250, 255), k);
}

static void drawCheckerboard(void)
{
    int y;

    for (y = 0; y < WIN_H; y += 8) {
        int top = y * 3 / WIN_H;
        int r = GetRValue(CLR_BG_DARK) - 4 - top;
        int g = GetGValue(CLR_BG_DARK) + top * 2;
        int b = GetBValue(CLR_BG) + top * 3;

        if (r < 0) r = 0;
        if (g > 255) g = 255;
        if (g < 0) g = 0;
        if (b > 255) b = 255;
        if (b < 0) b = 0;

        setfillcolor(RGB(r, g, b));
        solidrectangle(0, y, WIN_W, y + 8);
    }
}

static void drawMapChecker(int ox, int oy, int mapSize)
{
    int x, y;
    int mapPx = mapSize * CELL_PX;
    int cx = ox + mapPx / 2;
    int cy = oy + mapPx / 2;
    int maxDist = (int)(sqrtf((float)(mapPx * mapPx) * 0.5f) + 1);
    COLORREF sepColor;

    for (y = 0; y < mapSize; y++) {
        for (x = 0; x < mapSize; x++) {
            int left = ox + x * CELL_PX;
            int top = oy + y * CELL_PX;
            int cellCx = left + CELL_PX / 2;
            int cellCy = top + CELL_PX / 2;
            COLORREF fill = boardGlowAt(CLR_BOARD_BASE, cellCx, cellCy, cx, cy, maxDist);

            setfillcolor(fill);
            solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
        }
    }

    sepColor = blendColor(CLR_PANEL_GLOW, RGB(8, 28, 68), 0.22f);
    setlinecolor(sepColor);
    setlinestyle(PS_SOLID, 1);

    for (x = 1; x < mapSize; x++) {
        line(ox + x * CELL_PX, oy + 1, ox + x * CELL_PX, oy + mapPx - 1);
    }
    for (y = 1; y < mapSize; y++) {
        line(ox + 1, oy + y * CELL_PX, ox + mapPx - 1, oy + y * CELL_PX);
    }
}

static void drawWoodFrame(int ox, int oy, int mapSize)
{
    int frameThick = 12;
    int x1 = ox - frameThick;
    int y1 = oy - frameThick;
    int x2 = ox + mapSize * CELL_PX + frameThick;
    int y2 = oy + mapSize * CELL_PX + frameThick;

    setfillcolor(CLR_BOARD_EDGE_DN);
    solidrectangle(x1, y1, x2, oy);
    solidrectangle(x1, oy + mapSize * CELL_PX, x2, y2);
    solidrectangle(x1, oy, ox, oy + mapSize * CELL_PX);
    solidrectangle(ox + mapSize * CELL_PX, oy, x2, oy + mapSize * CELL_PX);

    setfillcolor(CLR_BOARD_EDGE);
    solidrectangle(x1 + 2, y1 + 2, x2 - 2, y2 - 2);

    setfillcolor(CLR_PANEL);
    solidrectangle(ox, oy, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);

    setlinecolor(CLR_BOARD_EDGE_DN);
    rectangle(ox - 1, oy - 1, ox + mapSize * CELL_PX, oy + mapSize * CELL_PX);
    setlinecolor(CLR_PANEL_LINE);
    rectangle(ox - frameThick, oy - frameThick, x2, y2);
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
    setfillcolor(CLR_PANEL_DK);
    solidrectangle(x - 2, y - 2, x + w + 2, y + h + 2);
    setfillcolor(CLR_PANEL_LINE);
    solidrectangle(x, y, x + w, y + h);
    setfillcolor(fill);
    solidrectangle(x + 2, y + 2, x + w - 2, y + h - 2);
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

static void drawPixelGemFood(int left, int top, COLORREF fill, COLORREF glow, COLORREF ambient)
{
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX / 2;
    int outer = CELL_PX / 2 - 2;
    int inner = CELL_PX / 2 - 5;
    int i;
    int radius;
    POINT outPts[4];
    POINT inPts[4];

    for (i = 0; i < 4; i++) {
        radius = outer + 2 + i;
        COLORREF haloColor = blendColor(ambient, glow, 0.78f - i * 0.18f);

        setfillcolor(haloColor);
        setlinecolor(haloColor);
        solidcircle(cx, cy, radius);
    }

    outPts[0].x = cx;
    outPts[0].y = cy - outer;
    outPts[1].x = cx + outer;
    outPts[1].y = cy;
    outPts[2].x = cx;
    outPts[2].y = cy + outer;
    outPts[3].x = cx - outer;
    outPts[3].y = cy;

    inPts[0].x = cx;
    inPts[0].y = cy - inner;
    inPts[1].x = cx + inner;
    inPts[1].y = cy;
    inPts[2].x = cx;
    inPts[2].y = cy + inner;
    inPts[3].x = cx - inner;
    inPts[3].y = cy;

    setlinecolor(blendColor(fill, RGB(255, 255, 255), 0.18f));
    setfillcolor(blendColor(fill, RGB(255, 255, 255), 0.35f));
    solidpolygon(outPts, 4);

    setlinecolor(blendColor(fill, RGB(0, 0, 0), 0.12f));
    setfillcolor(fill);
    solidpolygon(inPts, 4);

    setlinecolor(blendColor(fill, RGB(255, 255, 255), 0.45f));
    line(cx - inner + 2, cy, cx, cy - inner + 3);
    line(cx, cy - inner + 3, cx + inner - 2, cy);
    line(cx + inner - 2, cy, cx, cy + inner - 3);
    line(cx, cy + inner - 3, cx - inner + 2, cy);

    setlinecolor(blendColor(fill, RGB(0, 0, 0), 0.35f));
    line(cx - inner + 2, cy + 2, cx + inner - 2, cy + 2);

    setlinecolor(fill);
    setfillcolor(blendColor(fill, RGB(255, 255, 255), 0.32f));
    POINT spark[3];
    spark[0].x = cx;
    spark[0].y = cy - outer + 2;
    spark[1].x = cx - 1;
    spark[1].y = cy;
    spark[2].x = cx;
    spark[2].y = cy + 2;
    solidpolygon(spark, 3);
}

static void drawPixelBlueFood(int left, int top, COLORREF ambient)
{
    drawPixelGemFood(left, top, CLR_BLUE_FOOD, CLR_BLUE_GLOW, ambient);
}

static void drawPixelRedFood(int left, int top, COLORREF ambient)
{
    drawPixelGemFood(left, top, CLR_RED_FOOD, CLR_RED_GLOW, ambient);
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
    settextstyle(FONT_SCALE(14), 0, _T("Microsoft YaHei"));
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
            settextstyle(FONT_SCALE(16), 0, _T("Consolas"));
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
        settextstyle(FONT_SCALE(22), 0, _T("Microsoft YaHei"));
        setbkmode(TRANSPARENT);
        for (i = 0; i < ACH_MAX; i++) {
            if (g->achNewUnlock & (1u << i)) {
                TCHAR buf[64];
                _stprintf(buf, _T("成就解锁: %s!"), achName(i));
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
    int count = 0;

    settextstyle(FONT_SCALE(13), 0, FONT_UI);

    if (itemHasEffect(g, player, EFF_TURBO)) {
        settextcolor(CLR_TURBO);
        _stprintf(buf, _T("极速: %.0fs"), itemTimer(g, player, EFF_TURBO));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_SLOW)) {
        settextcolor(CLR_SLOW);
        _stprintf(buf, _T("减速: %.0fs"), itemTimer(g, player, EFF_SLOW));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_MAGNET)) {
        settextcolor(CLR_MAGNET);
        _stprintf(buf, _T("磁铁: %.0fs"), itemTimer(g, player, EFF_MAGNET));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_GHOST)) {
        settextcolor(CLR_GHOST);
        _stprintf(buf, _T("穿墙: %.0fs"), itemTimer(g, player, EFF_GHOST));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_DOUBLE)) {
        settextcolor(CLR_DOUBLE);
        _stprintf(buf, _T("x2: %.0fs"), itemTimer(g, player, EFF_DOUBLE));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemHasEffect(g, player, EFF_FROZEN)) {
        settextcolor(CLR_FREEZE);
        _stprintf(buf, _T("冻结: %.0fs"), itemTimer(g, player, EFF_FROZEN));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }
    if (itemShieldCount(g, player) > 0) {
        settextcolor(CLR_SHIELD);
        _stprintf(buf, _T("护盾 x%d"), itemShieldCount(g, player));
        outtextxy(startX, y, buf);
        y += 18;
        count++;
    }

    if (count == 0) {
        settextcolor(CLR_HINT);
        _stprintf(buf, _T("无状态"));
        outtextxy(startX, y, buf);
    }
}

static void drawPixelButton(int index, LPCTSTR text, int hover)
{
    int x = MENU_BTN_X;
    int y = MENU_BTN_START_Y + (index - 1) * MENU_BTN_GAP;
    int w = MENU_BTN_W;
    int h = MENU_BTN_H;

    COLORREF border = hover ? CLR_BORDER_HI : CLR_BORDER;
    COLORREF fill = hover ? CLR_PANEL_SOFT : CLR_PANEL;

    drawPixelPanel(x, y, w, h, fill);
    setlinecolor(border);
    rectangle(x + 2, y + 2, x + w - 2, y + h - 2);

    settextstyle(FONT_SCALE(24), 0, FONT_UI);
    drawTextCenter(x, y, w, h, text, hover ? CLR_PANEL_GLOW : CLR_TEXT);
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
    if (snakeColor == SNAKE_COLOR_BLUE) return _T("橙色");
    if (snakeColor == SNAKE_COLOR_PURPLE) return _T("紫色");
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
    drawPixelPanel(x, y, w, h, CLR_PANEL_SOFT);
    setlinecolor(CLR_PANEL_GLOW);
    line(x + 6, y + h + 1, x + w - 6, y + h + 1);
}

static void drawInfoPanel(int x, int y, int w, int h, LPCTSTR title, LPCTSTR content, COLORREF titleColor)
{
    drawPixelPanel(x, y, w, h, CLR_PANEL_INNER);

    if (title && *title) {
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(titleColor);
        outtextxy(x + 12, y + 8, title);
        setlinecolor(CLR_PANEL_LINE);
        line(x + 10, y + 28, x + w - 10, y + 28);
    }

    if (content && *content) {
        settextstyle(FONT_SCALE(14), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        outtextxy(x + 12, y + 48, content);
    }
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

    settextstyle(FONT_SCALE(20), 0, _T("Microsoft YaHei"));
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
    int mapPx;
    mapSize = normalizeMapSize(g->mapSize);
    mapPx = mapSize * CELL_PX;

    drawWoodFrame(ox, oy, mapSize);
    drawMapChecker(ox, oy, mapSize);

    {
        int mapRadius = (int)(sqrtf((float)(mapPx * mapPx) * 0.5f) + 1);
        int mapCx = ox + mapPx / 2;
        int mapCy = oy + mapPx / 2;

        for (y = 0; y < mapSize; y++) {
            for (x = 0; x < mapSize; x++) {
                int left = ox + x * CELL_PX;
                int top = oy + y * CELL_PX;
                int cellCx = left + CELL_PX / 2;
                int cellCy = top + CELL_PX / 2;
                int cell = g->grid[y][x];
                COLORREF tileBg = boardGlowAt(CLR_BOARD_BASE, cellCx, cellCy, mapCx, mapCy, mapRadius);

                if (cell == CELL_WALL) {
                    setfillcolor(CLR_WALL);
                    setlinecolor(CLR_WALL_LINE);
                    solidrectangle(left, top, left + CELL_PX, top + CELL_PX);
                    rectangle(left, top, left + CELL_PX, top + CELL_PX);
                } else if (cell == CELL_OBS) {
                    drawPixelObs(left, top);
                } else if (cell == CELL_BLUE) {
                    drawPixelBlueFood(left, top, tileBg);
                } else if (cell == CELL_RED) {
                    drawPixelRedFood(left, top, tileBg);
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
    }

    /* 绘制场上道具 */
    if (g->config.itemMode == GAMEPLAY_ITEM && g->itemOnField != ITEM_NONE) {
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

    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawInfoPanel(20, 12, 270, 92, _T("P1"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        if (g->gameMode == MODE_DUAL_TIMED) {
            _stprintf(buf, _T("时间: %d:%02d"), (int)(g->matchTimer / 60), (int)g->matchTimer % 60);
            outtextxy(36, 64, buf);
        }

        drawInfoPanel(810, 12, 270, 92, _T("P2"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score2);
        outtextxy(826, 42, buf);
        if (g->gameMode == MODE_DUAL_TIMED) {
            _stprintf(buf, _T("倒计时: %d:%02d"), (int)(g->matchTimer / 60), (int)g->matchTimer % 60);
            outtextxy(826, 64, buf);
        }
    } else {
        drawInfoPanel(20, 12, 320, 92, _T("游戏信息"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        _stprintf(buf, _T("最高: %d"), g->highScore);
        outtextxy(36, 64, buf);
    }

    drawInfoPanel(WIN_W - 190, 12, 180, 92, _T("难度/地图"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("难度: %s"), difficultyName(g->difficulty));
    outtextxy(WIN_W - 174, 42, buf);
    _stprintf(buf, _T("地图: %dx%d"), mapSize, mapSize);
    outtextxy(WIN_W - 174, 64, buf);

    drawMapAt(g, ox, oy);

    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawEffectIndicators(g, 0, 830, 112);
        drawEffectIndicators(g, 1, 28, 112);
    } else {
        drawEffectIndicators(g, 0, WIN_W - 184, 112);
    }

    if (g->hasRed) {
        int barX = WIN_W - 184;
        int barY = 116;
        int barW = 156;
        int barH = 10;
        int filledW = (int)((float)barW * g->redTimer / (float)RED_TIMER_MAX);

        if (filledW < 0) filledW = 0;
        if (filledW > barW) filledW = barW;
        settextstyle(FONT_SCALE(14), 0, FONT_UI);
        settextcolor(CLR_TEXT);
        outtextxy(barX, barY - 14, _T("红色食物"));
        setfillcolor(CLR_PANEL_DK);
        solidrectangle(barX, barY, barX + barW, barY + barH);
        setfillcolor(CLR_RED_FOOD);
        solidrectangle(barX, barY, barX + filledW, barY + barH);
    }

    settextstyle(FONT_SCALE(15), 0, FONT_UI);
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawTextLeft(20, WIN_H - 28, _T("P1:WASD+J  P2:方向键+0  P:暂停  Q:退出"), CLR_HINT);
    } else if (g->gameMode == MODE_DUAL_MIRROR) {
        /* 镜像模式底部提示由 gfxDrawMirrorGame 绘制 */
    } else if (g->gameMode == MODE_SURVIVAL) {
        if (g->config.itemMode == GAMEPLAY_ITEM)
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  道具:靠近自动拾取  生存中"), CLR_HINT);
        else
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  生存中"), CLR_HINT);
    } else {
        if (g->config.itemMode == GAMEPLAY_ITEM)
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  道具:靠近自动拾取  P:暂停  Q:退出"), CLR_HINT);
        else
            drawTextLeft(20, WIN_H - 28, _T("WASD移动  J:加速  P:暂停  Q:退出"), CLR_HINT);
    }

    drawFloatTexts(g, ox, oy);
    if (comboCount(g) >= 2) {
        TCHAR cbuf[32];
        _stprintf(cbuf, _T("COMBO x%d !"), comboCount(g));
        settextstyle(FONT_SCALE(24), 0, FONT_UI);
        settextcolor(CLR_GOLD);
        outtextxy(WIN_W / 2 - 60, 22, cbuf);
    }

    if (g->gameMode == MODE_SURVIVAL) {
        TCHAR sbuf[32];
        int m = (int)g->elapsedTime / 60, s = (int)g->elapsedTime % 60;
        _stprintf(sbuf, _T("生存 %d:%02d"), m, s);
        settextstyle(FONT_SCALE(16), 0, FONT_MONO);
        settextcolor(CLR_PANEL_GLOW);
        outtextxy(WIN_W - 118, 112, sbuf);
    }

    drawAchNotification(g);
}

void gfxInit(void)
{
    initgraph(WIN_W, WIN_H);
    setbkcolor(CLR_BG_DARK);
    cleardevice();
    setfillcolor(CLR_BG_DARK);
    setlinecolor(CLR_BG_DARK);
    solidrectangle(0, 0, WIN_W, WIN_H);
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

    setfillcolor(CLR_BG_DARK);
    solidrectangle(0, 0, WIN_W, 120);

    drawInfoPanel(160, 20, 780, 86, _T("复古贪吃蛇"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(44), 0, FONT_TITLE);
    settextcolor(CLR_PANEL_GLOW);
    setbkmode(TRANSPARENT);
    drawTextCenter(0, 36, WIN_W, 50, _T("贪吃蛇"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_UI);
    _stprintf(buf, _T("最高分: %d"), g ? g->highScore : 0);
    drawTextCenter(0, 80, WIN_W, 24, buf, CLR_TEXT);

    if (menuPage == MENU_MAIN) {
        drawPixelButton(1, _T("1  单人模式"), hoverIndex == 1);
        drawPixelButton(2, _T("2  双人对战"), hoverIndex == 2);
        drawPixelButton(3, _T("3  生存模式"), hoverIndex == 3);
        drawPixelButton(4, _T("4  设置"), hoverIndex == 4);
        drawPixelButton(5, _T("5  成就查看"), hoverIndex == 5);

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
    else if (menuPage == MENU_SETTINGS) {
        int itemMode = g ? g->config.itemMode : GAMEPLAY_ITEM;
        int aiEnabled = g ? g->config.aiEnabled : 1;

        drawTextCenter(0, 150, WIN_W, 30, _T("游戏设置"), CLR_TEXT);
        _stprintf(buf, _T("1  蛇颜色: %s"), colorName(g ? g->config.snakeColor : SNAKE_COLOR_GREEN));
        drawPixelButton(1, buf, hoverIndex == 1);
        _stprintf(buf, _T("2  地图: %s"), mapSizeName(g ? g->config.mapSize : MAP_SIZE_LARGE));
        drawPixelButton(2, buf, hoverIndex == 2);
        _stprintf(buf, _T("3  玩法: %s"), itemMode == GAMEPLAY_ITEM ? _T("道具版") : _T("经典版"));
        drawPixelButton(3, buf, hoverIndex == 3);
        _stprintf(buf, _T("4  AI敌人: %s"), aiEnabled ? _T("开启") : _T("关闭"));
        drawPixelButton(4, buf, hoverIndex == 4);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回"), CLR_HINT);
    }
    else if (menuPage == MENU_ACHIEVEMENTS) {
        int i;
        int unlocked = 0;
        int y = 170;
        TCHAR status[12];

        if (g) {
            for (i = 0; i < ACH_MAX; i++) {
                if (g->achUnlocked & (1u << i)) unlocked++;
            }
        }

        drawTextCenter(0, 150, WIN_W, 30, _T("成就查看"), CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(18), 0, FONT_UI);
        _stprintf(buf, _T("已解锁: %d/%d"), unlocked, ACH_MAX);
        drawTextCenter(0, 178, WIN_W, 24, buf, CLR_GOLD);

        for (i = 0; i < ACH_MAX; i++) {
            COLORREF color = CLR_HINT;
            if (g && (g->achUnlocked & (1u << i))) {
                color = CLR_GOLD;
                _tcscpy(status, _T("[已解锁]"));
            } else {
                _tcscpy(status, _T("[未解锁]"));
            }
            settextstyle(FONT_SCALE(16), 0, FONT_UI);
            settextcolor(color);
            _stprintf(buf, _T("%s %s"), status, achName(i));
            outtextxy(80, y, buf);

            settextstyle(FONT_SCALE(13), 0, FONT_UI);
            if (g && (g->achUnlocked & (1u << i))) settextcolor(CLR_TEXT);
            else settextcolor(RGB(130, 150, 180));
            outtextxy(100, y + 20, achDesc(i));

            y += 36;
        }

        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回"), CLR_HINT);
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
    drawOverlayPanel(WIN_W / 2 - 140, WIN_H / 2 - 90, 280, 180);
    settextstyle(FONT_SCALE(40), 0, FONT_TITLE);
    drawTextCenter(0, WIN_H / 2 - 60, WIN_W, 50, _T("暂停"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(22), 0, FONT_UI);
    drawTextCenter(0, WIN_H / 2 - 2, WIN_W, 30, _T("按 P 继续"), CLR_TEXT);
    drawTextCenter(0, WIN_H / 2 + 28, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);

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

    settextstyle(FONT_SCALE(40), 0, FONT_UI);
    if (g && g->gameMode == MODE_DUAL_TIMED && g->matchTimer <= 0.0f)
        drawTextCenter(px, py + 18, pw, 50, _T("TIME UP"), CLR_GOLD);
    else
        drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), CLR_RED_FOOD);

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);

    FlushBatchDraw();
}

void gfxDrawGameOver(const GameState *g, int score, int highScore, int isNewRecord, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];

    if (g) drawGameContent(g);

    pw = 360;
    ph = 290;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(34), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, _T("游戏结束"), CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
    _stprintf(buf, _T("SCORE: %d"), score);
    drawTextCenter(px, py + 66, pw, 30, buf, CLR_TEXT);
    _stprintf(buf, _T("HI: %d"), highScore);
    drawTextCenter(px, py + 94, pw, 30, buf, CLR_TEXT);

    if (isNewRecord) {
        settextstyle(FONT_SCALE(20), 0, FONT_UI);
        drawTextCenter(px, py + 124, pw, 28, _T("NEW RECORD!"), CLR_GOLD);
    }

    drawKeyHintsVertical(px, pw, py + 164, hoverIndex);
    FlushBatchDraw();
}

void gfxDrawDualOver(const GameState *g, int winner, int score1, int score2, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || (winner != WIN_P1 && winner != WIN_P2)) title = _T("平局");
    else if (winner == WIN_P1 && score1 > score2) title = _T("P1 获胜");
    else if (winner == WIN_P2 && score2 > score1) title = _T("P2 获胜");
    else if (score1 == score2) title = _T("平局");
    else if (score1 > score2) title = _T("P1 获胜");
    else title = _T("P2 获胜");

    if (g) drawGameContent(g);

    pw = 360;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(32), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, title, CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
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
    s_mirrorBtnW = 128;
    s_mirrorBtnH = 42;

    setfillcolor(CLR_PANEL);
    solidrectangle(s_mirrorBtnX, s_mirrorBtnY,
                   s_mirrorBtnX + s_mirrorBtnW, s_mirrorBtnY + s_mirrorBtnH);
    setlinecolor(CLR_BORDER_HI);
    rectangle(s_mirrorBtnX + 2, s_mirrorBtnY + 2, s_mirrorBtnX + s_mirrorBtnW - 2, s_mirrorBtnY + s_mirrorBtnH - 2);
    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_PANEL_GLOW);
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

    drawInfoPanel(20, 12, 260, 92, _T("P1 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P1: %d"), g1 ? g1->score : 0);
    outtextxy(36, 42, buf);

    drawInfoPanel(MIRROR_MAP2_X, 12, 260, 92, _T("P2 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P2: %d"), g2 ? g2->score : 0);
    outtextxy(MIRROR_MAP2_X + 16, 42, buf);

    if (g1) {
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
        drawEffectIndicators(g1, 0, 30, 42);
    }

    if (g2) {
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
        drawEffectIndicators(g2, 1, MIRROR_MAP2_X + 20, 42);
    }

    if (p1Dead || p2Dead) {
        settextstyle(FONT_SCALE(24), 0, FONT_UI);
        if (p1Dead) {
            settextcolor(CLR_RED_FOOD);
            outtextxy(MIRROR_MAP1_X + 138, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        if (p2Dead) {
            settextcolor(CLR_RED_FOOD);
            outtextxy(MIRROR_MAP2_X + 138, MIRROR_MAP_Y + 230, _T("已阵亡"));
        }
        if (p1Dead != p2Dead) {
            drawMirrorEndButton();
        }
    }

    settextstyle(FONT_SCALE(15), 0, FONT_UI);
    settextcolor(CLR_HINT);
    outtextxy(50, WIN_H - 28, _T("P1:WASD+J"));
    outtextxy(MIRROR_MAP2_X, WIN_H - 28, _T("P2:方向键+0   P暂停  Q退出"));

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

    settextstyle(FONT_SCALE(40), 0, FONT_UI);
    drawTextCenter(px, py + 18, pw, 50, _T("GAME OVER"), CLR_RED_FOOD);

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);

    FlushBatchDraw();
}

void gfxDrawMirrorOver(int score1, int score2, int winner, int hoverIndex)
{
    int px, py, pw, ph;
    TCHAR buf[128];
    LPCTSTR title;

    if (winner == WIN_DRAW || score1 == score2)
        title = _T("平局");
    else if (winner == WIN_P1 || score1 > score2)
        title = _T("P1 获胜");
    else
        title = _T("P2 获胜");

    pw = 360;
    ph = 270;
    px = WIN_W / 2 - pw / 2;
    py = WIN_H / 2 - ph / 2;
    drawOverlayPanel(px, py, pw, ph);

    settextstyle(FONT_SCALE(32), 0, FONT_UI);
    drawTextCenter(px, py + 14, pw, 40, title, CLR_PANEL_GLOW);

    settextstyle(FONT_SCALE(20), 0, FONT_MONO);
    _stprintf(buf, _T("P1: %d    P2: %d"), score1, score2);
    drawTextCenter(px, py + 70, pw, 30, buf, CLR_TEXT);

    drawKeyHintsVertical(px, pw, py + 140, hoverIndex);
    FlushBatchDraw();
}
