/* ============================================================
 *  input.h - 输入处理模块接口
 *
 *  负责：
 *  - 游戏过程中的键盘/鼠标输入
 *  - 菜单界面的键盘/鼠标输入（含悬停检测）
 *  - 死亡结算界面的键盘/鼠标输入（含按钮点击）
 *
 *  本模块从 gfx 模块中分离，独立处理所有用户输入。
 * ============================================================ */
#ifndef INPUT_H
#define INPUT_H

/* 加速键虚拟键码 */
#define VK_BOOST_P1  'J'
#define VK_BOOST_P2  '0'

/* 检测按键是否正在被按住（实时检测，不依赖消息队列） */
int gfxIsKeyDown(int vkcode);

/* 获取游戏过程中的按键（无输入返回0） */
int gfxGetKey(void);

/* 获取菜单界面的输入，自动更新悬停索引 */
int gfxGetMenuInput(int *hoverIndex);

/* 获取死亡结算界面的输入，自动更新悬停索引 */
int gfxGetDeadInput(int *hoverIndex);

#endif /* INPUT_H */
