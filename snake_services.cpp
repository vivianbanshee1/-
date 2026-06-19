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

#include "snake_services.h"

/* ============================================================
 *  ===== 板块二：系统服务层 =====
 *  负责：持久化配置/成就、音效与输入交互等外围服务。
 * ------------------------------------------------------------
 *  从 record.cpp 合并
 * ============================================================ */
/* ============================================================
 *  record.cpp - 记录持久化实现
 *
 *  从 snake.cpp 提取的文件读写逻辑。
 * ============================================================ */


static void trimString(char *text)
{
    char *p;
    size_t len;

    if (!text) return;

    while (isspace((unsigned char) *text)) {
        memmove(text, text + 1, strlen(text));
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char) text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

static int parseKeyValueLine(const char *srcLine, char *key, int keyCap,
                            char *value, int valueCap)
{
    char line[512];
    char *p;
    char *sep;
    size_t len;

    if (!srcLine || !key || keyCap <= 0 || !value || valueCap <= 0) {
        return 0;
    }

    snprintf(line, sizeof(line), "%s", srcLine);
    trimString(line);
    if (line[0] == '\0' || line[0] == '#') {
        return 0;
    }

    p = line;
    sep = strpbrk(p, " \t");
    if (!sep) {
        snprintf(key, keyCap, "%s", p);
        value[0] = '\0';
        return 1;
    }

    *sep = '\0';
    snprintf(key, keyCap, "%s", p);

    sep++;
    while (*sep && isspace((unsigned char) *sep)) {
        sep++;
    }

    len = strlen(sep);
    if (len >= (size_t) (valueCap)) {
        len = (size_t) (valueCap - 1);
    }

    memcpy(value, sep, len);
    value[len] = '\0';
    trimString(value);

    return 2;
}

static int parseIntValue(const char *value, int *out)
{
    if (!value || !*value || !out) return 0;
    if (sscanf(value, "%d", out) != 1) return 0;
    return 1;
}

static int readTopScore(FILE *fp, int *highScore)
{
    char line[512];

    if (!fp || !highScore) return 0;

    if (!fgets(line, sizeof(line), fp)) {
        *highScore = 0;
        return 0;
    }

    if (sscanf(line, "%d", highScore) != 1) {
        *highScore = 0;
        return 0;
    }

    return 1;
}

void loadRecordConfig(int *highScore, GameConfig *cfg)
{
    FILE *fp;
    char line[512];
    char key[64];
    char value[448];
    int valueInt;

    if (highScore) *highScore = 0;
    if (cfg) {
        cfg->snakeColor = SNAKE_COLOR_GREEN;
        cfg->mapSize = MAP_SIZE_LARGE;
        cfg->itemMode = GAMEPLAY_ITEM;
        cfg->aiEnabled = 1;
        cfg->menuBgImagePath[0] = '\0';
        cfg->menuMusicPath[0] = '\0';
    }

    fp = fopen("record.txt", "r");
    if (!fp) return;

    readTopScore(fp, highScore);

    while (fgets(line, sizeof(line), fp)) {
        int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

        if (parsed <= 0) {
            continue;
        }

        if (!cfg) {
            continue;
        }

        if (strcmp(key, "snakeColor") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->snakeColor = normalizeSnakeColor(valueInt);
            }
        } else if (strcmp(key, "mapSize") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->mapSize = normalizeMapSize(valueInt);
            }
        } else if (strcmp(key, "itemMode") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->itemMode = normalizeItemMode(valueInt);
            }
        } else if (strcmp(key, "aiEnabled") == 0) {
            if (parsed == 2 && parseIntValue(value, &valueInt)) {
                cfg->aiEnabled = normalizeAiEnabled(valueInt);
            }
        } else if (strcmp(key, "menuBgImagePath") == 0) {
            if (parsed == 2) {
                snprintf(cfg->menuBgImagePath, sizeof(cfg->menuBgImagePath), "%s", value);
            } else {
                cfg->menuBgImagePath[0] = '\0';
            }
        } else if (strcmp(key, "menuMusicPath") == 0) {
            if (parsed == 2) {
                snprintf(cfg->menuMusicPath, sizeof(cfg->menuMusicPath), "%s", value);
            } else {
                cfg->menuMusicPath[0] = '\0';
            }
        }
    }

    fclose(fp);
}

