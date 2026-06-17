/* ============================================================
 *  floattext.h - 飘字系统接口
 *
 *  吃食物/触发事件时在画面上显示飘字动画。
 *  文字从产生点向上飘动，逐渐变淡消失。
 * ============================================================ */
#ifndef FLOATTEXT_H
#define FLOATTEXT_H

#include "snake.h"
#include <tchar.h>

/* 添加一条飘字 */
void floatTextAdd(GameState *g, float x, float y,
                  LPCTSTR text, COLORREF color, float life);

/* 更新所有飘字 */
void floatTextUpdate(GameState *g, float dt);

/* 清除所有飘字 */
void floatTextClearAll(GameState *g);

#endif
