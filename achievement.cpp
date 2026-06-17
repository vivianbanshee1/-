/* ============================================================
 *  achievement.cpp - 成就系统实现
 * ============================================================ */
#include "achievement.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>

static const LPCTSTR aName[ACH_MAX] = {
    _T("初出茅庐"), _T("百炼成钢"), _T("大胃王"),
    _T("连击大师"), _T("蛇猎人"),   _T("钢铁之躯"),
    _T("速通达人"), _T("生存专家"), _T("镜像王者"),
    _T("满分猎人")
};

static const LPCTSTR aDesc[ACH_MAX] = {
    _T("首次得分超过100"),
    _T("累计吃掉100个蓝食物"),
    _T("单局吃掉20个蓝食物"),
    _T("达成5连击"),
    _T("累计杀死10条AI蛇"),
    _T("累计用护盾挡住10次死亡"),
    _T("60秒内获得200分"),
    _T("生存模式活过180秒"),
    _T("镜像对决中获胜"),
    _T("吃到满分的红食物")
};

static void parseCounterLine(const char *line, const char *key, int *out)
{
    const char *p = line;
    size_t keyLen = strlen(key);

    if (!out || !line || !key)
        return;

    while (*p && isspace((unsigned char)*p))
        p++;

    if (strncmp(p, key, keyLen) != 0)
        return;

    p += keyLen;
    if (*p == '\0' || isspace((unsigned char)*p) || *p == '=' || *p == ':') {
        while (*p && (isspace((unsigned char)*p) || *p == '=' || *p == ':'))
            p++;

        if (*p) {
            char *end = NULL;
            long val = strtol(p, &end, 10);
            if (end != p)
                *out = (int)val;
        }
    }
}

LPCTSTR achName(int achId) { return aName[achId]; }
LPCTSTR achDesc(int achId) { return aDesc[achId]; }

unsigned achLoad(void)
{
    FILE *fp = fopen("achievements.txt", "r");
    unsigned mask = 0;
    if (fp) {
        fscanf(fp, "%u", &mask);
        fclose(fp);
    }
    return mask;
}

void achLoadState(int *blueTotal, int *aiKills, int *shieldBlocks)
{
    FILE *fp = fopen("achievements.txt", "r");
    char line[128];

    if (blueTotal) *blueTotal = 0;
    if (aiKills) *aiKills = 0;
    if (shieldBlocks) *shieldBlocks = 0;

    if (!fp)
        return;

    while (fgets(line, sizeof(line), fp)) {
        parseCounterLine(line, "blueTotal", blueTotal);
        parseCounterLine(line, "aiKills", aiKills);
        parseCounterLine(line, "shieldBlocks", shieldBlocks);
    }

    fclose(fp);
}

void achSaveState(unsigned mask, int blueTotal, int aiKills, int shieldBlocks)
{
    FILE *fp = fopen("achievements.txt", "w");
    if (fp) {
        fprintf(fp, "%u\n", mask);
        fprintf(fp, "blueTotal %d\n", blueTotal);
        fprintf(fp, "aiKills %d\n", aiKills);
        fprintf(fp, "shieldBlocks %d\n", shieldBlocks);
        fclose(fp);
    }
}

int achUnlock(GameState *g, int achId)
{
    if (g->achUnlocked & (1u << achId)) return 0;
    g->achUnlocked |= (1u << achId);
    g->achNewUnlock |= (1u << achId);  /* 标记为新解锁（用于gfx通知） */
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
    return 1;
}

void achOnEatBlue(GameState *g)
{
    g->achBlueThisGame++;
    g->achBlueTotal++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

void achOnKillAI(GameState *g)
{
    g->achAIKills++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

void achOnShieldBlock(GameState *g)
{
    g->achShieldBlocks++;
    achSaveState(g->achUnlocked, g->achBlueTotal, g->achAIKills, g->achShieldBlocks);
}

int achCheckAll(GameState *g, int gameMode)
{
    int newCount = 0;

    if (g->score > 100)
        newCount += achUnlock(g, ACH_FIRST100);

    if (g->achBlueTotal >= 100)
        newCount += achUnlock(g, ACH_BLUE100);

    if (g->achBlueThisGame >= 20)
        newCount += achUnlock(g, ACH_BLUE20);

    if (g->maxCombo >= 5)
        newCount += achUnlock(g, ACH_COMBO5);

    if (g->achAIKills >= 10)
        newCount += achUnlock(g, ACH_KILLAI10);

    if (g->achShieldBlocks >= 10)
        newCount += achUnlock(g, ACH_SHIELD10);

    if (g->score >= 200 && g->elapsedTime <= 60.0f)
        newCount += achUnlock(g, ACH_SPEED200);

    if (gameMode == MODE_SURVIVAL && g->elapsedTime >= 180.0f)
        newCount += achUnlock(g, ACH_SURVIVE180);

    /* ACH_MIRRORWIN and ACH_RED50 checked externally */

    return newCount;
}
