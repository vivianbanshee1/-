#include <windows.h>
#include <easyx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "snake_render.h"
#include "snake_services.h"

/* ============================================================
 *  ===== 板块三：渲染与主流程 =====
 *  负责：画面绘制、菜单交互展示、主状态机入口与游戏循环。
 * ------------------------------------------------------------
 *  从 gfx.cpp 合并
 * ============================================================ */
/* ============================================================
 *  gfx.cpp - 视图与输入实现
 *
 *  将原先 gfx.h 中全部实现迁移到本文件。
 *  本文件不改变游戏逻辑，仅负责窗口绘制、输入解包与
 *  与 UI 相关的状态展示。
 * ============================================================ */


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

/* 水果配色 */
#define CLR_BLUEBERRY      RGB(72, 92, 200)
#define CLR_BLUEBERRY_HL   RGB(170, 200, 255)
#define CLR_BLUEBERRY_DARK RGB(40, 60, 140)
#define CLR_BLUEBERRY_LEAF RGB(80, 180, 80)
#define CLR_STRAWBERRY     RGB(230, 50, 70)
#define CLR_STRAWBERRY_HL  RGB(255, 170, 180)
#define CLR_STRAWBERRY_DK  RGB(150, 25, 40)
#define CLR_STRAWBERRY_LEAF RGB(70, 190, 80)
#define CLR_STRAWBERRY_SEED RGB(255, 220, 130)

/* UI 多彩点缀 */
#define CLR_ACCENT_CYAN    RGB(94, 220, 230)
#define CLR_ACCENT_LIME    RGB(180, 230, 90)
#define CLR_ACCENT_PINK    RGB(255, 130, 180)
#define CLR_ACCENT_AMBER   RGB(255, 200, 80)
#define CLR_ACCENT_GOLD    RGB(255, 215, 90)

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
 *  主题状态 - 仅现代主题
 * =================================================== */

static void drawTextCenter(int x, int y, int w, int h, LPCTSTR text, COLORREF color);
static void drawCheckerboard(void);

/* ===================================================
 *  菜单按钮命中缓存
 * =================================================== */
#define MENU_HIT_MAX 8

typedef struct {
    int x;
    int y;
    int w;
    int h;
} MenuHitRect;

static MenuHitRect s_menuBtns[MENU_HIT_MAX];
static int s_menuBtnCount = 0;

static void resetMenuButtons(void)
{
    s_menuBtnCount = 0;
}

static void addMenuButtonRect(int x, int y, int w, int h, int index)
{
    if (index >= 1 && index <= MENU_HIT_MAX) {
        s_menuBtns[index - 1].x = x;
        s_menuBtns[index - 1].y = y;
        s_menuBtns[index - 1].w = w;
        s_menuBtns[index - 1].h = h;
        if (index > s_menuBtnCount) s_menuBtnCount = index;
    }
}

int gfxHitMenuButton(int mx, int my)
{
    int i;
    for (i = 0; i < s_menuBtnCount; i++) {
        if (mx >= s_menuBtns[i].x && mx <= s_menuBtns[i].x + s_menuBtns[i].w &&
            my >= s_menuBtns[i].y && my <= s_menuBtns[i].y + s_menuBtns[i].h) {
            return i + 1;
        }
    }
    return 0;
}

/* ===================================================
 *  自定义角落按钮（背景图 / 背景音乐）
 * =================================================== */
#define MENU_CORNER_BTN_W       38
#define MENU_CORNER_BTN_H       38
#define MENU_CORNER_MARGIN_X    12
#define MENU_CORNER_MARGIN_Y    12
#define MENU_CORNER_BTN_GAP      8

static int s_menuHasBgImage = 0;
static IMAGE s_menuBgImage;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} MenuCornerButton;

static MenuCornerButton s_menuImageBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuMusicBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuClearImageBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};
static MenuCornerButton s_menuClearMusicBtn = {0, 0, MENU_CORNER_BTN_W, MENU_CORNER_BTN_H};

