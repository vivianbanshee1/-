/* ============================================================
 *  sound.cpp - 简易音效实现
 *
 *  优先播放本地 wav 文件（可从网上下载后放入 assets/sounds），
 *  若文件不存在则回退到短频率 Beep。
 * ============================================================ */
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

#include "sound.h"
#include "item.h"

static int s_soundEnabled = 1;
static char s_soundDir[MAX_PATH] = "assets\\sounds";

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
}

int soundIsEnabled(void)
{
    return s_soundEnabled;
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