void saveRecordConfig(int highScore, const GameConfig *cfg)
{
    unsigned achMask = 0;
    int achBlueTotal = 0, achAIKills = 0, achShieldBlocks = 0;
    int hasAch = loadRecordAchievements(&achMask, &achBlueTotal, &achAIKills, &achShieldBlocks);
    FILE *fp = fopen("record.txt", "w");

    if (!fp) return;

    fprintf(fp, "%d\n", highScore);
    fprintf(fp, "snakeColor %d\n", cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN);
    fprintf(fp, "mapSize %d\n", cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE);
    fprintf(fp, "itemMode %d\n", cfg ? normalizeItemMode(cfg->itemMode) : GAMEPLAY_ITEM);
    fprintf(fp, "aiEnabled %d\n", cfg ? normalizeAiEnabled(cfg->aiEnabled) : 1);
    if (cfg) {
        fprintf(fp, "menuBgImagePath %s\n", cfg->menuBgImagePath[0] ? cfg->menuBgImagePath : "");
        fprintf(fp, "menuMusicPath %s\n", cfg->menuMusicPath[0] ? cfg->menuMusicPath : "");
    }
    if (hasAch) {
        fprintf(fp, "achMask %u\n", achMask);
        fprintf(fp, "achBlueTotal %d\n", achBlueTotal);
        fprintf(fp, "achAIKills %d\n", achAIKills);
        fprintf(fp, "achShieldBlocks %d\n", achShieldBlocks);
    }
    fclose(fp);
}

/* --- 成就数据合并到 record.txt ---
 * 格式：行首为 "ach " 前缀的字段由本模块管理，其它字段保持 record.txt 原样。
 * 这样 record.txt 成为唯一的存档文件。 */

static void rewriteRecordWithAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    FILE *fp = fopen("record.txt", "r");
    int highScore = 0;
    char line[512];

    typedef struct {
        char k[64];
        char v[448];
    } RecordLine;

    RecordLine extras[32];
    int extraCount = 0;
    char key[64];
    char value[448];
    int i;

    if (fp) {
        readTopScore(fp, &highScore);

        while (fgets(line, sizeof(line), fp)) {
            int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

            if (parsed <= 0) {
                continue;
            }
            if (strcmp(key, "achMask") == 0) continue;
            if (strcmp(key, "achBlueTotal") == 0) continue;
            if (strcmp(key, "achAIKills") == 0) continue;
            if (strcmp(key, "achShieldBlocks") == 0) continue;

            if (extraCount < 32) {
                snprintf(extras[extraCount].k, sizeof(extras[extraCount].k), "%s", key);
                snprintf(extras[extraCount].v, sizeof(extras[extraCount].v), "%s", (parsed == 2 ? value : ""));
                extraCount++;
            }
        }
        fclose(fp);
    }

    fp = fopen("record.txt", "w");
    if (!fp) return;

    fprintf(fp, "%d\n", highScore);
    for (i = 0; i < extraCount; i++) {
        if (extras[i].v[0] == '\0') {
            fprintf(fp, "%s\n", extras[i].k);
        } else {
            fprintf(fp, "%s %s\n", extras[i].k, extras[i].v);
        }
    }

    fprintf(fp, "achMask %u\n", mask);
    fprintf(fp, "achBlueTotal %d\n", blueTotal);
    fprintf(fp, "achAIKills %d\n", aiKills);
    fprintf(fp, "achShieldBlocks %d\n", shieldBlocks);
    fclose(fp);
}

int loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks)
{
    FILE *fp = fopen("record.txt", "r");
    char line[512];
    char key[64];
    char value[448];
    int valueInt;
    int found = 0;

    if (mask) *mask = 0;
    if (blueTotal) *blueTotal = 0;
    if (aiKills) *aiKills = 0;
    if (shieldBlocks) *shieldBlocks = 0;

    if (!fp) return 0;

    {
        int dummy = 0;
        readTopScore(fp, &dummy);
    }

    while (fgets(line, sizeof(line), fp)) {
        int parsed = parseKeyValueLine(line, key, sizeof(key), value, sizeof(value));

        if (parsed < 2) {
            continue;
        }

        if (strcmp(key, "achMask") == 0) {
            if (parseIntValue(value, &valueInt)) {
                if (mask) *mask = (unsigned) valueInt;
                found = 1;
            }
        }
        else if (strcmp(key, "achBlueTotal") == 0 && blueTotal && parseIntValue(value, &valueInt)) {
            *blueTotal = valueInt;
        }
        else if (strcmp(key, "achAIKills") == 0 && aiKills && parseIntValue(value, &valueInt)) {
            *aiKills = valueInt;
        }
        else if (strcmp(key, "achShieldBlocks") == 0 && shieldBlocks && parseIntValue(value, &valueInt)) {
            *shieldBlocks = valueInt;
        }
    }

    fclose(fp);
    return found;
}