static int fileExistsInFileSystem(const char *path)
{
    return (path != NULL && *path != '\0' && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES);
}

static int pointInRect(int x, int y, const MenuCornerButton *r)
{
    if (!r) return 0;
    return x >= r->x && x <= r->x + r->w && y >= r->y && y <= r->y + r->h;
}

static void updateCornerButtonRects(void)
{
    s_menuImageBtn.x = MENU_CORNER_MARGIN_X;
    s_menuImageBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuImageBtn.w = MENU_CORNER_BTN_W;
    s_menuImageBtn.h = MENU_CORNER_BTN_H;

    s_menuClearImageBtn.x = s_menuImageBtn.x + MENU_CORNER_BTN_W + MENU_CORNER_BTN_GAP;
    s_menuClearImageBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuClearImageBtn.w = MENU_CORNER_BTN_W;
    s_menuClearImageBtn.h = MENU_CORNER_BTN_H;

    s_menuClearMusicBtn.x = WIN_W - 2 * MENU_CORNER_BTN_W - MENU_CORNER_BTN_GAP - MENU_CORNER_MARGIN_X;
    s_menuClearMusicBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuClearMusicBtn.w = MENU_CORNER_BTN_W;
    s_menuClearMusicBtn.h = MENU_CORNER_BTN_H;

    s_menuMusicBtn.x = WIN_W - MENU_CORNER_BTN_W - MENU_CORNER_MARGIN_X;
    s_menuMusicBtn.y = WIN_H - MENU_CORNER_BTN_H - MENU_CORNER_MARGIN_Y;
    s_menuMusicBtn.w = MENU_CORNER_BTN_W;
    s_menuMusicBtn.h = MENU_CORNER_BTN_H;
}

int gfxHitMenuCornerAction(int mx, int my)
{
    updateCornerButtonRects();
    if (pointInRect(mx, my, &s_menuImageBtn)) {
        return GFX_MENU_ACTION_IMAGE;
    }
    if (pointInRect(mx, my, &s_menuClearImageBtn)) {
        return GFX_MENU_ACTION_CLEAR_IMAGE;
    }
    if (pointInRect(mx, my, &s_menuClearMusicBtn)) {
        return GFX_MENU_ACTION_CLEAR_MUSIC;
    }
    if (pointInRect(mx, my, &s_menuMusicBtn)) {
        return GFX_MENU_ACTION_MUSIC;
    }
    return 0;
}

void gfxSetMenuBackgroundImage(const char *path)
{
    if (path == NULL || *path == '\0') {
        gfxClearMenuBackgroundImage();
        return;
    }

    if (!fileExistsInFileSystem(path)) {
        return;
    }

    loadimage(&s_menuBgImage, path, WIN_W, WIN_H, true);
    s_menuHasBgImage = 1;
}

void gfxClearMenuBackgroundImage(void)
{
    s_menuHasBgImage = 0;
}

int gfxHasMenuBackgroundImage(void)
{
    return s_menuHasBgImage;
}

