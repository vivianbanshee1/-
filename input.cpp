/* ============================================================
 *  input.cpp - 输入处理模块实现
 *
 *  从 gfx.cpp 提取的键盘/鼠标输入处理逻辑。
 *  依赖 gfx.h 中的 gfxHitDeadButton 进行死亡界面按钮命中检测。
 * ============================================================ */
#include "input.h"
#include "gfx.h"

#include <easyx.h>

int gfxGetKey(void)
{
    ExMessage m;
    while (peekmessage(&m, EX_KEY | EX_MOUSE, true)) {
        if (m.message == WM_KEYDOWN) {
            if (m.vkcode == VK_UP) return KEY_UP;
            if (m.vkcode == VK_DOWN) return KEY_DOWN;
            if (m.vkcode == VK_LEFT) return KEY_LEFT;
            if (m.vkcode == VK_RIGHT) return KEY_RIGHT;
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
        if (m.message == WM_LBUTTONDOWN || m.message == WM_RBUTTONDOWN || m.message == WM_MBUTTONDOWN)
            return ' ';
    }
    return 0;
}

int gfxGetMenuInput(int *hoverIndex)
{
    ExMessage m;
    int i, mx, my, hit;

    while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
        if (m.message == WM_MOUSEMOVE) {
            mx = m.x; my = m.y; hit = 0;
            for (i = 1; i <= 5; i++) {
                int bx = MENU_BTN_X;
                int by = MENU_BTN_START_Y + (i - 1) * MENU_BTN_GAP;
                if (mx >= bx && mx <= bx + MENU_BTN_W && my >= by && my <= by + MENU_BTN_H) { hit = i; break; }
            }
            if (hoverIndex) *hoverIndex = hit;
        }
        else if (m.message == WM_LBUTTONDOWN) {
            mx = m.x; my = m.y; hit = 0;
            for (i = 1; i <= 5; i++) {
                int bx = MENU_BTN_X;
                int by = MENU_BTN_START_Y + (i - 1) * MENU_BTN_GAP;
                if (mx >= bx && mx <= bx + MENU_BTN_W && my >= by && my <= by + MENU_BTN_H) { hit = i; break; }
            }
            if (hoverIndex) *hoverIndex = hit;
            if (hit > 0) return '0' + hit;
        }
        else if (m.message == WM_KEYDOWN) {
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxGetDeadInput(int *hoverIndex)
{
    ExMessage m;
    int hit;
    static const char keys[3] = { 'R', 'M', 'Q' };

    while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
        if (m.message == WM_MOUSEMOVE) {
            hit = gfxHitDeadButton(m.x, m.y);
            if (hoverIndex) *hoverIndex = hit;
        }
        else if (m.message == WM_LBUTTONDOWN) {
            hit = gfxHitDeadButton(m.x, m.y);
            if (hoverIndex) *hoverIndex = hit;
            if (hit > 0) return keys[hit - 1];
        }
        else if (m.message == WM_KEYDOWN) {
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxIsKeyDown(int vkcode)
{
    return (GetAsyncKeyState(vkcode) & 0x8000) != 0;
}
