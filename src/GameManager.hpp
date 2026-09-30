#pragma once
#include <Windows.h>
#include <d3d8.h>
#include <d3dx8math.h>

#include "Chain.hpp"
#include "Global.hpp"
#include "ResultScreen.hpp"
#include "ZunResult.hpp"
#include "decomp.hpp"

namespace th06
{
enum Difficulty
{
    EASY,
    NORMAL,
    HARD,
    LUNATIC,
    EXTRA,
};

enum StageNumber
{
    STAGE1,
    STAGE2,
    STAGE3,
    STAGE4,
    STAGE5,
    FINAL_STAGE,
    EXTRA_STAGE,
};

#define PSCR_NUM_STAGES 6
#define PSCR_NUM_DIFFICULTIES 4

#define CATK_NUM_CAPTURES 64

#define GAME_REGION_TOP 16.0f
#define GAME_REGION_LEFT 32.0f

#define GAME_REGION_WIDTH 384.0f
#define GAME_REGION_HEIGHT 448.0f

#define GAME_REGION_RIGHT (GAME_REGION_LEFT + GAME_REGION_WIDTH)
#define GAME_REGION_BOTTOM (GAME_REGION_TOP + GAME_REGION_HEIGHT)

#define MAX_CLEARS 99

#define MAX_POWER 128

struct GameManager;

DIFFABLE_EXTERN(GameManager, g_GameManager);
struct GameManager
{
    GameManager();
    static ZunResult RegisterChain();
    static void CutChain();
    static ChainCallbackResult OnUpdate(GameManager *gameManager);
    static ChainCallbackResult OnDraw(GameManager *gameManager);
    static ZunResult AddedCallback(GameManager *gameManager);
    static ZunResult DeletedCallback(GameManager *gameManager);
    static void SetupCamera(f32 extraRenderDistance);
    static void SetupCameraStageBackground(f32 extraRenderDistance);

    ZunBool HasReachedMaxClears(i32 character, i32 shottype)
    {
        return
            this->clrd[shottype + character * SHOTTYPES_PER_CHARACTER].difficultyClearedWithRetries[1] == MAX_CLEARS ||
            this->clrd[shottype + character * SHOTTYPES_PER_CHARACTER].difficultyClearedWithRetries[2] == MAX_CLEARS ||
            this->clrd[shottype + character * SHOTTYPES_PER_CHARACTER].difficultyClearedWithRetries[3] == MAX_CLEARS;
    }
    void IncreaseSubrank(i32 amount);
    void DecreaseSubrank(i32 amount);
    ZunBool IsInBounds(f32 x, f32 y, f32 width, f32 height);

    void AddScore(i32 points)
    {
        this->score += points;
    }

    static i32 CharacterShotType()
    {
        return g_GameManager.shotType + g_GameManager.character * SHOTTYPES_PER_CHARACTER;
    }

    u32 guiScore;
    u32 score;
    u32 nextScoreIncrement;
    u32 highScore;
    Difficulty difficulty;
    i32 grazeInStage;
    i32 grazeInTotal;
    ZunBool isInReplay;
    i32 deaths;
    i32 bombsUsed;
    i32 spellcardsCaptured;
    i8 isTimeStopped;
    Catk catk[CATK_NUM_CAPTURES];
    Clrd clrd[SHOTTYPE_COUNT];
    Pscr pscr[SHOTTYPE_COUNT][PSCR_NUM_STAGES][PSCR_NUM_DIFFICULTIES];
    u16 currentPower;
    unreferenced_fields(0x2);
    u16 pointItemsCollectedInStage;
    u16 pointItemsCollected;
    u8 numRetries;
    i8 powerItemCountForScore;
    i8 livesRemaining;
    i8 bombsRemaining;
    i8 extraLives;
    u8 character;
    u8 shotType;
    u8 isInGameMenu;
    u8 isInRetryMenu;
    u8 isInMenu;
    u8 isGameCompleted;
    u8 isInPracticeMode;
    u8 demoMode;
    alignment_padding(0x3);
    i32 demoFrames;
    char replayFile[256];
    unused_array_field(char, 256);
    u16 randomSeed;
    u32 gameFrames;
    i32 currentStage;
    u32 menuCursorBackup;
    D3DXVECTOR2 arcadeRegionTopLeftPos;
    D3DXVECTOR2 arcadeRegionSize;
    D3DXVECTOR2 playerMovementAreaTopLeftPos;
    D3DXVECTOR2 playerMovementAreaSize;
    f32 cameraDistance;
    D3DXVECTOR3 stageCameraFacingDir;
    i32 counat;
    i32 rank;
    i32 maxRank;
    i32 minRank;
    i32 subRank;
};
ZUN_ASSERT_TYPE(GameManager, 0x1a80, 4);
} // namespace th06
