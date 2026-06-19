# 板块二：系统服务层 — 教学文档

> **对应文件：** `snake_services.h` + `snake_services.cpp`  
> **模块定位：** 游戏的"后勤部队"——持久化存档、音效播放、键盘鼠标输入，全在这里。

---

## 1. 模块总览

```
snake_services 板块二
├── 存档子系统      record.cpp 部分     — record.txt 读写、配置/成就持久化
├── 音效子系统      sound.cpp 部分      — Beep回退音效 + WAV文件 + MCI背景音乐
└── 输入子系统      input.cpp 部分      — EasyX 消息循环、键盘/鼠标事件解包
```

本模块不涉及任何游戏逻辑——它只是为"板块一"和"板块三"提供数据持久化和人机交互的基础设施。

---

## 2. 存档子系统

### 2.1 文件格式：`record.txt`

```
98                          ← 第一行：最高分
snakeColor 0                ← 蛇颜色（0=绿 1=橙 2=紫）
mapSize 20                  ← 地图大小（16/18/20）
itemMode 1                  ← 玩法（0=经典 1=道具）
aiEnabled 1                 ← AI开关（0=关 1=开）
menuBgImagePath             ← 菜单背景图路径（可为空）
menuMusicPath               ← 菜单音乐路径（可为空）
achMask 37                  ← 成就位掩码
achBlueTotal 42             ← 累计蓝食物数
achAIKills 3                ← 累计AI击杀数
achShieldBlocks 1           ← 累计护盾格挡数
```

**设计决策：** 原先有 `record.txt`（最高分+配置）和 `achievements.txt`（成就）两个文件，后来统一合并到 `record.txt`，兼容读取旧版 `achievements.txt`。

### 2.2 配置读写流程

```
loadRecordConfig(&highScore, &cfg)
  ├─ fopen("record.txt", "r")
  ├─ readTopScore(fp, &highScore)    → 读第一行 int
  └─ while (fgets):
       parseKeyValueLine(line, key, value)
       ├─ "snakeColor" → cfg->snakeColor = normalizeSnakeColor(v)
       ├─ "mapSize"    → cfg->mapSize    = normalizeMapSize(v)
       ├─ "itemMode"   → cfg->itemMode   = normalizeItemMode(v)
       ├─ "aiEnabled"  → cfg->aiEnabled  = normalizeAiEnabled(v)
       ├─ "menuBgImagePath" → cfg->menuBgImagePath
       └─ "menuMusicPath"  → cfg->menuMusicPath

saveRecordConfig(highScore, &cfg)
  ├─ loadRecordAchievements()  → 先读出现有成就数据
  ├─ fopen("record.txt", "w")
  ├─ fprintf 最高分 → 配置字段 → 成就字段
  └─ fclose
```

### 2.3 Key-Value 解析器

```c
static int parseKeyValueLine(const char *srcLine, char *key, int keyCap,
                             char *value, int valueCap);
```

解析逻辑：
1. `trimString` 去除首尾空白
2. 跳过空行和 `#` 注释行
3. 用 `strpbrk` 按空格/Tab 分割 key 和 value
4. 返回 0=无效行，1=仅有key，2=key+value

### 2.4 成就数据合并写入

`rewriteRecordWithAchievements()` 使用了"读-改-写"模式：

```
1. 读取 record.txt 所有行
2. 保留非成就行（snakeColor/mapSize/...）
3. 丢弃旧成就行（achMask/achBlueTotal/...）
4. 重新写入：保留行 + 新成就行
```

这保证了多次写入不会产生重复的成就字段。

---

## 3. 音效子系统

### 3.1 架构：三层回退策略

```
尝试 WAV 文件播放（PlaySoundA）
    ↓ 失败
尝试 Beep 频率序列（Win32 Beep）
    ↓ s_soundEnabled = 0
全部静音
```

**WAV 文件路径解析：**
1. 先尝试 `<exe目录>/assets/sounds/<文件名>`
2. 再尝试当前工作目录

### 3.2 Beep 音效原理

```c
static void playToneSeq(const unsigned *freq, const unsigned *ms, int n) {
    for (i = 0; i < n; i++) {
        if (freq[i] == 0) Sleep(ms[i]);  // 休止符
        else Beep(freq[i], ms[i]);        // 发声
    }
}
```

`Beep(freq, duration)` 是 Windows API，用主板蜂鸣器发出指定频率的方波。频率单位为 Hz，时长单位为 ms。

**音效示例 — 吃蓝食物：**

```c
static const unsigned f[] = { 1047, 1319, 1568 };  // C6, E6, G6 → 大三和弦上行
static const unsigned d[] = { 24, 24, 26 };         // 24ms, 24ms, 26ms
```

| 事件 | 音符序列 | 效果 |
|---|---|---|
| 菜单确认 | 740→980→1319 | 上行三音 |
| 吃蓝食物 | 1047→1319→1568 | 大三和弦 |
| 吃红食物 | 784→988→1319 | 紧张上行 |
| 死亡 | 880→698→523→392 | 下行四音（悲壮） |
| 护盾格挡 | 1568→1568→1175→1568 | 金属感 |
| 拾取道具(正面) | 880→1175→1397 | 明亮 |
| 拾取道具(负面) | 392→330→262→220 | 低沉下行 |

### 3.3 MCI 背景音乐

使用 Windows MCI（Media Control Interface）播放 MP3/WAV 等格式的背景音乐：

```c
mciSendStringA("open \"bgm.mp3\" type mpegvideo alias SnakeBGM", ...);
mciSendStringA("play SnakeBGM repeat", ...);  // 循环播放
mciSendStringA("stop SnakeBGM", ...);         // 停止
mciSendStringA("close SnakeBGM", ...);        // 释放
```