void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    rewriteRecordWithAchievements(mask, blueTotal, aiKills, shieldBlocks);
}


/* ============================================================
 *  从 sound.cpp 合并
 * ============================================================ */
/* ============================================================
 *  sound.cpp - 简易音效实现
 *
 *  优先播放本地 wav 文件（可从网上下载后放入 assets/sounds），
 *  若文件不存在则回退到短频率 Beep。
 * ============================================================ */


static int s_soundEnabled = 1;
static char s_soundDir[MAX_PATH] = "assets/sounds";

static const char BGM_ALIAS[] = "SnakeBGM";
static char s_bgmPath[MAX_PATH] = "";
static int s_bgmLoaded = 0;
static int s_bgmPlaying = 0;

static void closeBackgroundMusicNoLock(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded) return;

    snprintf(cmd, sizeof(cmd), "close %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "stop %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "seek %s to start", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);

    s_bgmLoaded = 0;
    s_bgmPlaying = 0;
}

static int fileExists(const char *path)
{
    if (path == NULL || *path == '\0') return 0;
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static void soundResolveDir(void)
{
    char exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, exePath, (DWORD) (sizeof(exePath) - 1));
    if (len == 0 || len >= sizeof(exePath) - 1) return;

    char *sep = strrchr(exePath, '\\');
    if (sep == NULL) return;
    *sep = '\0';

    snprintf(s_soundDir, sizeof(s_soundDir), "%s/assets/sounds", exePath);
}

static int playSoundFile(const char *file)
{
    char path[MAX_PATH] = {0};
    if (file == NULL || *file == '\0') return 0;

    if (s_soundDir[0] != '\0') {
        snprintf(path, sizeof(path), "%s/%s", s_soundDir, file);
        if (fileExists(path) && PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)) {
            return 1;
        }
    }

    snprintf(path, sizeof(path), "%s", file);
    if (fileExists(path) && PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)) {
        return 1;
    }

    return 0;
}

static void playToneSeq(const unsigned *freq, const unsigned *ms, int n)
{
    int i;
    if (!s_soundEnabled || n <= 0) return;

    for (i = 0; i < n; i++) {
        if (freq[i] == 0 || ms[i] == 0) {
            Sleep((DWORD)ms[i]);
            continue;
        }
        Beep((unsigned int)freq[i], (int)ms[i]);
    }
}

static void playSoundOrTone(const char *file, const unsigned *freq, const unsigned *ms, int n)
{
    if (!s_soundEnabled) {
        return;
    }

    if (!playSoundFile(file)) {
        playToneSeq(freq, ms, n);
    }
}

void soundInit(void)
{
    soundResolveDir();
}

void soundSetEnabled(int enabled)
{
    s_soundEnabled = (enabled != 0);

    if (!s_soundEnabled) {
        soundStopBackgroundMusic();
    } else if (s_bgmLoaded) {
        soundPlayBackgroundMusic();
    }
}

int soundIsEnabled(void)
{
    return s_soundEnabled;
}

int soundSetBackgroundMusic(const char *filePath)
{
    char cmd[3 * MAX_PATH];
    int ret;

    closeBackgroundMusicNoLock();
    s_bgmPath[0] = '\0';

    if (filePath == NULL || *filePath == '\0') {
        return 0;
    }

    if (!fileExists(filePath)) {
        return 0;
    }

    snprintf(s_bgmPath, sizeof(s_bgmPath), "%s", filePath);

    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", filePath, BGM_ALIAS);
    ret = mciSendStringA(cmd, NULL, 0, NULL);

    if (ret != 0) {
        snprintf(cmd, sizeof(cmd), "open \"%s\" alias %s", filePath, BGM_ALIAS);
        ret = mciSendStringA(cmd, NULL, 0, NULL);
    }

    if (ret != 0) {
        s_bgmPath[0] = '\0';
        return 0;
    }

    s_bgmLoaded = 1;
    s_bgmPlaying = 0;

    if (s_soundEnabled) {
        soundPlayBackgroundMusic();
    }

    return 1;
}

