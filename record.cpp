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
    }

    fp = fopen("record.txt", "r");
    if (!fp) return;

    if (highScore) fscanf(fp, "%d", highScore);
    while (fscanf(fp, "%63s %d", key, &value) == 2) {
        if (cfg && strcmp(key, "snakeColor") == 0)
            cfg->snakeColor = normalizeSnakeColor(value);
        else if (cfg && strcmp(key, "mapSize") == 0)
            cfg->mapSize = normalizeMapSize(value);
    }
    fclose(fp);
}

void saveRecordConfig(int highScore, const GameConfig *cfg)
{
    FILE *fp = fopen("record.txt", "w");
    if (!fp) return;
    fprintf(fp, "%d\n", highScore);
    fprintf(fp, "snakeColor %d\n", cfg ? normalizeSnakeColor(cfg->snakeColor) : SNAKE_COLOR_GREEN);
    fprintf(fp, "mapSize %d\n", cfg ? normalizeMapSize(cfg->mapSize) : MAP_SIZE_LARGE);
    fclose(fp);
}
