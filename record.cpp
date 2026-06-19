/* ============================================================
 *  record.cpp - 记录持久化实现
 *
 *  从 snake.cpp 提取的文件读写逻辑。
 * ============================================================ */
#include "record.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
            if (strcmp(key, "menuBgImagePath") == 0) continue;
            if (strcmp(key, "menuMusicPath") == 0) continue;

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