static void drawMenuCornerButtons(void)
{
    MenuCornerButton imgBtn;
    MenuCornerButton musicBtn;
    MenuCornerButton clearImageBtn;
    MenuCornerButton clearMusicBtn;

    updateCornerButtonRects();
    imgBtn = s_menuImageBtn;
    musicBtn = s_menuMusicBtn;
    clearImageBtn = s_menuClearImageBtn;
    clearMusicBtn = s_menuClearMusicBtn;

    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    setbkmode(TRANSPARENT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(imgBtn.x, imgBtn.y, imgBtn.x + imgBtn.w, imgBtn.y + imgBtn.h);

    setfillcolor(s_menuHasBgImage ? CLR_PANEL_GLOW : CLR_HINT);
    solidcircle(imgBtn.x + imgBtn.w / 2, imgBtn.y + imgBtn.h / 2, 10);
    settextcolor(CLR_TEXT);
    drawTextCenter(imgBtn.x, imgBtn.y, imgBtn.w, imgBtn.h, _T("图"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(clearImageBtn.x, clearImageBtn.y, clearImageBtn.x + clearImageBtn.w, clearImageBtn.y + clearImageBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(clearImageBtn.x, clearImageBtn.y, clearImageBtn.w, clearImageBtn.h, _T("清图"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(clearMusicBtn.x, clearMusicBtn.y, clearMusicBtn.x + clearMusicBtn.w, clearMusicBtn.y + clearMusicBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(clearMusicBtn.x, clearMusicBtn.y, clearMusicBtn.w, clearMusicBtn.h, _T("清音"), CLR_TEXT);

    setfillcolor(CLR_PANEL_SOFT);
    setlinecolor(CLR_BORDER);
    solidrectangle(musicBtn.x, musicBtn.y, musicBtn.x + musicBtn.w, musicBtn.y + musicBtn.h);

    settextcolor(CLR_TEXT);
    drawTextCenter(musicBtn.x, musicBtn.y, musicBtn.w, musicBtn.h, _T("音"), CLR_TEXT);
}

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
    int cx, cy;
    int eye1x, eye2x, eye1y, eye2y;
    int eyeLen;

    setfillcolor(headColor);
    solidrectangle(left + 1, top + 1, left + CELL_PX - 1, top + CELL_PX - 1);

    cx = left + CELL_PX / 2;
    cy = top + CELL_PX / 2;
    eyeLen = CELL_PX / 4;

    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 2);

    if (dir == DIR_UP) {
        /* 相对中心线(x=cx)左右对称，朝上时在头部上半区 */
        eye1x = cx - CELL_PX / 6;
        eye2x = cx + CELL_PX / 6;
        eye1y = top + CELL_PX / 3;
        eye2y = eye1y;
        line(eye1x, eye1y, eye1x, eye1y + eyeLen);
        line(eye2x, eye2y, eye2x, eye2y + eyeLen);
    } else if (dir == DIR_DOWN) {
        /* 相对中心线(x=cx)左右对称，朝下时在头部下半区 */
        eye1x = cx - CELL_PX / 6;
        eye2x = cx + CELL_PX / 6;
        eye1y = top + CELL_PX * 2 / 3 - eyeLen;
        eye2y = eye1y;
        line(eye1x, eye1y, eye1x, eye1y + eyeLen);
        line(eye2x, eye2y, eye2x, eye2y + eyeLen);
    } else if (dir == DIR_LEFT) {
        /* 相对中心线(y=cy)上下对称，朝左时眼睛在左前方 */
        eye1y = cy - CELL_PX / 6;
        eye2y = cy + CELL_PX / 6;
        eye1x = left + CELL_PX / 3;
        eye2x = eye1x;
        line(eye1x, eye1y, eye1x + eyeLen, eye1y);
        line(eye2x, eye2y, eye2x + eyeLen, eye2y);
    } else {
        /* 相对中心线(y=cy)上下对称，朝右时眼睛在右前方 */
        eye1y = cy - CELL_PX / 6;
        eye2y = cy + CELL_PX / 6;
        eye1x = left + CELL_PX * 2 / 3 - eyeLen;
        eye2x = eye1x;
        line(eye1x, eye1y, eye1x + eyeLen, eye1y);
        line(eye2x, eye2y, eye2x + eyeLen, eye2y);
    }

    setlinestyle(PS_SOLID, 1);
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
    (void)ambient;
    /* 蓝莓：深紫实心圆 + 高光斑 + 顶部一片小绿叶 */
    int cx = left + CELL_PX / 2;
    int cy = top + CELL_PX * 2 / 3;
    int r = CELL_PX * 2 / 5;

    /* 暗光晕 */
    setfillcolor(CLR_BLUEBERRY_DARK);
    setlinecolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy, r + 1);

    /* 主体 */
    setfillcolor(CLR_BLUEBERRY);
    setlinecolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy, r);

    /* 高光斑（小白点） */
    setfillcolor(CLR_BLUEBERRY_HL);
    setlinecolor(CLR_BLUEBERRY_HL);
    solidcircle(cx - r / 3, cy - r / 3, r / 4);

    /* 顶部小绿叶（三角形） */
    {
        POINT leaf[3];
        leaf[0].x = cx;
        leaf[0].y = cy - r - 1;
        leaf[1].x = cx - r / 2;
        leaf[1].y = cy - r / 4;
        leaf[2].x = cx + r / 2;
        leaf[2].y = cy - r / 4;
        setfillcolor(CLR_BLUEBERRY_LEAF);
        setlinecolor(CLR_BLUEBERRY_LEAF);
        solidpolygon(leaf, 3);
    }

    /* 底部小果蒂（深色） */
    setfillcolor(CLR_BLUEBERRY_DARK);
    solidcircle(cx, cy + r - 1, 2);
}

static void drawPixelRedFood(int left, int top, COLORREF ambient)
{
    (void)ambient;
    /* 草莓：红色三角/倒锥形 + 顶部绿叶 + 黄色籽点 */
    int cx = left + CELL_PX / 2;
    int topY = top + CELL_PX / 3;
    int bottomY = top + CELL_PX - 2;
    int halfW = CELL_PX / 3;
    POINT body[3];

    body[0].x = cx - halfW; body[0].y = topY + 2;
    body[1].x = cx + halfW; body[1].y = topY + 2;
    body[2].x = cx;        body[2].y = bottomY;

    /* 主体深底 */
    POINT bodyShadow[3];
    bodyShadow[0].x = body[0].x - 1; bodyShadow[0].y = body[0].y - 1;
    bodyShadow[1].x = body[1].x + 1; bodyShadow[1].y = body[1].y - 1;
    bodyShadow[2].x = body[2].x;     bodyShadow[2].y = body[2].y + 1;
    setfillcolor(CLR_STRAWBERRY_DK);
    setlinecolor(CLR_STRAWBERRY_DK);
    solidpolygon(bodyShadow, 3);

    /* 主体红 */
    setfillcolor(CLR_STRAWBERRY);
    setlinecolor(CLR_STRAWBERRY_DK);
    solidpolygon(body, 3);

    /* 高光 */
    setfillcolor(CLR_STRAWBERRY_HL);
    setlinecolor(CLR_STRAWBERRY_HL);
    POINT hl[3] = {{cx - halfW/2, topY + 4}, {cx - halfW/4, topY + 4}, {cx - halfW/2, topY + halfW + 2}};
    solidpolygon(hl, 3);

    /* 顶部绿叶（5 片三角形围成） */
    setfillcolor(CLR_STRAWBERRY_LEAF);
    setlinecolor(CLR_STRAWBERRY_LEAF);
    solidcircle(cx, topY + 1, 4);
    /* 旁两片小叶 */
    {
        POINT side[3] = {{cx - 5, topY + 2}, {cx - 1, topY - 2}, {cx - 1, topY + 4}};
        POINT side2[3] = {{cx + 5, topY + 2}, {cx + 1, topY - 2}, {cx + 1, topY + 4}};
        solidpolygon(side, 3);
        solidpolygon(side2, 3);
    }

    /* 黄色籽点（5 颗小黄点散布在主体上） */
    setfillcolor(CLR_STRAWBERRY_SEED);
    setlinecolor(CLR_STRAWBERRY_SEED);
    solidcircle(cx - 3, topY + 9, 1);
    solidcircle(cx + 4, topY + 10, 1);
    solidcircle(cx - 5, topY + 14, 1);
    solidcircle(cx + 2, topY + 15, 1);
    solidcircle(cx + 6, topY + 15, 1);
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
    ((GameState *)g)->achNewUnlock = 0;
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

    addMenuButtonRect(x, y, w, h, index);
    drawPixelPanel(x, y, w, h, fill);
    setlinecolor(border);
    rectangle(x + 2, y + 2, x + w - 2, y + h - 2);

    /* 左上角色块点缀：5 个按钮各分配一个主题色 */
    {
        COLORREF accent = CLR_ACCENT_CYAN;
        switch (index) {
            case 1: accent = CLR_ACCENT_CYAN;  break;  /* 单人: 青 */
            case 2: accent = CLR_ACCENT_PINK;  break;  /* 双人: 粉 */
            case 3: accent = CLR_ACCENT_AMBER; break;  /* 玩法: 琥珀 */
            case 4: accent = CLR_ACCENT_LIME;  break;  /* 设置: 嫩绿 */
            case 5: accent = CLR_ACCENT_GOLD;  break;  /* 成就: 金 */
            default: break;
        }
        setfillcolor(accent);
        setlinecolor(accent);
        /* hover 时稍大；常态小圆点 */
        solidcircle(x + 14, y + 14, hover ? 7 : 5);
    }

    /* 右下角小三角点缀（hover 时出现） */
    if (hover) {
        POINT tri[3] = {
            { x + w - 16, y + h - 14 },
            { x + w - 6,  y + h - 14 },
            { x + w - 11, y + h - 6 }
        };
        setfillcolor(CLR_PANEL_GLOW);
        setlinecolor(CLR_PANEL_GLOW);
        solidpolygon(tri, 3);
    }

    settextstyle(FONT_SCALE(24), 0, FONT_UI);
    drawTextCenter(x, y, w, h, text, hover ? CLR_PANEL_GLOW : CLR_TEXT);
}

static void mapOrigin(const GameState *g, int *ox, int *oy)
{
    int mapSize = normalizeMapSize(g ? g->mapSize : MAP_SIZE_LARGE);
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
    (void)index; (void)len;
    snakeColor = normalizeSnakeColor(snakeColor);
    if (snakeColor == SNAKE_COLOR_BLUE)   return CLR_S1B_B1;
    if (snakeColor == SNAKE_COLOR_PURPLE) return CLR_S1P_B1;
    return CLR_S1_B1;
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
    (void)index; (void)len;
    return CLR_S2_B1;
}

static COLORREF getAIBodyColor(int index, int len)
{
    (void)index; (void)len;
    return CLR_AI_B1;
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
    if (!g) return;

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
                } else if (cell == CELL_ITEM) {
                    if (g->config.itemMode == GAMEPLAY_ITEM)
                        drawPixelItem(left, top, g->itemOnField);
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

    /* 先画地图，再画计分板，避免地图外框遮住左上角计分板 */
    drawMapAt(g, ox, oy);

    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        drawInfoPanel(20, 12, 270, 92, _T("P1"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        if (g->gameMode == MODE_DUAL_TIMED) {
            _stprintf(buf, _T("时间: %d:%02d"), (int)(g->matchTimer / 60), (int)g->matchTimer % 60);
            outtextxy(36, 64, buf);
        }

        drawInfoPanel(810, 12, 270, 92, _T("P2"), NULL, CLR_PANEL_GLOW);
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
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
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        _stprintf(buf, _T("得分: %d"), g->score);
        outtextxy(36, 42, buf);
        _stprintf(buf, _T("最高: %d"), g->highScore);
        outtextxy(36, 64, buf);
    }

    drawInfoPanel(WIN_W - 190, 12, 180, 92, _T("难度/地图"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("难度: %s"), difficultyName(g->difficulty));
    outtextxy(WIN_W - 174, 42, buf);
    _stprintf(buf, _T("地图: %dx%d"), mapSize, mapSize);
    outtextxy(WIN_W - 174, 64, buf);

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
    if (g->gameMode == MODE_DUAL || g->gameMode == MODE_DUAL_TIMED) {
        int c1 = comboCountForPlayer(g, 0);
        int c2 = comboCountForPlayer(g, 1);
        if (c1 >= 2 || c2 >= 2) {
            settextstyle(FONT_SCALE(20), 0, FONT_UI);
            settextcolor(CLR_GOLD);
            if (c1 >= 2) {
                TCHAR cbuf[32];
                _stprintf(cbuf, _T("P1 COMBO x%d"), c1);
                outtextxy(20, 22, cbuf);
            }
            if (c2 >= 2) {
                TCHAR cbuf[32];
                _stprintf(cbuf, _T("P2 COMBO x%d"), c2);
                outtextxy(WIN_W - 170, 22, cbuf);
            }
        }
    } else if (comboCount(g) >= 2) {
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

    resetMenuButtons();
    cleardevice();

    if (s_menuHasBgImage) {
        putimage(0, 0, &s_menuBgImage);
    } else {
        drawCheckerboard();
        setfillcolor(CLR_BG_DARK);
        solidrectangle(0, 0, WIN_W, 120);
    }

    drawMenuCornerButtons();

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
        drawPixelButton(3, _T("3  玩法说明"), hoverIndex == 3);
        drawPixelButton(4, _T("4  设置"), hoverIndex == 4);
        drawPixelButton(5, _T("5  成就查看"), hoverIndex == 5);

        drawTextCenter(0, 520, WIN_W, 30, _T("按 Q 退出"), CLR_HINT);
    }
    else if (menuPage == MENU_SINGLE_DIFF) {
        drawTextCenter(0, 150, WIN_W, 30, _T("选择单人玩法"), CLR_TEXT);
        drawPixelButton(1, _T("1  简单"), hoverIndex == 1);
        drawPixelButton(2, _T("2  普通"), hoverIndex == 2);
        drawPixelButton(3, _T("3  困难"), hoverIndex == 3);
        drawPixelButton(4, _T("4  生存模式"), hoverIndex == 4);
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
        _stprintf(buf, _T("4  AI敌人(单人): %s"), aiEnabled ? _T("开启") : _T("关闭"));
        drawPixelButton(4, buf, hoverIndex == 4);
        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回"), CLR_HINT);
    }
    else if (menuPage == MENU_HOWTOPLAY) {
        drawTextCenter(0, 150, WIN_W, 30, _T("玩法说明"), CLR_PANEL_GLOW);

        /* === 经典模式 vs 道具模式 对比 === */
        settextstyle(FONT_SCALE(16), 0, FONT_UI);
        setbkmode(TRANSPARENT);
        settextcolor(CLR_TEXT);
        outtextxy(60, 195, _T("经典模式:无道具,纯净贪吃蛇体验;蓝色食物 +10 倍率"));
        outtextxy(60, 215, _T("道具模式:场上随机刷道具,8 种效果改变战局"));

        settextcolor(CLR_ACCENT_AMBER);
        outtextxy(60, 240, _T("── 8 种道具 ──"));

        /* 道具列表 */
        struct { LPCTSTR name; LPCTSTR desc; COLORREF c; } items[] = {
            { _T("极速 Z"), _T("速度×3,持续5秒"),         CLR_TURBO },
            { _T("护盾 S"), _T("免死1次,可叠3层,15秒"),   CLR_SHIELD },
            { _T("减速 -"), _T("速度×1.5,持续4秒"),        CLR_SLOW },
            { _T("磁铁 M"), _T("3格内自动吃蓝食,6秒"),    CLR_MAGNET },
            { _T("冻结 F"), _T("冻结对手3秒(双人)"),       CLR_FREEZE },
            { _T("缩短 X"), _T("蛇尾减3节,即时生效"),     CLR_SHRINK },
            { _T("穿墙 G"), _T("穿过自己身体,6秒"),       CLR_GHOST },
            { _T("双倍 2"), _T("得分×2,持续10秒"),         CLR_DOUBLE },
        };
        int y = 270;
        int i;
        for (i = 0; i < 8; i++) {
            /* 色块 */
            setfillcolor(items[i].c);
            setlinecolor(items[i].c);
            solidcircle(75, y + 6, 5);
            /* 名称 + 描述 */
            settextstyle(FONT_SCALE(14), 0, FONT_UI);
            settextcolor(CLR_TEXT);
            outtextxy(95, y, items[i].name);
            settextcolor(CLR_HINT);
            outtextxy(180, y, items[i].desc);
            y += 22;
        }

        /* AI 敌人特性 */
        y += 8;
        settextcolor(CLR_ACCENT_PINK);
        outtextxy(60, y, _T("── AI 敌人(仅单人模式)──"));
        y += 24;
        settextstyle(FONT_SCALE(13), 0, FONT_UI);
        settextcolor(CLR_HINT);
        outtextxy(60, y, _T("贪心寻路:始终朝最近蓝食物移动 (Manhattan 距离)"));
        y += 18;
        outtextxy(60, y, _T("随机生成:场上最多3条,间隔18-30秒"));
        y += 18;
        outtextxy(60, y, _T("击杀奖励:撞死AI按其长度×蓝食分数得分"));
        y += 18;
        outtextxy(60, y, _T("多人模式下默认关闭,避免和玩家蛇互相干扰"));

        drawTextCenter(0, 520, WIN_W, 30, _T("M 返回 | Q 退出"), CLR_HINT);
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

void gfxDrawDeadTitle(const GameState *g, int remainingMs)
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
    if (remainingMs > 0) {
        TCHAR buf[32];
        _stprintf(buf, _T("缓冲中... %.1fs"), remainingMs / 1000.0f);
        drawTextCenter(px, py + 80, pw, 30, buf, CLR_ACCENT_AMBER);
    } else {
        drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);
    }

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

/* 镜像模式立即结算按钮 — 纯函数式坐标计算（避免渲染与输入检测的时序错位） */
static void computeMirrorEndRect(int *x, int *y, int *w, int *h)
{
    int cx = MIRROR_GAP_CX;
    int cy = WIN_H / 2 + 100;
    *w = 128;
    *h = 42;
    *x = cx - *w / 2;
    *y = cy;
}

static void drawMirrorEndButton(void)
{
    int bx, by, bw, bh;
    computeMirrorEndRect(&bx, &by, &bw, &bh);

    setfillcolor(CLR_PANEL);
    solidrectangle(bx, by, bx + bw, by + bh);
    setlinecolor(CLR_BORDER_HI);
    rectangle(bx + 2, by + 2, bx + bw - 2, by + bh - 2);
    settextstyle(FONT_SCALE(18), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_PANEL_GLOW);
    {
        RECT r = { bx, by, bx + bw, by + bh };
        drawtext(_T("立即结算"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

int gfxHitMirrorEndButton(int mx, int my)
{
    int bx, by, bw, bh;
    computeMirrorEndRect(&bx, &by, &bw, &bh);
    if (mx >= bx && mx <= bx + bw &&
        my >= by && my <= by + bh)
        return 1;
    return 0;
}

void gfxDrawMirrorGame(const GameState *g1, const GameState *g2,
                       int p1Dead, int p2Dead)
{
    TCHAR buf[128];

    cleardevice();
    drawCheckerboard();

    if (g1) {
        drawMapAt(g1, MIRROR_MAP1_X, MIRROR_MAP_Y);
    }

    if (g2) {
        drawMapAt(g2, MIRROR_MAP2_X, MIRROR_MAP_Y);
    }

    /* 计分板最后绘制，避免被地图覆盖 */
    drawInfoPanel(20, 12, 260, 92, _T("P1 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P1: %d"), g1 ? g1->score : 0);
    outtextxy(36, 42, buf);
    if (g1) drawEffectIndicators(g1, 0, 30, 62);

    drawInfoPanel(MIRROR_MAP2_X, 12, 260, 92, _T("P2 对战区"), NULL, CLR_PANEL_GLOW);
    settextstyle(FONT_SCALE(16), 0, FONT_UI);
    setbkmode(TRANSPARENT);
    settextcolor(CLR_TEXT);
    _stprintf(buf, _T("P2: %d"), g2 ? g2->score : 0);
    outtextxy(MIRROR_MAP2_X + 16, 42, buf);
    if (g2) drawEffectIndicators(g2, 1, MIRROR_MAP2_X + 20, 62);

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
    setbkmode(TRANSPARENT);
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
                            int p1Dead, int p2Dead, int remainingMs)
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
    if (remainingMs > 0) {
        TCHAR buf[32];
        _stprintf(buf, _T("缓冲中... %.1fs"), remainingMs / 1000.0f);
        drawTextCenter(px, py + 80, pw, 30, buf, CLR_ACCENT_AMBER);
    } else {
        drawTextCenter(px, py + 80, pw, 30, _T("按任意键继续..."), CLR_TEXT);
    }

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


/* ============================================================
 *  从 main.cpp 合并
 * ============================================================ */
/* ============================================================
 *  main.cpp - 主程序入口
 *  菜单逻辑、主循环
 * ============================================================ */


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
    DWORD lastDualTick;

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
    lastDualTick = lastTick;

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
                    lastDualTick = lastTick;
                    g.lastTickP1 = lastTick;
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
                    lastTick = g.lastTickP1;
                    lastDualTick = lastTick;
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
                    lastDualTick = lastTick;
                    g.lastTickP1 = lastTick;
                    g.lastTickP2 = lastTick;
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
                    if (isSoloMode(g.gameMode)) {
                        float dt = (float)(now - lastTick) / 1000.0f;
                        int moveReady;

                        lastTick = now;

                        gameApplySpeed(&g, gfxIsKeyDown(VK_BOOST_P1), 0);
                        moveReady = (now - g.lastTickP1 >= (DWORD)g.speed);
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

                        if (moveReady) {
                            if (gameMove(&g) == 0) {
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
                            g.lastTickP1 = now;
                        }
                    } else {
                        int ret = 1;
                        float dt = (float)(now - lastDualTick) / 1000.0f;
                        int p1Ready = (now - g.lastTickP1 >= (DWORD)g.speed);
                        int p2Ready = (now - g.lastTickP2 >= (DWORD)g.speed2);

                        gameApplySpeed(&g,
                                      gfxIsKeyDown(VK_BOOST_P1),
                                      gfxIsKeyDown(VK_BOOST_P2));
                        if (g.config.itemMode == GAMEPLAY_ITEM)
                            itemUpdate(&g, dt);
                        comboUpdateForPlayer(&g, dt, 0);
                        comboUpdateForPlayer(&g, dt, 1);
                        floatTextUpdate(&g, dt);
                        g.elapsedTime += dt;
                        g.diffFactor = 1.0f + g.elapsedTime / 120.0f;
                        if (g.diffFactor > 3.0f) g.diffFactor = 3.0f;
                        movingObsUpdate(&g, dt);
                        if (g.config.aiEnabled)
                            aiUpdate(&g, dt);
                        gameUpdateRedTimer(&g, dt);

                        if (p1Ready || p2Ready) {
                            ret = gameMoveDual(&g, p1Ready, p2Ready);
                            if (p1Ready) g.lastTickP1 = now;
                            if (p2Ready) g.lastTickP2 = now;
                        }

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

                        lastDualTick = now;
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
                    lastDualTick = resumeNow;
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
                    lastTick = g.lastTickP1;
                    lastDualTick = lastTick;
                } else if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED)
                    gameInitDual(&g, g.difficulty, g.gameMode, &cfg);
                else {
                    int restartMode = g.gameMode;
                    gameInit(&g, g.difficulty, &cfg);
                    if (restartMode == MODE_SURVIVAL)
                        g.gameMode = MODE_SURVIVAL;
                }
                lastTick = GetTickCount();
                lastDualTick = lastTick;
                g.lastTickP1 = lastTick;
                if (g.gameMode == MODE_DUAL || g.gameMode == MODE_DUAL_TIMED) {
                    g.lastTickP2 = lastTick;
                }
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
