#pragma once
#include "AnmVm.hpp"
#include "Chain.hpp"
#include "Enemy.hpp"
#include "decomp.hpp"
#include <Windows.h>

namespace th06
{
struct GuiImpl;
struct GuiFlags
{
    u32 flag0 : 2;
    u32 flag1 : 2;
    u32 flag2 : 2;
    u32 flag3 : 2;
    u32 flag4 : 2;
    alignment_bitfields(u32, 22);
};
ZUN_ASSERT_TYPE(GuiFlags, 0x4, 4);

struct Gui
{

    ZunResult ActualAddedCallback();
    ZunResult LoadMsg(const char *path);
    void FreeMsgFile();

    ZunBool IsStageFinished();

    void UpdateStageElements();
    ZunBool HasCurrentMsgIdx();

    void DrawStageElements();
    void DrawGameScene();

    void MsgRead(i32 msgIdx);
    ZunBool MsgWait();

    void ShowSpellcard(i32 spellcardSprite, const char *spellcardName);
    void ShowSpellcardBonus(u32 spellcardScore);
    void ShowBombNamePortrait(u32 sprite, const char *bombName);
    void ShowBonusScore(u32 bonusScore);
    void EndEnemySpellcard();
    void EndPlayerSpellcard();
    ZunBool IsDialogueSkippable();

    void ShowFullPowerMode(i32 fmtArg);

    void SetBossHealthBar(f32 val)
    {
        this->bossHealthBar1 = val;
    }

    void SetBossLives(i32 lives)
    {
        this->eclSetLives = lives;
    }

    bool BossPresent()
    {
        return this->bossPresent;
    }

    void SetSpellcardSeconds(i32 val)
    {
        this->spellcardSecondsRemaining = val;
    }

    i32 SpellcardSecondsRemaining()
    {
        return this->spellcardSecondsRemaining;
    }

    GuiFlags flags;
    GuiImpl *impl;
    f32 bombSpellcardBarLength;
    f32 blueSpellcardBarLength;
    u32 bossUIOpacity;
    i32 eclSetLives;
    i32 spellcardSecondsRemaining;
    i32 lastSpellcardSecondsRemaining;
    bool bossPresent;
    alignment_padding(0x3);
    f32 bossHealthBar1;
    f32 bossHealthBar2;
};
ZUN_ASSERT_TYPE(Gui, 0x2c, 4);

ZunResult Gui_RegisterChain();
void Gui_CutChain();
ZunResult Gui_AddedCallback(Gui *);
ZunResult Gui_DeletedCallback(Gui *);
ChainCallbackResult Gui_OnUpdate(Gui *);
ChainCallbackResult Gui_OnDraw(Gui *);

DIFFABLE_EXTERN(Gui, g_Gui);
} // namespace th06