void soundPlayBackgroundMusic(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded || !s_soundEnabled || s_bgmPlaying) {
        return;
    }

    snprintf(cmd, sizeof(cmd), "play %s repeat", BGM_ALIAS);
    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        s_bgmPlaying = 1;
    }
}

void soundStopBackgroundMusic(void)
{
    char cmd[MAX_PATH + 64];

    if (!s_bgmLoaded && s_bgmPlaying == 0) {
        return;
    }

    snprintf(cmd, sizeof(cmd), "stop %s", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "seek %s to start", BGM_ALIAS);
    mciSendStringA(cmd, NULL, 0, NULL);
    s_bgmPlaying = 0;
}

int soundHasBackgroundMusic(void)
{
    return s_bgmLoaded != 0;
}

void soundPlayMenuConfirm(void)
{
    static const unsigned f[] = { 740, 980, 1319 };
    static const unsigned d[] = { 22, 18, 30 };
    playSoundOrTone("menu_confirm.wav", f, d, 3);
}

void soundPlayEatBlue(void)
{
    static const unsigned f[] = { 1047, 1319, 1568 };
    static const unsigned d[] = { 24, 24, 26 };
    playSoundOrTone("eat_blue.wav", f, d, 3);
}

void soundPlayEatRed(void)
{
    static const unsigned f[] = { 784, 988, 1319 };
    static const unsigned d[] = { 28, 28, 40 };
    playSoundOrTone("eat_red.wav", f, d, 3);
}

void soundPlayItemCollect(int itemType)
{
    if (itemType == ITEM_TURBO || itemType == ITEM_SHIELD || itemType == ITEM_DOUBLE || itemType == ITEM_GHOST) {
        static const unsigned f[] = { 880, 1175, 1397 };
        static const unsigned d[] = { 25, 25, 30 };
        playSoundOrTone("item_collect_bright.wav", f, d, 3);
    } else if (itemType == ITEM_SLOW) {
        static const unsigned f[] = { 587, 659, 587, 523 };
        static const unsigned d[] = { 22, 22, 22, 28 };
        playSoundOrTone("item_collect_slow.wav", f, d, 4);
    } else if (itemType == ITEM_MAGNET || itemType == ITEM_FREEZE) {
        static const unsigned f[] = { 1047, 1175, 1175, 880 };
        static const unsigned d[] = { 20, 20, 20, 30 };
        playSoundOrTone("item_collect_magic.wav", f, d, 4);
    } else if (itemType == ITEM_SHRINK) {
        static const unsigned f[] = { 392, 330, 262, 220 };
        static const unsigned d[] = { 20, 20, 24, 24 };
        playSoundOrTone("item_collect_shrink.wav", f, d, 4);
    } else {
        static const unsigned f[] = { 1047, 988, 1047 };
        static const unsigned d[] = { 26, 26, 30 };
        playSoundOrTone("item_collect.wav", f, d, 3);
    }
}

void soundPlayDeath(void)
{
    static const unsigned f[] = { 880, 698, 523, 392 };
    static const unsigned d[] = { 30, 30, 40, 45 };
    playSoundOrTone("death.wav", f, d, 4);
}

void soundPlayPause(void)
{
    static const unsigned f[] = { 523, 0, 523 };
    static const unsigned d[] = { 18, 18, 18 };
    playSoundOrTone("pause.wav", f, d, 3);
}

void soundPlayResume(void)
{
    static const unsigned f[] = { 523, 659 };
    static const unsigned d[] = { 20, 20 };
    playSoundOrTone("resume.wav", f, d, 2);
}

void soundPlayShieldBlock(void)
{
    static const unsigned f[] = { 1568, 1568, 1175, 1568 };
    static const unsigned d[] = { 18, 18, 20, 40 };
    playSoundOrTone("shield.wav", f, d, 4);
}

void soundPlayGameOver(void)
{
    static const unsigned f[] = { 523, 440, 392, 330, 262, 196 };
    static const unsigned d[] = { 30, 30, 30, 30, 30, 60 };
    playSoundOrTone("game_over.wav", f, d, 6);
}


/* ============================================================
 *  从 input.cpp 合并
 * ============================================================ */
/* ============================================================
 *  input.cpp - 输入处理模块实现
 *
 *  从 gfx.cpp 提取的键盘/鼠标输入处理逻辑。
 *  依赖 gfx.h 中的 gfxHitDeadButton 进行死亡界面按钮命中检测。
 * ============================================================ */


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


/* ============================================================
 *  ===== 板块二结束 =====
 * ============================================================ */