**MCI 别名机制：** 用 `alias SnakeBGM` 给音乐文件起别名，后续所有操作通过别名引用，避免路径中的特殊字符问题。

**回退策略：** 如果 `type mpegvideo` 打开失败，尝试不带类型参数重新打开（适配不同系统的 MCI 驱动）。

### 3.4 音效开关

```c
void soundSetEnabled(int enabled) {
    s_soundEnabled = (enabled != 0);
    if (!s_soundEnabled) soundStopBackgroundMusic();  // 关闭时立即停止
    else if (s_bgmLoaded) soundPlayBackgroundMusic();  // 开启时恢复
}
```

全局变量 `s_soundEnabled` 控制所有音效和音乐。

---

## 4. 输入子系统

### 4.1 三层输入函数

| 函数 | 场景 | 消费的消息 |
|---|---|---|
| `gfxGetKey()` | 游戏中 | 仅键盘 |
| `gfxGetMenuInput()` | 菜单中 | 键盘 + 鼠标 |
| `gfxGetDeadInput()` | 死亡结算 | 键盘 + 鼠标 |

### 4.2 EasyX 消息循环

使用 EasyX 的 `peekmessage(&m, filter, true)` 非阻塞取消息。`filter` 指定 EX_KEY / EX_MOUSE 或二者组合。

```c
ExMessage m;
while (peekmessage(&m, EX_MOUSE | EX_KEY, true)) {
    if (m.message == WM_MOUSEMOVE)  { /* 悬停检测 */ }
    if (m.message == WM_LBUTTONDOWN) { /* 点击检测 */ }
    if (m.message == WM_KEYDOWN)    { /* 按键处理 */ }
    if (isCloseMessage(&m))         { /* 窗口关闭 */ }
}
```

### 4.3 按键映射

```c
// 游戏中
VK_UP    → KEY_UP    (0x101)
VK_DOWN  → KEY_DOWN  (0x102)
VK_LEFT  → KEY_LEFT  (0x103)
VK_RIGHT → KEY_RIGHT (0x104)
a-z      → A-Z       (自动转大写)
```

特殊按键常量（定义在 `snake_services.h`）：
```c
#define VK_BOOST_P1  'J'   // P1 加速键
#define VK_BOOST_P2  '0'   // P2 加速键
```

### 4.4 游戏内实时按键检测

```c
int gfxIsKeyDown(int vkcode) {
    return (GetAsyncKeyState(vkcode) & 0x8000) != 0;
}
```

`GetAsyncKeyState` 读取物理键盘的即时状态，不依赖消息队列。用于持续按键检测（如加速键按住不放）。注意 `& 0x8000` 检测最高位——该位为 1 表示键当前被按下。

### 4.5 窗口关闭检测

```c
static int isCloseMessage(const MSG *msg) {
    return msg->message == WM_CLOSE || msg->message == WM_QUIT
        || msg->message == WM_DESTROY || msg->message == WM_NCDESTROY
        || (msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_CLOSE));
}
```

检测以下关闭信号：
- `WM_CLOSE` — 窗口关闭按钮
- `WM_QUIT` — PostQuitMessage
- `WM_DESTROY` / `WM_NCDESTROY` — 窗口销毁
- `WM_SYSCOMMAND` + `SC_CLOSE` — 系统菜单关闭

并在 `gfxGetKey()` / `gfxGetMenuInput()` / `gfxGetDeadInput()` 中统一拦截。

### 4.6 菜单鼠标悬停/点击

```c
int gfxGetMenuInput(int *hoverIndex) {
    // WM_MOUSEMOVE → gfxHitMenuButton()  → 更新 hoverIndex
    // WM_LBUTTONDOWN → gfxHitMenuButton() → 返回 '1'~'5'
    //                 → gfxHitMenuCornerAction() → 返回特殊动作码
    // WM_KEYDOWN → 返回按键值
}
```

---

## 5. 数据流图

```
游戏启动
  │
  ├─→ loadRecordConfig(&highScore, &cfg)  读取存档
  ├─→ achLoad()                           读取成就
  ├─→ achLoadState()                      读取累计计数
  │
游戏运行中
  │
  ├─→ gfxGetKey / gfxGetMenuInput         读取输入
  ├─→ soundPlayEatBlue / ...              播放即时音效
  ├─→ saveRecordAchievements()            成就变更时立即写入
  │
游戏退出
  │
  └─→ saveRecordConfig(highScore, &cfg)   写入存档
```

**注意：** 成就是**即时写入**的（每次 `achUnlock` 都立即 `achSaveState`），而配置和最高分是**退出时写入**。这样即使游戏崩溃，已解锁的成就也不会丢失。

---

## 6. 容错设计

### 6.1 存档容错

- `record.txt` 不存在 → 全部使用默认值
- 某字段读取失败 → 该字段使用默认值，不影响其他字段
- 旧版 `achievements.txt` → 兼容读取，然后统一到 `record.txt`
- 写入时保留非成就字段 → 不会损坏其他模块的数据

### 6.2 音效容错

- WAV 文件不存在 → 回退 Beep
- Beep 不可用（远程桌面等）→ 静默跳过
- MCI 打开失败 → 回退无类型参数重试
- `s_soundEnabled = 0` → 全局静音

### 6.3 输入容错

- 窗口关闭消息在所有输入函数中统一拦截
- 按住不放（GetAsyncKeyState）与单次按键（消息循环）分层处理
- 鼠标坐标直接传递给模块三的命中检测函数
