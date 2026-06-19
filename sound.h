/* ============================================================
 *  sound.h - 音效接口
 *
 *  为游戏事件提供最小化的音效层。
 * ============================================================ */
#ifndef SOUND_H
#define SOUND_H

void soundInit(void);

void soundSetEnabled(int enabled);
int  soundIsEnabled(void);

void soundPlayMenuConfirm(void);
void soundPlayEatBlue(void);
void soundPlayEatRed(void);
void soundPlayItemCollect(int itemType);
void soundPlayDeath(void);
void soundPlayPause(void);
void soundPlayResume(void);
void soundPlayShieldBlock(void);
void soundPlayGameOver(void);

#endif /* SOUND_H */
