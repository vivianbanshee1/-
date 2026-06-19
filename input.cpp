/* ============================================================
 *  input.cpp - 输入处理模块实现
 *
 *  从 gfx.cpp 提取的键盘/鼠标输入处理逻辑。
 *  依赖 gfx.h 中的 gfxHitDeadButton 进行死亡界面按钮命中检测。
 * ============================================================ */
#include "input.h"
#include "gfx.h"

#include <easyx.h>
#include <windows.h>

static int s_windowClosePending = 0;

static void markWindowClosePending(void)
{
    s_windowClosePending = 1;
}

static int isCloseMessage(const MSG *msg)
{
    if (!msg) return 0;

    if (msg->message == WM_CLOSE || msg->message == WM_QUIT
        || msg->message == WM_DESTROY || msg->message == WM_NCDESTROY)
        return 1;

    if (msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_CLOSE))
        return 1;

    return 0;
}

int gfxWindowCloseRequested(void)
{
    MSG msg;

    if (s_windowClosePending) return 1;

    if (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_DESTROY, WM_DESTROY, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_NCDESTROY, WM_NCDESTROY, PM_REMOVE)) {
        markWindowClosePending();
        return 1;
    }
    if (PeekMessage(&msg, NULL, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE)) {
        if (isCloseMessage(&msg)) {
            markWindowClosePending();
            return 1;
        }
    }

    return 0;
}

int gfxGetKey(void)
{
    ExMessage m;
    /* 仅处理键盘消息，不消费鼠标消息。
     * 鼠标消息留给 pollMirrorEndButton() 等专门处理，避免镜像模式按钮点击被吞掉。 */
    while (peekmessage(&m, EX_KEY, true)) {
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

        if (m.message == WM_KEYDOWN) {
            if (m.vkcode == VK_UP) return KEY_UP;
            if (m.vkcode == VK_DOWN) return KEY_DOWN;
            if (m.vkcode == VK_LEFT) return KEY_LEFT;
            if (m.vkcode == VK_RIGHT) return KEY_RIGHT;
            if (m.vkcode >= 'a' && m.vkcode <= 'z') return m.vkcode - 'a' + 'A';
            return m.vkcode;
        }
    }
    return 0;
}

int gfxGetMenuInput(int *hoverIndex)
{
    ExMessage m;
    int mx, my, hit;

    while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

        if (m.message == WM_MOUSEMOVE) {
            mx = m.x; my = m.y;
            hit = gfxHitMenuButton(mx, my);
            if (hoverIndex) *hoverIndex = hit;
        }
        else if (m.message == WM_LBUTTONDOWN) {
            mx = m.x; my = m.y;
            hit = gfxHitMenuCornerAction(mx, my);
            if (hit == GFX_MENU_ACTION_MUSIC || hit == GFX_MENU_ACTION_IMAGE
                || hit == GFX_MENU_ACTION_CLEAR_IMAGE || hit == GFX_MENU_ACTION_CLEAR_MUSIC) {
                return hit;
            }

            hit = gfxHitMenuButton(mx, my);
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
        if (m.message == WM_CLOSE || m.message == WM_QUIT
            || (m.message == WM_SYSCOMMAND && ((m.wParam & 0xFFF0) == SC_CLOSE))) {
            markWindowClosePending();
            return 0;
        }

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
