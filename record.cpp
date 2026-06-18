/* ============================================================
 *  record.cpp - 记录持久化实现
 *
 *  从 snake.cpp 提取的文件读写逻辑。
 * ============================================================ */
#include "record.h"

#include <stdio.h>
#include <string.h>

void loadRecordConfig(int *highScore, GameConfig *cfg)
{
    FILE *fp;
    char key[64];
    int value;

    if (highScore) *highScore = 0;
    if (cfg) {
        cfg->snakeColor = SNAKE_COLOR_GREEN;
        cfg->mapSize = MAP_SIZE_LARGE;
        cfg->itemMode = GAMEPLAY_ITEM;
        cfg->aiEnabled = 1;
    }

    fp = fopen("record.txt", "r");
    if (!fp) return;

    if (highScore) fscanf(fp, "%d", highScore);
    while (fscanf(fp, "%63s %d", key, &value) == 2) {
        if (cfg && strcmp(key, "snakeColor") == 0)
            cfg->snakeColor = normalizeSnakeColor(value);
        else if (cfg && strcmp(key, "mapSize") == 0)
            cfg->mapSize = normalizeMapSize(value);
        else if (cfg && strcmp(key, "itemMode") == 0)
            cfg->itemMode = normalizeItemMode(value);
        else if (cfg && strcmp(key, "aiEnabled") == 0)
            cfg->aiEnabled = normalizeAiEnabled(value);
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
    char key[64];
    int value;
    /* 临时缓冲区 */
    struct { char k[64]; int v; } extras[16];
    int extraCount = 0;
    int i;

    if (fp) {
        fscanf(fp, "%d", &highScore);
        while (fscanf(fp, "%63s %d", key, &value) == 2) {
            if (strcmp(key, "achMask") == 0) continue;
            if (strcmp(key, "achBlueTotal") == 0) continue;
            if (strcmp(key, "achAIKills") == 0) continue;
            if (strcmp(key, "achShieldBlocks") == 0) continue;
            if (extraCount < 16) {
                strncpy(extras[extraCount].k, key, 63);
                extras[extraCount].k[63] = '\0';
                extras[extraCount].v = value;
                extraCount++;
            }
        }
        fclose(fp);
    }

    fp = fopen("record.txt", "w");
    if (!fp) return;
    fprintf(fp, "%d\n", highScore);
    for (i = 0; i < extraCount; i++)
        fprintf(fp, "%s %d\n", extras[i].k, extras[i].v);
    fprintf(fp, "achMask %u\n", mask);
    fprintf(fp, "achBlueTotal %d\n", blueTotal);
    fprintf(fp, "achAIKills %d\n", aiKills);
    fprintf(fp, "achShieldBlocks %d\n", shieldBlocks);
    fclose(fp);
}

int loadRecordAchievements(unsigned *mask, int *blueTotal, int *aiKills, int *shieldBlocks)
{
    FILE *fp = fopen("record.txt", "r");
    char key[64];
    int value;
    int found = 0;

    if (mask) *mask = 0;
    if (blueTotal) *blueTotal = 0;
    if (aiKills) *aiKills = 0;
    if (shieldBlocks) *shieldBlocks = 0;

    if (!fp) return 0;

    /* 第一行是最高分，跳过 */
    fscanf(fp, "%*d");
    while (fscanf(fp, "%63s %d", key, &value) == 2) {
        if (strcmp(key, "achMask") == 0) {
            if (mask) *mask = (unsigned)value;
            found = 1;
        }
        else if (strcmp(key, "achBlueTotal") == 0 && blueTotal) { *blueTotal = value; }
        else if (strcmp(key, "achAIKills") == 0 && aiKills) { *aiKills = value; }
        else if (strcmp(key, "achShieldBlocks") == 0 && shieldBlocks) { *shieldBlocks = value; }
    }
    fclose(fp);
    return found;
}

void saveRecordAchievements(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    rewriteRecordWithAchievements(mask, blueTotal, aiKills, shieldBlocks);
}
